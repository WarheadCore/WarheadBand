/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 * Copyright (C) 2021+ WarheadCore <https://github.com/WarheadCore>
 */

#ifndef _SERVER_AUTO_SHUTDOWN_LOADER_H_
#define _SERVER_AUTO_SHUTDOWN_LOADER_H_

// From SC
void AddSC_ServerAutoShutdown();

// Add all
void Addmod_server_auto_shutdownScripts()
{
    AddSC_ServerAutoShutdown();
}

#endif /* _SERVER_AUTO_SHUTDOWN_LOADER_H_ */
