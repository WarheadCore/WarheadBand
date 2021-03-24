/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/** \file
    \ingroup Trinityd
*/

#include "ACSoap.h"
#include "BigNumber.h"
#include "CliRunnable.h"
#include "Common.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DatabaseWorkerPool.h"
#include "GitRevision.h"
#include "Log.h"
#include "Master.h"
#include "OpenSSLCrypto.h"
#include "RARunnable.h"
#include "RealmList.h"
#include "ScriptMgr.h"
#include "SignalHandler.h"
#include "Timer.h"
#include "Util.h"
#include "World.h"
#include "WorldRunnable.h"
#include "WorldSocket.h"
#include "WorldSocketMgr.h"
#include <ace/Sig_Handler.h>

#ifdef _WIN32
#include "ServiceWin32.h"
extern int m_ServiceStatus;
#endif

#ifdef __linux__
#include <sched.h>
#include <sys/resource.h>
#define PROCESS_HIGH_PRIORITY -15 // [-20, 19], default is 0
#endif

/// Handle worldservers's termination signals
class WorldServerSignalHandler : public Warhead::SignalHandler
{
public:
    void HandleSignal(int sigNum) override
    {
        switch (sigNum)
        {
            case SIGINT:
                World::StopNow(RESTART_EXIT_CODE);
                break;
            case SIGTERM:
#ifdef _WIN32
            case SIGBREAK:
                if (m_ServiceStatus != 1)
#endif
                    World::StopNow(SHUTDOWN_EXIT_CODE);
                break;
                /*case SIGSEGV:
                    LOG_INFO("server", "ZOMG! SIGSEGV handled!");
                    World::StopNow(SHUTDOWN_EXIT_CODE);
                    break;*/
        }
    }
};

class FreezeDetectorRunnable : public Warhead::Runnable
{
private:
    uint32 _loops;
    uint32 _lastChange;
    uint32 _delayTime;

public:
    FreezeDetectorRunnable(uint32 freezeDelay) : _loops(0), _lastChange(0), _delayTime(freezeDelay) {}

    void run() override
    {
        if (!_delayTime)
            return;

        LOG_INFO("server", "Starting up anti-freeze thread (%u seconds max stuck time)...", _delayTime / 1000);
        while (!World::IsStopped())
        {
            uint32 curtime = getMSTime();
            if (_loops != World::m_worldLoopCounter)
            {
                _lastChange = curtime;
                _loops = World::m_worldLoopCounter;
            }
            else if (getMSTimeDiff(_lastChange, curtime) > _delayTime)
            {
                LOG_INFO("server", "World Thread hangs, kicking out server!");
                ABORT();
            }

            Warhead::Thread::Sleep(1000);
        }
        LOG_INFO("server", "Anti-freeze thread exiting without problems.");
    }
};

Master* Master::instance()
{
    static Master instance;
    return &instance;
}

