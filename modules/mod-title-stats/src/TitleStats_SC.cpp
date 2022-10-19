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

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "TitleStats.h"
#include "ScriptObject.h"

class TitleStats_Player : public PlayerScript
{
public:
    TitleStats_Player() : PlayerScript("TitleStats_Player") { }

    void OnChangeTitle(Player* player, int32 title) override
    {
        sTitleStatsMgr->OnChangeTitle(player, title);
    }

    void OnLogin(Player* player) override
    {
        sTitleStatsMgr->OnLogin(player);
    }
};

class TitleStats_World : public WorldScript
{
public:
    TitleStats_World() : WorldScript("TitleStats_World") { }

    void OnAfterConfigLoad(bool reload) override
    {
        sTitleStatsMgr->LoadConfig(reload);
    }

    void OnStartup() override
    {
        sTitleStatsMgr->Initialize();
    }
};

// Group all custom scripts
void AddSC_TitleStats()
{
    new TitleStats_Player();
    new TitleStats_World();
}
