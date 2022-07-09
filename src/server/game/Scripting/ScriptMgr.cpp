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

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "ScriptSystem.h"
#include "SmartAI.h"
#include "SpellMgr.h"
#include "UnitAI.h"
#include "ScriptRegistryMgr.h"
#include "ScriptObject.h"

struct TSpellSummary
{
    uint8 Targets; // set of enum SelectTarget
    uint8 Effects; // set of enum SelectEffect
}*SpellSummary;

void ScriptMgr::Initialize()
{
    LOG_INFO("server.loading", "> Loading C++ scripts");
    LOG_INFO("server.loading", "");

    AddSC_SmartScripts();

    ASSERT(_script_loader_callback,
        "Script loader callback wasn't registered!");

    ASSERT(_modules_loader_callback,
        "Modules loader callback wasn't registered!");

    _script_loader_callback();
    _modules_loader_callback();
}

void ScriptMgr::Unload()
{
    sScriptRegistryMgr(AccountScript)->ClearPoinerList();
    sScriptRegistryMgr(AchievementCriteriaScript)->ClearPoinerList();
    sScriptRegistryMgr(AchievementScript)->ClearPoinerList();
    sScriptRegistryMgr(AllCreatureScript)->ClearPoinerList();
    sScriptRegistryMgr(AllGameObjectScript)->ClearPoinerList();
    sScriptRegistryMgr(AllItemScript)->ClearPoinerList();
    sScriptRegistryMgr(AllMapScript)->ClearPoinerList();
    sScriptRegistryMgr(AreaTriggerScript)->ClearPoinerList();
    sScriptRegistryMgr(ArenaScript)->ClearPoinerList();
    sScriptRegistryMgr(ArenaTeamScript)->ClearPoinerList();
    sScriptRegistryMgr(AuctionHouseScript)->ClearPoinerList();
    sScriptRegistryMgr(BGScript)->ClearPoinerList();
    sScriptRegistryMgr(BattlegroundMapScript)->ClearPoinerList();
    sScriptRegistryMgr(BattlegroundScript)->ClearPoinerList();
    sScriptRegistryMgr(CommandSC)->ClearPoinerList();
    sScriptRegistryMgr(CommandScript)->ClearPoinerList();
    sScriptRegistryMgr(ConditionScript)->ClearPoinerList();
    sScriptRegistryMgr(CreatureScript)->ClearPoinerList();
    sScriptRegistryMgr(DatabaseScript)->ClearPoinerList();
    sScriptRegistryMgr(DynamicObjectScript)->ClearPoinerList();
    sScriptRegistryMgr(ElunaScript)->ClearPoinerList();
    sScriptRegistryMgr(FormulaScript)->ClearPoinerList();
    sScriptRegistryMgr(GameEventScript)->ClearPoinerList();
    sScriptRegistryMgr(GameObjectScript)->ClearPoinerList();
    sScriptRegistryMgr(GlobalScript)->ClearPoinerList();
    sScriptRegistryMgr(GroupScript)->ClearPoinerList();
    sScriptRegistryMgr(GuildScript)->ClearPoinerList();
    sScriptRegistryMgr(InstanceMapScript)->ClearPoinerList();
    sScriptRegistryMgr(ItemScript)->ClearPoinerList();
    sScriptRegistryMgr(LootScript)->ClearPoinerList();
    sScriptRegistryMgr(MailScript)->ClearPoinerList();
    sScriptRegistryMgr(MiscScript)->ClearPoinerList();
    sScriptRegistryMgr(ModuleScript)->ClearPoinerList();
    sScriptRegistryMgr(MovementHandlerScript)->ClearPoinerList();
    sScriptRegistryMgr(OutdoorPvPScript)->ClearPoinerList();
    sScriptRegistryMgr(PetScript)->ClearPoinerList();
    sScriptRegistryMgr(PlayerScript)->ClearPoinerList();
    sScriptRegistryMgr(ServerScript)->ClearPoinerList();
    sScriptRegistryMgr(SpellSC)->ClearPoinerList();
    sScriptRegistryMgr(SpellScriptLoader)->ClearPoinerList();
    sScriptRegistryMgr(TransportScript)->ClearPoinerList();
    sScriptRegistryMgr(UnitScript)->ClearPoinerList();
    sScriptRegistryMgr(VehicleScript)->ClearPoinerList();
    sScriptRegistryMgr(WeatherScript)->ClearPoinerList();
    sScriptRegistryMgr(WorldMapScript)->ClearPoinerList();
    sScriptRegistryMgr(WorldObjectScript)->ClearPoinerList();
    sScriptRegistryMgr(WorldScript)->ClearPoinerList();

    delete[] SpellSummary;
}