/// Main function
int Master::Run()
{
    OpenSSLCrypto::threadsSetup();
    BigNumber seed1;
    seed1.SetRand(16 * 8);

    LOG_INFO("server", "%s (worldserver-daemon)", GitRevision::GetFullVersion());
    LOG_INFO("server", "<Ctrl-C> to stop.\n");

    LOG_INFO("server", "   █████╗ ███████╗███████╗██████╗  ██████╗ ████████╗██╗  ██╗");
    LOG_INFO("server", "  ██╔══██╗╚══███╔╝██╔════╝██╔══██╗██╔═══██╗╚══██╔══╝██║  ██║");
    LOG_INFO("server", "  ███████║  ███╔╝ █████╗  ██████╔╝██║   ██║   ██║   ███████║");
    LOG_INFO("server", "  ██╔══██║ ███╔╝  ██╔══╝  ██╔══██╗██║   ██║   ██║   ██╔══██║");
    LOG_INFO("server", "  ██║  ██║███████╗███████╗██║  ██║╚██████╔╝   ██║   ██║  ██║");
    LOG_INFO("server", "  ╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝ ╚═════╝    ╚═╝   ╚═╝  ╚═╝");
    LOG_INFO("server", "                                ██████╗ ██████╗ ██████╗ ███████╗");
    LOG_INFO("server", "                                ██╔════╝██╔═══██╗██╔══██╗██╔═══╝");
    LOG_INFO("server", "                                ██║     ██║   ██║██████╔╝█████╗");
    LOG_INFO("server", "                                ██║     ██║   ██║██╔══██╗██╔══╝");
    LOG_INFO("server", "                                ╚██████╗╚██████╔╝██║  ██║███████╗");
    LOG_INFO("server", "                                 ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝\n");

    LOG_INFO("server", "     AzerothCore 3.3.5a  -  www.azerothcore.org\n");

    /// worldserver PID file creation
    std::string pidFile = sConfigMgr->GetOption<std::string>("PidFile", "");
    if (!pidFile.empty())
    {
        if (uint32 pid = CreatePIDFile(pidFile))
            LOG_ERROR("server", "Daemon PID: %u\n", pid); // outError for red color in console
        else
        {
            LOG_ERROR("server", "Cannot create PID file %s (possible error: permission)\n", pidFile.c_str());
            return 1;
        }
    }

    ///- Start the databases
    if (!_StartDB())
        return 1;

    // set server offline (not connectable)
    LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = (flag & ~%u) | %u WHERE id = '%d'", REALM_FLAG_OFFLINE, REALM_FLAG_INVALID, realmID);

    // Loading modules configs
    sConfigMgr->LoadModulesConfigs();

    ///- Initialize the World
    sWorld->SetInitialWorldSettings();

    sScriptMgr->OnStartup();

    ///- Initialize the signal handlers
    WorldServerSignalHandler signalINT, signalTERM; //, signalSEGV
#ifdef _WIN32
    WorldServerSignalHandler signalBREAK;
#endif /* _WIN32 */

    ///- Register worldserver's signal handlers
    ACE_Sig_Handler handle;
    handle.register_handler(SIGINT, &signalINT);
    handle.register_handler(SIGTERM, &signalTERM);
#ifdef _WIN32
    handle.register_handler(SIGBREAK, &signalBREAK);
#endif
    //handle.register_handler(SIGSEGV, &signalSEGV);

    ///- Launch WorldRunnable thread
    Warhead::Thread worldThread(new WorldRunnable);
    worldThread.setPriority(Warhead::Priority_Highest);

    Warhead::Thread* cliThread = nullptr;

#ifdef _WIN32
    if (sConfigMgr->GetOption<bool>("Console.Enable", true) && (m_ServiceStatus == -1)/* need disable console in service mode*/)
#else
    if (sConfigMgr->GetOption<bool>("Console.Enable", true))
#endif
    {
        ///- Launch CliRunnable thread
        cliThread = new Warhead::Thread(new CliRunnable);
    }

    Warhead::Thread rarThread(new RARunnable);

    // pussywizard:
    Warhead::Thread auctionLising_thread(new AuctionListingRunnable);
    auctionLising_thread.setPriority(Warhead::Priority_High);

#if defined(_WIN32) || defined(__linux__)

    ///- Handle affinity for multiple processors and process priority
    uint32 affinity = sConfigMgr->GetOption<int32>("UseProcessors", 0);
    bool highPriority = sConfigMgr->GetOption<bool>("ProcessPriority", false);

#ifdef _WIN32 // Windows

    HANDLE hProcess = GetCurrentProcess();

    if (affinity > 0)
    {
        ULONG_PTR appAff;
        ULONG_PTR sysAff;

        if (GetProcessAffinityMask(hProcess, &appAff, &sysAff))
        {
            ULONG_PTR currentAffinity = affinity & appAff;            // remove non accessible processors

            if (!currentAffinity)
                LOG_ERROR("server", "Processors marked in UseProcessors bitmask (hex) %x are not accessible for the worldserver. Accessible processors bitmask (hex): %x", affinity, appAff);
            else if (SetProcessAffinityMask(hProcess, currentAffinity))
                LOG_INFO("server", "Using processors (bitmask, hex): %x", currentAffinity);
            else
                LOG_ERROR("server", "Can't set used processors (hex): %x", currentAffinity);
        }
    }

    if (highPriority)
    {
        if (SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS))
            LOG_INFO("server", "worldserver process priority class set to HIGH");
        else
            LOG_ERROR("server", "Can't set worldserver process priority class.");
    }

#else // Linux

    if (affinity > 0)
    {
        cpu_set_t mask;
        CPU_ZERO(&mask);

        for (unsigned int i = 0; i < sizeof(affinity) * 8; ++i)
            if (affinity & (1 << i))
                CPU_SET(i, &mask);

        if (sched_setaffinity(0, sizeof(mask), &mask))
            LOG_ERROR("server", "Can't set used processors (hex): %x, error: %s", affinity, strerror(errno));
        else
        {
            CPU_ZERO(&mask);
            sched_getaffinity(0, sizeof(mask), &mask);
            LOG_INFO("server", "Using processors (bitmask, hex): %lx", *(__cpu_mask*)(&mask));
        }
    }

    if (highPriority)
    {
        if (setpriority(PRIO_PROCESS, 0, PROCESS_HIGH_PRIORITY))
            LOG_ERROR("server", "Can't set worldserver process priority class, error: %s", strerror(errno));
        else
            LOG_INFO("server", "worldserver process priority class set to %i", getpriority(PRIO_PROCESS, 0));
    }

