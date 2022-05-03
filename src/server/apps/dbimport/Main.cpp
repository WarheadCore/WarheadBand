/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Common.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DatabaseLoader.h"
#include "IoContext.h"
#include "Log.h"
#include "Logo.h"
#include "MySQLThreading.h"
#include "Util.h"
#include <boost/version.hpp>
#include <csignal>
#include <openssl/crypto.h>
#include <openssl/opensslv.h>

#ifndef _WARHEAD_DB_IMPORT_CONFIG
#define _WARHEAD_DB_IMPORT_CONFIG "dbimport.conf"
#endif

bool StartDB();
void StopDB();

/// Print out the usage string for this program on the console.
void usage(const char* prog)
{
    LOG_INFO("server.authserver", "Usage: \n {} [<options>]\n"
        "    -c config_file           use config_file as configuration file\n\r", prog);
}

/// Launch the auth server
int main(int argc, char** argv)
{
    signal(SIGABRT, &Warhead::AbortHandler);

    // Command line parsing to get the configuration file name
    std::string configFile = sConfigMgr->GetConfigPath() + std::string(_WARHEAD_DB_IMPORT_CONFIG);
    int count = 1;

    while (count < argc)
    {
        if (strcmp(argv[count], "--dry-run") == 0)
        {
            sConfigMgr->setDryRun(true);
        }

        if (strcmp(argv[count], "-c") == 0)
        {
            if (++count >= argc)
            {
                printf("Runtime-Error: -c option requires an input argument\n");
                usage(argv[0]);
                return 1;
            }
            else
                configFile = argv[count];
        }

        ++count;
    }

    // Add file and args in config
    sConfigMgr->Configure(configFile, std::vector<std::string>(argv, argv + argc));

    if (!sConfigMgr->LoadAppConfigs())
        return 1;

    // Init logging
    sLog->Initialize();

    Warhead::Logo::Show("dbimport",
        [](std::string_view text)
        {
            LOG_INFO("dbimport", text);
        },
        []()
        {
            LOG_INFO("dbimport", "> Using configuration file:       {}", sConfigMgr->GetFilename());
            LOG_INFO("dbimport", "> Using SSL version:              {} (library: {})", OPENSSL_VERSION_TEXT, SSLeay_version(SSLEAY_VERSION));
            LOG_INFO("dbimport", "> Using Boost version:            {}.{}.{}", BOOST_VERSION / 100000, BOOST_VERSION / 100 % 1000, BOOST_VERSION % 100);
        }
    );

    // Initialize the database connection
    if (!StartDB())
        return 1;

    std::shared_ptr<void> dbHandle(nullptr, [](void*) { StopDB(); });

    LOG_INFO("dbimport", "Halting process...");

    return 0;
}

/// Initialize connection to the database
bool StartDB()
{
    MySQL::Library_Init();

    // Load databases
    DatabaseLoader loader("dbimport");
    loader
        .AddDatabase(LoginDatabase, "Login")
        .AddDatabase(CharacterDatabase, "Character")
        .AddDatabase(WorldDatabase, "World");

    if (!loader.Load())
        return false;

    LOG_INFO("dbimport", "Started database connection pool.");
    return true;
}

/// Close the connection to the database
void StopDB()
{
    CharacterDatabase.Close();
    WorldDatabase.Close();
    LoginDatabase.Close();
    MySQL::Library_End();
}
