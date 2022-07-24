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

#include "GameLocale.h"
#include "Log.h"
#include "ModuleLocale.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "ScriptObject.h"
#include "StringFormat.h"
#include "GameLocale.h"

class Boss_Announcer_Player : public PlayerScript
{
public:
    Boss_Announcer_Player() : PlayerScript("Boss_Announcer_Player") {}

    void OnCreatureKill(Player* player, Creature* creature) override
    {
        if (!MOD_CONF_GET_BOOL("BossAnnouncer.Enable"))
            return;

        if (creature->GetMaxHealth() <= 1000000)
            return;

        if (!creature->isWorldBoss())
            return;

        auto creatureName = sGameLocale->GetCreatureNamelocale(creature->GetEntry(), player->GetSession()->GetSessionDbcLocale());

        if (IsKilledRaidBoss(player))
            sModuleLocale->SendGlobalMessage(false, "BOSS_AN_LOCALE_SEND_TEXT_RAID", player->GetName(), creatureName, GetDiffString(player));
        else
            sModuleLocale->SendGlobalMessage(false, "BOSS_AN_LOCALE_SEND_TEXT_WORLD", player->GetName(), creatureName);
    };

private:
    std::string const GetDiffString(Player* player)
    {
        if (!player)
            return "";

        auto map = player->GetMap();
        if (!map || !map->IsRaid())
            return "";

        uint8 localeIndex = static_cast<uint8>(player->GetSession()->GetSessionDbLocaleIndex());

        switch (map->GetDifficulty())
        {
            case RAID_DIFFICULTY_10MAN_NORMAL:
                return *sModuleLocale->GetModuleString("BOSS_AN_LOCALE_10_MAN_NORMAL", localeIndex);
            case RAID_DIFFICULTY_10MAN_HEROIC:
                return *sModuleLocale->GetModuleString("BOSS_AN_LOCALE_10_MAN_HEROIC", localeIndex);
            case RAID_DIFFICULTY_25MAN_NORMAL:
                return *sModuleLocale->GetModuleString("BOSS_AN_LOCALE_25_MAN_NORMAL", localeIndex);
            case RAID_DIFFICULTY_25MAN_HEROIC:
                return *sModuleLocale->GetModuleString("BOSS_AN_LOCALE_25_MAN_HEROIC", localeIndex);
            default:
                return "";
        }
    }

    bool IsKilledRaidBoss(Player* player)
    {
        auto map = player->GetMap();
        if (!map || !map->IsRaid())
            return false;

        return true;
    }
};

// Group all custom scripts
void AddSC_BossAnnouncer()
{
    new Boss_Announcer_Player();
}