#endif
#endif

    // Start soap serving thread
    Warhead::Thread* soapThread = nullptr;
    if (sConfigMgr->GetOption<bool>("SOAP.Enabled", false))
    {
        ACSoapRunnable* runnable = new ACSoapRunnable();
        runnable->SetListenArguments(sConfigMgr->GetOption<std::string>("SOAP.IP", "127.0.0.1"), uint16(sConfigMgr->GetOption<int32>("SOAP.Port", 7878)));
        soapThread = new Warhead::Thread(runnable);
    }

    // Start up freeze catcher thread
    Warhead::Thread* freezeThread = nullptr;
    if (uint32 freezeDelay = sConfigMgr->GetOption<int32>("MaxCoreStuckTime", 0))
    {
        FreezeDetectorRunnable* runnable = new FreezeDetectorRunnable(freezeDelay * 1000);
        freezeThread = new Warhead::Thread(runnable);
        freezeThread->setPriority(Warhead::Priority_Highest);
    }

    ///- Launch the world listener socket
    uint16 worldPort = uint16(sWorld->getIntConfig(CONFIG_PORT_WORLD));
    std::string bindIp = sConfigMgr->GetOption<std::string>("BindIP", "0.0.0.0");
    if (sWorldSocketMgr->StartNetwork(worldPort, bindIp.c_str()) == -1)
    {
        LOG_ERROR("server", "Failed to start network");
        World::StopNow(ERROR_EXIT_CODE);
        // go down and shutdown the server
    }

    // set server online (allow connecting now)
    LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = flag & ~%u, population = 0 WHERE id = '%u'", REALM_FLAG_INVALID, realmID);

    LOG_INFO("server", "%s (worldserver-daemon) ready...", GitRevision::GetFullVersion());

    // when the main thread closes the singletons get unloaded
    // since worldrunnable uses them, it will crash if unloaded after master
    worldThread.wait();
    rarThread.wait();
    auctionLising_thread.wait();

    if (soapThread)
    {
        soapThread->wait();
        soapThread->destroy();
        delete soapThread;
    }

    if (freezeThread)
    {
        freezeThread->wait();
        delete freezeThread;
    }

    // set server offline
    LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = flag | %u WHERE id = '%d'", REALM_FLAG_OFFLINE, realmID);

    ///- Clean database before leaving
    ClearOnlineAccounts();

    _StopDB();

    LOG_INFO("server", "Halting process...");

    if (cliThread)
    {
#ifdef _WIN32

        // this only way to terminate CLI thread exist at Win32 (alt. way exist only in Windows Vista API)
        //_exit(1);
        // send keyboard input to safely unblock the CLI thread
        INPUT_RECORD b[4];
        HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
        b[0].EventType = KEY_EVENT;
        b[0].Event.KeyEvent.bKeyDown = TRUE;
        b[0].Event.KeyEvent.uChar.AsciiChar = 'X';
        b[0].Event.KeyEvent.wVirtualKeyCode = 'X';
        b[0].Event.KeyEvent.wRepeatCount = 1;

        b[1].EventType = KEY_EVENT;
        b[1].Event.KeyEvent.bKeyDown = FALSE;
        b[1].Event.KeyEvent.uChar.AsciiChar = 'X';
        b[1].Event.KeyEvent.wVirtualKeyCode = 'X';
        b[1].Event.KeyEvent.wRepeatCount = 1;

        b[2].EventType = KEY_EVENT;
        b[2].Event.KeyEvent.bKeyDown = TRUE;
        b[2].Event.KeyEvent.dwControlKeyState = 0;
        b[2].Event.KeyEvent.uChar.AsciiChar = '\r';
        b[2].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
        b[2].Event.KeyEvent.wRepeatCount = 1;
        b[2].Event.KeyEvent.wVirtualScanCode = 0x1c;

        b[3].EventType = KEY_EVENT;
        b[3].Event.KeyEvent.bKeyDown = FALSE;
        b[3].Event.KeyEvent.dwControlKeyState = 0;
        b[3].Event.KeyEvent.uChar.AsciiChar = '\r';
        b[3].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
        b[3].Event.KeyEvent.wVirtualScanCode = 0x1c;
        b[3].Event.KeyEvent.wRepeatCount = 1;
        DWORD numb;
        WriteConsoleInput(hStdIn, b, 4, &numb);

        cliThread->wait();

#else

        cliThread->destroy();

#endif

        delete cliThread;
    }

    // for some unknown reason, unloading scripts here and not in worldrunnable
    // fixes a memory leak related to detaching threads from the module
    //UnloadScriptingModule();

    OpenSSLCrypto::threadsCleanup();
    // Exit the process with specified return value
    return World::GetExitCode();
}