void ScriptMgr::LoadDatabase()
{
    uint32 oldMSTime = getMSTime();

    sScriptSystemMgr->LoadScriptWaypoints();

    // Add all scripts that must be loaded after db/maps
    sScriptRegistryMgr(WorldMapScript)->AddAfterLoadDBScripts();
    sScriptRegistryMgr(BattlegroundMapScript)->AddAfterLoadDBScripts();
    sScriptRegistryMgr(InstanceMapScript)->AddAfterLoadDBScripts();
    sScriptRegistryMgr(SpellScriptLoader)->AddAfterLoadDBScripts();
    sScriptRegistryMgr(ItemScript)->AddAfterLoadDBScripts();
    sScriptRegistryMgr(CreatureScript)->AddAfterLoadDBScripts();
    sScriptRegistryMgr(GameObjectScript)->AddAfterLoadDBScripts();
    sScriptRegistryMgr(AreaTriggerScript)->AddAfterLoadDBScripts();
    sScriptRegistryMgr(BattlegroundScript)->AddAfterLoadDBScripts();
    sScriptRegistryMgr(OutdoorPvPScript)->AddAfterLoadDBScripts();
    sScriptRegistryMgr(WeatherScript)->AddAfterLoadDBScripts();
    sScriptRegistryMgr(ConditionScript)->AddAfterLoadDBScripts();
    sScriptRegistryMgr(TransportScript)->AddAfterLoadDBScripts();
    sScriptRegistryMgr(AchievementCriteriaScript)->AddAfterLoadDBScripts();

    FillSpellSummary();

    CheckIfScriptsInDatabaseExist();

    LOG_INFO("server", ">> Loaded {} C++ scripts in {} ms", GetScriptCount(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server", " ");

    ASSERT(_script_loader_callback,
        "Script loader callback wasn't registered!");

    ASSERT(_modules_loader_callback,
        "Modules loader callback wasn't registered!");
}

void ScriptMgr::CheckIfScriptsInDatabaseExist()
{
    for (auto const& scriptName : sObjectMgr->GetScriptNames())
    {
        if (uint32 sid = sObjectMgr->GetScriptId(scriptName.c_str()))
        {
            if (!sScriptRegistryMgr(SpellScriptLoader)->GetScriptById(sid) &&
                !sScriptRegistryMgr(ServerScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(WorldScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(FormulaScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(WorldMapScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(InstanceMapScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(BattlegroundMapScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(ItemScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(CreatureScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(GameObjectScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(AreaTriggerScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(BattlegroundScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(OutdoorPvPScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(CommandScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(WeatherScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(AuctionHouseScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(ConditionScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(VehicleScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(DynamicObjectScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(TransportScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(AchievementCriteriaScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(PlayerScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(GuildScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(BGScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(AchievementScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(ArenaTeamScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(SpellSC)->GetScriptById(sid) &&
                !sScriptRegistryMgr(MiscScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(PetScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(CommandSC)->GetScriptById(sid) &&
                !sScriptRegistryMgr(ArenaScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(GroupScript)->GetScriptById(sid) &&
                !sScriptRegistryMgr(DatabaseScript)->GetScriptById(sid))
                {
                    LOG_ERROR("sql.sql", "Script named '{}' is assigned in the database, but has no code!", scriptName);
                }
        }
    }
}

void ScriptMgr::FillSpellSummary()
{
    UnitAI::FillAISpellInfo();

    SpellSummary = new TSpellSummary[sSpellMgr->GetSpellInfoStoreSize()];

    SpellInfo const* pTempSpell;

    for (uint32 i = 0; i < sSpellMgr->GetSpellInfoStoreSize(); ++i)
    {
        SpellSummary[i].Effects = 0;
        SpellSummary[i].Targets = 0;

        pTempSpell = sSpellMgr->GetSpellInfo(i);
        // This spell doesn't exist.
        if (!pTempSpell)
            continue;

        for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            // Spell targets self.
            if (pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_CASTER)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_SELF - 1);

            // Spell targets a single enemy.
            if (pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_TARGET_ENEMY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_DEST_TARGET_ENEMY)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_SINGLE_ENEMY - 1);

            // Spell targets AoE at enemy.
            if (pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_SRC_AREA_ENEMY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_DEST_AREA_ENEMY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_SRC_CASTER ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_DEST_DYNOBJ_ENEMY)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_AOE_ENEMY - 1);

            // Spell targets an enemy.
            if (pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_TARGET_ENEMY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_DEST_TARGET_ENEMY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_SRC_AREA_ENEMY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_DEST_AREA_ENEMY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_SRC_CASTER ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_DEST_DYNOBJ_ENEMY)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_ANY_ENEMY - 1);

            // Spell targets a single friend (or self).
            if (pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_CASTER ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_TARGET_ALLY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_TARGET_PARTY)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_SINGLE_FRIEND - 1);

            // Spell targets AoE friends.
            if (pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_CASTER_AREA_PARTY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_LASTTARGET_AREA_PARTY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_SRC_CASTER)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_AOE_FRIEND - 1);

            // Spell targets any friend (or self).
            if (pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_CASTER ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_TARGET_ALLY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_TARGET_PARTY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_CASTER_AREA_PARTY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_UNIT_LASTTARGET_AREA_PARTY ||
                    pTempSpell->Effects[j].TargetA.GetTarget() == TARGET_SRC_CASTER)
                SpellSummary[i].Targets |= 1 << (SELECT_TARGET_ANY_FRIEND - 1);

            // Make sure that this spell includes a damage effect.
            if (pTempSpell->Effects[j].Effect == SPELL_EFFECT_SCHOOL_DAMAGE ||
                    pTempSpell->Effects[j].Effect == SPELL_EFFECT_INSTAKILL ||
                    pTempSpell->Effects[j].Effect == SPELL_EFFECT_ENVIRONMENTAL_DAMAGE ||
                    pTempSpell->Effects[j].Effect == SPELL_EFFECT_HEALTH_LEECH)
                SpellSummary[i].Effects |= 1 << (SELECT_EFFECT_DAMAGE - 1);

            // Make sure that this spell includes a healing effect (or an apply aura with a periodic heal).
            if (pTempSpell->Effects[j].Effect == SPELL_EFFECT_HEAL ||
                    pTempSpell->Effects[j].Effect == SPELL_EFFECT_HEAL_MAX_HEALTH ||
                    pTempSpell->Effects[j].Effect == SPELL_EFFECT_HEAL_MECHANICAL ||
                    (pTempSpell->Effects[j].Effect == SPELL_EFFECT_APPLY_AURA  && pTempSpell->Effects[j].ApplyAuraName == 8))
                SpellSummary[i].Effects |= 1 << (SELECT_EFFECT_HEALING - 1);

            // Make sure that this spell applies an aura.
            if (pTempSpell->Effects[j].Effect == SPELL_EFFECT_APPLY_AURA)
                SpellSummary[i].Effects |= 1 << (SELECT_EFFECT_AURA - 1);
        }
    }
}

///-
AllMapScript::AllMapScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(AllMapScript)->AddScript(this);
}

AllCreatureScript::AllCreatureScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(AllCreatureScript)->AddScript(this);
}

UnitScript::UnitScript(std::string_view name, bool addToScripts)
    : ScriptObject(name)
{
    if (addToScripts)
        sScriptRegistryMgr(UnitScript)->AddScript(this);
}

MovementHandlerScript::MovementHandlerScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(MovementHandlerScript)->AddScript(this);
}

SpellScriptLoader::SpellScriptLoader(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(SpellScriptLoader)->AddScript(this);
}

ServerScript::ServerScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(ServerScript)->AddScript(this);
}

WorldScript::WorldScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(WorldScript)->AddScript(this);
}

FormulaScript::FormulaScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(FormulaScript)->AddScript(this);
}

WorldMapScript::WorldMapScript(std::string_view name, uint32 mapId)
    : ScriptObject(name), MapScript<Map>(mapId)
{
    sScriptRegistryMgr(WorldMapScript)->AddScript(this);
}

InstanceMapScript::InstanceMapScript(std::string_view name, uint32 mapId)
    : ScriptObject(name), MapScript<InstanceMap>(mapId)
{
    sScriptRegistryMgr(InstanceMapScript)->AddScript(this);
}

BattlegroundMapScript::BattlegroundMapScript(std::string_view name, uint32 mapId)
    : ScriptObject(name), MapScript<BattlegroundMap>(mapId)
{
    sScriptRegistryMgr(BattlegroundMapScript)->AddScript(this);
}

ItemScript::ItemScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(ItemScript)->AddScript(this);
}

CreatureScript::CreatureScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(CreatureScript)->AddScript(this);
}

GameObjectScript::GameObjectScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(GameObjectScript)->AddScript(this);
}

AreaTriggerScript::AreaTriggerScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(AreaTriggerScript)->AddScript(this);
}

bool OnlyOnceAreaTriggerScript::OnTrigger(Player* player, AreaTrigger const* trigger)
{
    uint32 const triggerId = trigger->entry;

    if (InstanceScript* instance = player->GetInstanceScript())
    {
        if (instance->IsAreaTriggerDone(triggerId))
        {
            return true;
        }
        else
        {
            instance->MarkAreaTriggerDone(triggerId);
        }
    }

    return _OnTrigger(player, trigger);
}

void OnlyOnceAreaTriggerScript::ResetAreaTriggerDone(InstanceScript* script, uint32 triggerId)
{
    script->ResetAreaTriggerDone(triggerId);
}

void OnlyOnceAreaTriggerScript::ResetAreaTriggerDone(Player const* player, AreaTrigger const* trigger)
{
    if (InstanceScript* instance = player->GetInstanceScript())
    {
        ResetAreaTriggerDone(instance, trigger->entry);
    }
}

BattlegroundScript::BattlegroundScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(BattlegroundScript)->AddScript(this);
}

OutdoorPvPScript::OutdoorPvPScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(OutdoorPvPScript)->AddScript(this);
}

CommandScript::CommandScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(CommandScript)->AddScript(this);
}

WeatherScript::WeatherScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(WeatherScript)->AddScript(this);
}

AuctionHouseScript::AuctionHouseScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(AuctionHouseScript)->AddScript(this);
}

ConditionScript::ConditionScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(ConditionScript)->AddScript(this);
}

VehicleScript::VehicleScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(VehicleScript)->AddScript(this);
}

DynamicObjectScript::DynamicObjectScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(DynamicObjectScript)->AddScript(this);
}

TransportScript::TransportScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(TransportScript)->AddScript(this);
}

AchievementCriteriaScript::AchievementCriteriaScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(AchievementCriteriaScript)->AddScript(this);
}

PlayerScript::PlayerScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(PlayerScript)->AddScript(this);
}

AccountScript::AccountScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(AccountScript)->AddScript(this);
}

GuildScript::GuildScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(GuildScript)->AddScript(this);
}

