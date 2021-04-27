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
#include "DatabaseLoader.h"
#include "DatabaseWorkerPool.h"
#include "GitRevision.h"
#include "Log.h"
#include "Master.h"
#include "MySQLThreading.h"
#include "ProcessPriority.h"
#include "OpenSSLCrypto.h"
#include "RARunnable.h"
#include "RealmList.h"
#include "ScriptMgr.h"
#include "SecretMgr.h"
#include "SignalHandler.h"
#include "ScriptLoader.h"
#include "Timer.h"
#include "Util.h"
#include "World.h"
#include "WorldRunnable.h"
#include "WorldSocket.h"
#include "WorldSocketMgr.h"

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
void HandleSignal(int sigNum)
{
    switch (sigNum)
    {
        case SIGINT:
            World::StopNow(RESTART_EXIT_CODE);
            break;
        case SIGTERM:
#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
        case SIGBREAK:
            if (m_ServiceStatus != 1)
#endif
                World::StopNow(SHUTDOWN_EXIT_CODE);
            break;
        default:
            break;
    }
}

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

    // Set process priority according to configuration settings
    SetProcessPriority("server.worldserver", sConfigMgr->GetOption<int32>(CONFIG_PROCESSOR_AFFINITY, 0), sConfigMgr->GetOption<bool>(CONFIG_HIGH_PRIORITY, false));

    ///- Start the databases
    if (!_StartDB())
        return 1;

    // set server offline (not connectable)
    LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = (flag & ~%u) | %u WHERE id = '%d'", REALM_FLAG_OFFLINE, REALM_FLAG_VERSION_MISMATCH, realmID);

    // Loading modules configs
    sConfigMgr->LoadModulesConfigs();

    ///- Initialize the World
    sSecretMgr->Initialize();
    sScriptMgr->SetScriptLoader(AddScripts);
    sWorld->SetInitialWorldSettings();

    sScriptMgr->OnStartup();

    ///- Initialize the signal handlers
    Warhead::SignalHandler signalHandler;

    signalHandler.handle_signal(SIGINT, &HandleSignal);
    signalHandler.handle_signal(SIGTERM, &HandleSignal);

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
    signalHandler.handle_signal(SIGBREAK, &HandleSignal);
#endif

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
    LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = flag & ~%u, population = 0 WHERE id = '%u'", REALM_FLAG_VERSION_MISMATCH, realmID);

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

    // Load databases
    DatabaseLoader loader("server.worldserver", DatabaseLoader::DATABASE_NONE);
    loader
        .AddDatabase(LoginDatabase, "Login")
        .AddDatabase(CharacterDatabase, "Character")
        .AddDatabase(WorldDatabase, "World");

    if (!loader.Load())
        return false;

    ///- Get the realm Id from the configuration file
    realmID = sConfigMgr->GetIntDefault("RealmID", 0);
    if (!realmID)
    {
        LOG_ERROR("server.worldserver", "Realm ID not defined in configuration file");
        return false;
    }
    else if (realmID > 255)
    {
        /*
         * Due to the client only being able to read a realmID
         * with a size of uint8 we can "only" store up to 255 realms
         * anything further the client will behave anormaly
        */
        LOG_ERROR("server.worldserver", "Realm ID must range from 1 to 255");
        return false;
    }

    LOG_INFO("server.loading", "Loading world information...");
    LOG_INFO("server.loading", "> RealmID:              %u", realmID);

    ///- Clean the database before starting
    ClearOnlineAccounts();

    ///- Insert version info into DB
    WorldDatabase.PExecute("UPDATE version SET core_version = '%s', core_revision = '%s'", GitRevision::GetFullVersion(), GitRevision::GetHash());        // One-time query

    sWorld->LoadDBVersion();

    LOG_INFO("server.loading", "> Version DB world:     %s", sWorld->GetDBVersion());
    LOG_INFO("server.loading", "");

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
