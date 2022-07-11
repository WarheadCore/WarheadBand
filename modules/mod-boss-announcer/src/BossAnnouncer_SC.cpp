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

#include "GameLocale.h"
#include "Log.h"
#include "ModuleLocale.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "ScriptObject.h"
#include "StringFormat.h"

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

        if (IsKilledRaidBoss(player))
            sModuleLocale->SendGlobalMessage(false, "BOSS_AN_LOCALE_SEND_TEXT_RAID", player->GetName().c_str(), GetCreatureName(player, creature).c_str(), GetDiffString(player).c_str());
        else
            sModuleLocale->SendGlobalMessage(false, "BOSS_AN_LOCALE_SEND_TEXT_WORLD", player->GetName().c_str(), GetCreatureName(player, creature).c_str());
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

    std::string const GetCreatureName(Player* player, Creature* creature)
    {
        auto creatureTemplate = sObjectMgr->GetCreatureTemplate(creature->GetEntry());
        auto cretureLocale = sGameLocale->GetCreatureLocale(creature->GetEntry());
        std::string name;

        if (cretureLocale)
            name = cretureLocale->Name[player->GetSession()->GetSessionDbcLocale()].c_str();

        if (name.empty() && creatureTemplate)
            name = creatureTemplate->Name.c_str();

        if (name.empty())
            name = "Unknown creature";

        return name;
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
