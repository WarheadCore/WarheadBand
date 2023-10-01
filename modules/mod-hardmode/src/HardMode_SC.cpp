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

#include "HardMode.h"
#include "ScriptObject.h"

class HardMode_Player : public PlayerScript
{
public:
    HardMode_Player() : PlayerScript("HardMode_Player") { }

    bool CanResurrect(Player* /*player*/, float /*restorePercent*/, bool /*applySickness*/) override
    {
        return sHardModeMgr->CanResurrect();
    }
};

class HardMode_World : public WorldScript
{
public:
    HardMode_World() : WorldScript("HardMode_World") { }

    void OnAfterConfigLoad(bool reload) override
    {
        sHardModeMgr->LoadConfig(reload);
    }

    void OnStartup() override
    {
        sHardModeMgr->Initialize();
    }
};

// Group all custom scripts
void AddSC_HardMode()
{
    new HardMode_Player();
    new HardMode_World();
}