GroupScript::GroupScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(GroupScript)->AddScript(this);
}

GlobalScript::GlobalScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(GlobalScript)->AddScript(this);
}

BGScript::BGScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(BGScript)->AddScript(this);
}

ArenaTeamScript::ArenaTeamScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(ArenaTeamScript)->AddScript(this);
}

SpellSC::SpellSC(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(SpellSC)->AddScript(this);
}

ModuleScript::ModuleScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(ModuleScript)->AddScript(this);
}

GameEventScript::GameEventScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(GameEventScript)->AddScript(this);
}

MailScript::MailScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(MailScript)->AddScript(this);
}

AchievementScript::AchievementScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(AchievementScript)->AddScript(this);
}

PetScript::PetScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(PetScript)->AddScript(this);
}

ArenaScript::ArenaScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(ArenaScript)->AddScript(this);
}

MiscScript::MiscScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(MiscScript)->AddScript(this);
}

CommandSC::CommandSC(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(CommandSC)->AddScript(this);
}

DatabaseScript::DatabaseScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(DatabaseScript)->AddScript(this);
}

WorldObjectScript::WorldObjectScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(WorldObjectScript)->AddScript(this);
}

LootScript::LootScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(LootScript)->AddScript(this);
}

ElunaScript::ElunaScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(ElunaScript)->AddScript(this);
}

AllItemScript::AllItemScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(AllItemScript)->AddScript(this);
}

AllGameObjectScript::AllGameObjectScript(std::string_view name)
    : ScriptObject(name)
{
    sScriptRegistryMgr(AllGameObjectScript)->AddScript(this);
}
