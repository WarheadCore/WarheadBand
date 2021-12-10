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

#include "Transmogrification.h"
#include "ScriptMgr.h"
#include "ModulesConfig.h"

class Transmogrification_NPC : public CreatureScript
{
public:
    Transmogrification_NPC() : CreatureScript("Transmogrification_NPC") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        sTransmog->OnGossipHello(player, creature);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        sTransmog->OnGossipSelect(player, creature, action, sender);
        return true;
    }

    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code) override
    {
        sTransmog->OnGossipSelectCode(player, creature, action, sender, code);
        return true;
    }
};

class Transmogrification_Player : public PlayerScript
{
public:
    Transmogrification_Player() : PlayerScript("Player_Transmogrify") { }

    void OnAfterSetVisibleItemSlot(Player* player, uint8 slot, Item* item) override
    {
        sTransmog->SetVisibleItemSlot(player, slot, item);
    }

    void OnAfterMoveItemFromInventory(Player* /*player*/, Item* item, uint8 /*bag*/, uint8 /*slot*/, bool /*update*/) override
    {
        sTransmog->DeleteFakeFromDB(item->GetGUID().GetCounter());
    }

    void OnLogin(Player* player) override
    {
        sTransmog->LoadPlayerAtLogin(player);
    }

    void OnLogout(Player* player) override
    {
        sTransmog->ClearPlayerAtLogout(player);
    }
};

class Transmogrification_World : public WorldScript
{
public:
    Transmogrification_World() : WorldScript("Transmogrification_World") { }

    void OnAfterConfigLoad(bool reload) override
    {
        // Add bool
        sModulesConfig->AddOption<bool>("Transmogrification.EnableSetInfo");
        sModulesConfig->AddOption<bool>("Transmogrification.EnableSets", true);
        sModulesConfig->AddOption<bool>("Transmogrification.EnableTransmogInfo", true);
        sModulesConfig->AddOption<bool>("Transmogrification.RequireToken");
        sModulesConfig->AddOption<bool>("Transmogrification.AllowPoor");
        sModulesConfig->AddOption<bool>("Transmogrification.AllowCommon");
        sModulesConfig->AddOption<bool>("Transmogrification.AllowUncommon", true);
        sModulesConfig->AddOption<bool>("Transmogrification.AllowRare", true);
        sModulesConfig->AddOption<bool>("Transmogrification.AllowEpic", true);
        sModulesConfig->AddOption<bool>("Transmogrification.AllowLegendary");
        sModulesConfig->AddOption<bool>("Transmogrification.AllowArtifact");
        sModulesConfig->AddOption<bool>("Transmogrification.AllowHeirloom", true);
        sModulesConfig->AddOption<bool>("Transmogrification.AllowMixedArmorTypes");
        sModulesConfig->AddOption<bool>("Transmogrification.AllowMixedWeaponTypes");
        sModulesConfig->AddOption<bool>("Transmogrification.AllowFishingPoles");
        sModulesConfig->AddOption<bool>("Transmogrification.IgnoreReqRace");
        sModulesConfig->AddOption<bool>("Transmogrification.IgnoreReqClass");
        sModulesConfig->AddOption<bool>("Transmogrification.IgnoreReqSkill");
        sModulesConfig->AddOption<bool>("Transmogrification.IgnoreReqSpell");
        sModulesConfig->AddOption<bool>("Transmogrification.IgnoreReqLevel");
        sModulesConfig->AddOption<bool>("Transmogrification.IgnoreReqEvent");
        sModulesConfig->AddOption<bool>("Transmogrification.IgnoreReqStats");

        // Add int
        sModulesConfig->AddOption<int32>("Transmogrification.SetNpcText", 601084);
        sModulesConfig->AddOption<int32>("Transmogrification.MaxSets", 10);
        sModulesConfig->AddOption<int32>("Transmogrification.SetCopperCost");
        sModulesConfig->AddOption<int32>("Transmogrification.TransmogNpcText", 601083);
        sModulesConfig->AddOption<int32>("Transmogrification.CopperCost");
        sModulesConfig->AddOption<int32>("Transmogrification.TokenEntry", 49426);
        sModulesConfig->AddOption<int32>("Transmogrification.TokenAmount", 1);

        // Add float
        sModulesConfig->AddOption<float>("Transmogrification.SetCostModifier", 3.0f);
        sModulesConfig->AddOption<float>("Transmogrification.ScaledCostModifier");

        // Add string
        sModulesConfig->AddOption<std::string>("Transmogrification.Allowed");
        sModulesConfig->AddOption<std::string>("Transmogrification.NotAllowed");

        sTransmog->LoadConfig(reload);
    }

    void OnStartup() override
    {
        sTransmog->Init();
    }
};

class Transmogrification_Global : public GlobalScript
{
public:
    Transmogrification_Global() : GlobalScript("Transmogrification_Global") { }

    void OnItemDelFromDB(CharacterDatabaseTransaction trans, uint32 itemGuid) override
    {
        sTransmog->DeleteFakeFromDB(itemGuid, &trans);
    }

    void OnMirrorImageDisplayItem(const Item* item, uint32& display) override
    {
        sTransmog->MirrorImageDisplayItem(item, display);
    }
};

// Group all custom scripts
void AddSC_Transmogrification()
{
    new Transmogrification_Global();
    new Transmogrification_NPC();
    new Transmogrification_Player();
    new Transmogrification_World();
}
