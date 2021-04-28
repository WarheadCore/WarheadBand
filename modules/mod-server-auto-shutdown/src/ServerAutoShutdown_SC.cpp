/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 * Copyright (C) 2021+ WarheadCore <https://github.com/WarheadCore>
 */

#include "ServerAutoShutdown.h"
#include "Config.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "TaskScheduler.h"

class ServerAutoShutdown_World : public WorldScript
{
public:
    ServerAutoShutdown_World() : WorldScript("ServerAutoShutdown_World") { }

    void OnUpdate(uint32 diff) override
    {
        sSAS->OnUpdate(diff);
    }

    void OnAfterConfigLoad(bool reload) override
    {
        if (reload)
            sSAS->Init();
    }

    void OnStartup() override
    {
        sSAS->Init();
    }
};

// Group all custom scripts
void AddSC_ServerAutoShutdown()
{
    new ServerAutoShutdown_World();
}
