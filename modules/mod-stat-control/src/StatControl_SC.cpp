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

#include "ScriptObject.h"
#include "StatControl.h"

class StatControl_Player : public PlayerScript
{
public:
    StatControl_Player() : PlayerScript("StatControl_Player") { }

    void OnGetDodgeFromAgility(Player* /*player*/, float& diminishing, float& /*nondiminishing*/) override
    {
        sStatControlMgr->CorrectStat(diminishing, StatControlType::DoodgeFromAgility);
    }

    void OnGetArmorFromAgility(Player* /*player*/, float& value) override
    {
        sStatControlMgr->CorrectStat(value, StatControlType::ArmorFromAgility);
    }

    void OnGetMeleeCritFromAgility(Player* /*player*/, float& value) override
    {
        sStatControlMgr->CorrectStat(value, StatControlType::CritFromAgility);
    }

    void OnGetSpellCritFromIntellect(Player* /*player*/, float& value) override
    {
        sStatControlMgr->CorrectStat(value, StatControlType::CritFromIntellect);
    }

    void OnGetManaBonusFromIntellect(Player* /*player*/, float& value) override
    {
        sStatControlMgr->CorrectStat(value, StatControlType::ManaFromIntellect);
    }

    void OnGetShieldBlockValue(Player* /*player*/, float& value) override
    {
        sStatControlMgr->CorrectStat(value, StatControlType::BlockFromStrenght);
    }

    void OnUpdateAttackPowerAndDamage(Player* /*player*/, float& value) override
    {
        sStatControlMgr->CorrectStat(value, StatControlType::AttackPowerFromAgility);
    }
};

class StatControl_World : public WorldScript
{
public:
    StatControl_World() : WorldScript("StatControl_World") { }

    void OnAfterConfigLoad(bool reload) override
    {
        sStatControlMgr->LoadConfig(reload);
    }

    void OnStartup() override
    {
        sStatControlMgr->Initialize();
    }
};

// Group all custom scripts
void AddSC_StatControl()
{
    new StatControl_Player();
    new StatControl_World();
}