/// Initialize connection to the databases
bool Master::_StartDB()
{
    MySQL::Library_Init();

    std::string dbstring;
    uint8 async_threads, synch_threads;

    dbstring = sConfigMgr->GetOption<std::string>("WorldDatabaseInfo", "");
    if (dbstring.empty())
    {
        LOG_ERROR("server", "World database not specified in configuration file");
        return false;
    }

    async_threads = uint8(sConfigMgr->GetOption<int32>("WorldDatabase.WorkerThreads", 1));
    if (async_threads < 1 || async_threads > 32)
    {
        LOG_ERROR("server", "World database: invalid number of worker threads specified. "
                       "Please pick a value between 1 and 32.");
        return false;
    }

    synch_threads = uint8(sConfigMgr->GetOption<int32>("WorldDatabase.SynchThreads", 1));
    ///- Initialise the world database
    if (!WorldDatabase.Open(dbstring, async_threads, synch_threads))
    {
        LOG_ERROR("server", "Cannot connect to world database %s", dbstring.c_str());
        return false;
    }

    ///- Get character database info from configuration file
    dbstring = sConfigMgr->GetOption<std::string>("CharacterDatabaseInfo", "");
    if (dbstring.empty())
    {
        LOG_ERROR("server", "Character database not specified in configuration file");
        return false;
    }

    async_threads = uint8(sConfigMgr->GetOption<int32>("CharacterDatabase.WorkerThreads", 1));
    if (async_threads < 1 || async_threads > 32)
    {
        LOG_ERROR("server", "Character database: invalid number of worker threads specified. "
                       "Please pick a value between 1 and 32.");
        return false;
    }

    synch_threads = uint8(sConfigMgr->GetOption<int32>("CharacterDatabase.SynchThreads", 2));

    ///- Initialise the Character database
    if (!CharacterDatabase.Open(dbstring, async_threads, synch_threads))
    {
        LOG_ERROR("server", "Cannot connect to Character database %s", dbstring.c_str());
        return false;
    }

    ///- Get login database info from configuration file
    dbstring = sConfigMgr->GetOption<std::string>("LoginDatabaseInfo", "");
    if (dbstring.empty())
    {
        LOG_ERROR("server", "Login database not specified in configuration file");
        return false;
    }

    async_threads = uint8(sConfigMgr->GetOption<int32>("LoginDatabase.WorkerThreads", 1));
    if (async_threads < 1 || async_threads > 32)
    {
        LOG_ERROR("server", "Login database: invalid number of worker threads specified. "
                       "Please pick a value between 1 and 32.");
        return false;
    }

    synch_threads = uint8(sConfigMgr->GetOption<int32>("LoginDatabase.SynchThreads", 1));
    ///- Initialise the login database
    if (!LoginDatabase.Open(dbstring, async_threads, synch_threads))
    {
        LOG_ERROR("server", "Cannot connect to login database %s", dbstring.c_str());
        return false;
    }

    ///- Get the realm Id from the configuration file
    realmID = sConfigMgr->GetOption<int32>("RealmID", 0);
    if (!realmID)
    {
        LOG_ERROR("server", "Realm ID not defined in configuration file");
        return false;
    }
    else if (realmID > 255)
    {
        /*
         * Due to the client only being able to read a realmID
         * with a size of uint8 we can "only" store up to 255 realms
         * anything further the client will behave anormaly
        */
        LOG_ERROR("server", "Realm ID must range from 1 to 255");
        return false;
    }

    LOG_INFO("server", "Realm running as realm ID %d", realmID);

    ///- Clean the database before starting
    ClearOnlineAccounts();

    ///- Insert version info into DB
    WorldDatabase.PExecute("UPDATE version SET core_version = '%s', core_revision = '%s'", GitRevision::GetFullVersion(), GitRevision::GetHash());        // One-time query

    sWorld->LoadDBVersion();

    LOG_INFO("server", "Using World DB: %s", sWorld->GetDBVersion());
    return true;
}

void Master::_StopDB()
{
    CharacterDatabase.Close();
    WorldDatabase.Close();
    LoginDatabase.Close();

    MySQL::Library_End();
}

/// Clear 'online' status for all accounts with characters in this realm
void Master::ClearOnlineAccounts()
{
    // Reset online status for all accounts with characters on the current realm
    // pussywizard: tc query would set online=0 even if logged in on another realm >_>
    LoginDatabase.DirectPExecute("UPDATE account SET online = 0 WHERE online = %u", realmID);

    // Reset online status for all characters
    CharacterDatabase.DirectExecute("UPDATE characters SET online = 0 WHERE online <> 0");
}
