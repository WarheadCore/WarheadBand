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

#include "Player.h"
#include "ScriptObject.h"
#include "Spell.h"
#include "Vip.h"

class Vip_Player : public PlayerScript
{
public:
    Vip_Player() : PlayerScript("Vip_Player") { }

    void OnGiveXP(Player* player, uint32& amount, Unit* /*victim*/) override
    {
        if (!sVip->IsEnable() || !sVip->IsVip(player))
            return;

        amount = static_cast<uint32>(float(amount) * sVip->GetRateForPlayer(player, VipRateType::XP));
    }

    void OnGiveHonorPoints(Player* player, float& points, Unit* /*victim*/) override
    {
        if (!sVip->IsEnable() || !sVip->IsVip(player))
            return;

        points *= sVip->GetRateForPlayer(player, VipRateType::Honor);
    }

    bool OnReputationChange(Player* player, uint32 /*factionID*/, int32& standing, bool /*incremental*/) override
    {
        if (!sVip->IsEnable() || !sVip->IsVip(player))
            return true;

        standing = static_cast<int32>(float(standing) * sVip->GetRateForPlayer(player, VipRateType::Reputation));
        return true;
    }

    void OnUpdateProfessionSkill(Player* player, uint16 /*skillId*/, int32 /*chance*/, uint32& step) override
    {
        if (!sVip->IsEnable() || !sVip->IsVip(player))
            return;

        step = static_cast<uint32>(float(step) * sVip->GetRateForPlayer(player, VipRateType::Profession));
    }

    void OnLogin(Player* player) override
    {
        sVip->OnLoginPlayer(player);
    }

    void OnLogout(Player* player) override
    {
        sVip->OnLogoutPlayer(player);
    }

    void OnAfterResurrect(Player* player, float /* restore_percent */, bool applySickness) override
    {
        if (!sVip->IsEnable() || !sVip->IsVip(player) || !applySickness)
            return;

        if (player->HasAura(SPELL_RESSURECTION_SICKESS))
            player->RemoveAura(SPELL_RESSURECTION_SICKESS);
    }

    void OnSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (!sVip->IsEnable() || !player)
            return;

        if (spell->GetSpellInfo()->Id == SPELL_HEARTHSTONE)
            sVip->RemoveCooldown(player, SPELL_HEARTHSTONE);
    }

private:
    enum VipSpells
    {
        SPELL_RESSURECTION_SICKESS = 15007,
        SPELL_HEARTHSTONE = 8690
    };
};

class Vip_World : public WorldScript
{
public:
    Vip_World() : WorldScript("Vip_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sVip->LoadConfig();
    }

    void OnStartup() override
    {
        sVip->InitSystem();
    }

    void OnUpdate(uint32 diff) override
    {
        sVip->Update(diff);
    }
};

class Vip_AllCreature : public AllCreatureScript
{
public:
    Vip_AllCreature() : AllCreatureScript("Vip_AllCreature") { }

    bool CanCreatureSendListInventory(Player* player, Creature* creature, uint32 /*vendorEntry*/) override
    {
        return sVip->CanUsingVendor(player, creature);
    }
};

// Group all custom scripts
void AddSC_Vip()
{
    new Vip_World();
    new Vip_Player();
    new Vip_AllCreature();
}
