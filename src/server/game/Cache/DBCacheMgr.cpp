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

#include "DBCacheMgr.h"
#include "DatabaseEnv.h"
#include "Util.h"
#include "StopWatch.h"
#include "GameConfig.h"
#include "Log.h"

/*static*/ DBCacheMgr* DBCacheMgr::instance()
{
    static DBCacheMgr instance;
    return &instance;
}

void DBCacheMgr::Initialize()
{
    StopWatch sw;

    LOG_INFO("server.loading", "Initialize database cache");

    _isEnableAsyncLoad = CONF_GET_BOOL("DBCache.AsyncLoad.Enable");
    _isEnableWaitAtAdd = CONF_GET_BOOL("DBCache.WaitAtAdd.Enable");

    InitializeDefines();
    InitializeQuery();

    LOG_INFO("server.loading", ">> Initialized database cache in {}", sw);
    LOG_INFO("server.loading", "");
}

void DBCacheMgr::AddQuery(DBCacheTable index)
{
    if (!_isEnableAsyncLoad)
        return;

    if (_queryList.contains(index))
    {
        LOG_ERROR("db.async", "Query with index {} exist!", AsUnderlyingType(index));
        return;
    }

    auto sql{ GetStringQuery(index) };
    if (sql.empty())
    {
        LOG_ERROR("db.async", "Empty string for index {}", AsUnderlyingType(index));
        return;
    }

    auto task = new BasicStatementTask(sql, true);
    auto result = task->GetFuture();
    WorldDatabase.Enqueue(task);

    if (_isEnableWaitAtAdd)
        result.wait();

    _queryList.emplace(index, std::move(result));
}

QueryResult DBCacheMgr::GetResult(DBCacheTable index)
{
    if (!_isEnableAsyncLoad)
    {
        auto sql{ GetStringQuery(index) };
        if (sql.empty())
        {
            LOG_ERROR("db.async", "Empty string for index {}", AsUnderlyingType(index));
            return nullptr;
        }

        return WorldDatabase.Query(sql);
    }

    auto const& itr = _queryList.find(index);
    if (itr == _queryList.end())
    {
        LOG_ERROR("db.async", "Not found query with index {}", AsUnderlyingType(index));

        auto sql{ GetStringQuery(index) };
        if (sql.empty())
        {
            LOG_ERROR("db.async", "Empty string for index {}", AsUnderlyingType(index));
            return nullptr;
        }

        return WorldDatabase.Query(sql);
    }

    itr->second.wait();
    auto result{ itr->second.get() };
    _queryList.erase(index);
    return result;
}

std::string_view DBCacheMgr::GetStringQuery(DBCacheTable index)
{
    auto const& itr = _queryStrings.find(index);
    if (itr == _queryStrings.end())
        return {};

    return itr->second;
}

void DBCacheMgr::InitializeQuery()
{
    AddQuery(DBCacheTable::GameEventMaxEvent);
    AddQuery(DBCacheTable::WarheadStrings);
    AddQuery(DBCacheTable::GameGraveyard);
    AddQuery(DBCacheTable::SpellRanks);
    AddQuery(DBCacheTable::SpellCustomAttr);
    AddQuery(DBCacheTable::ScriptNames);
    AddQuery(DBCacheTable::InstanceTemplate);
    AddQuery(DBCacheTable::BroadcastTexts);
    AddQuery(DBCacheTable::BroadcastTextLocales);
    AddQuery(DBCacheTable::AchievementRewardLocales);
    AddQuery(DBCacheTable::CreatureLocales);
    AddQuery(DBCacheTable::GameObjectLocales);
    AddQuery(DBCacheTable::ItemLocales);
    AddQuery(DBCacheTable::ItemSetNameLocales);
    AddQuery(DBCacheTable::NpcTextLocales);
    AddQuery(DBCacheTable::PageTextLocales);
    AddQuery(DBCacheTable::GossipMenuItemsLocales);
    AddQuery(DBCacheTable::PointOfInterestLocales);
    AddQuery(DBCacheTable::QuestLocales);
    AddQuery(DBCacheTable::QuestOfferRewardLocale);
    AddQuery(DBCacheTable::QuestRequestItemsLocale);
    AddQuery(DBCacheTable::QuestGreetingLocales);
    AddQuery(DBCacheTable::ChatCommandsLocales);
    AddQuery(DBCacheTable::RaceStrings);
    AddQuery(DBCacheTable::ClassStrings);
    AddQuery(DBCacheTable::CommonStrings);
    AddQuery(DBCacheTable::PageText);
    AddQuery(DBCacheTable::GameobjectTemplate);
    AddQuery(DBCacheTable::GameobjectTemplateAddon);
    AddQuery(DBCacheTable::TransportTemplates);
    AddQuery(DBCacheTable::SpellRequired);
    AddQuery(DBCacheTable::SpellGroup);
    AddQuery(DBCacheTable::SpellProcEvent);
    AddQuery(DBCacheTable::SpellProc);
    AddQuery(DBCacheTable::SpellBonusData);
    AddQuery(DBCacheTable::SpellThreat);
    AddQuery(DBCacheTable::SpellMixology);
    AddQuery(DBCacheTable::SpellGroupStackRules);
    AddQuery(DBCacheTable::NpcText);
    AddQuery(DBCacheTable::SpellEnchantProcData);
    AddQuery(DBCacheTable::ItemEnchantmentTemplate);
    AddQuery(DBCacheTable::Disables);
    AddQuery(DBCacheTable::ItemTemplate);
    AddQuery(DBCacheTable::ItemSetNames);
    AddQuery(DBCacheTable::CreatureModelInfo);
    AddQuery(DBCacheTable::CreatureTemplate);
    AddQuery(DBCacheTable::CreatureEquipTemplate);
    AddQuery(DBCacheTable::CreatureTemplateAddon);
    AddQuery(DBCacheTable::ReputationRewardRate);
    AddQuery(DBCacheTable::CreatureOnkillReputation);
    AddQuery(DBCacheTable::ReputationSpilloverTemplate);
    AddQuery(DBCacheTable::PointsOfInterest);
    AddQuery(DBCacheTable::CreatureClassLevelStats);
    AddQuery(DBCacheTable::Creature);
    AddQuery(DBCacheTable::CreatureSummonGroups);
    AddQuery(DBCacheTable::CreatureAddon);
    AddQuery(DBCacheTable::CreatureMovementOverride);
    AddQuery(DBCacheTable::Gameobject);
    AddQuery(DBCacheTable::GameobjectAddon);
    AddQuery(DBCacheTable::GameObjectQuestItem);
    AddQuery(DBCacheTable::CreatureQuestItem);
    AddQuery(DBCacheTable::LinkedRespawn);
    AddQuery(DBCacheTable::GameWeather);
    AddQuery(DBCacheTable::QuestTemplate);
    AddQuery(DBCacheTable::QuestPOI);
    AddQuery(DBCacheTable::QuestPOIPoints);
    AddQuery(DBCacheTable::QuestGreeting);
    AddQuery(DBCacheTable::QuestMoneyReward);
    AddQuery(DBCacheTable::PoolTemplate);
    AddQuery(DBCacheTable::PoolCreature);
    AddQuery(DBCacheTable::PoolGameobject);
    AddQuery(DBCacheTable::PoolPool);
    AddQuery(DBCacheTable::PoolObjects);
    AddQuery(DBCacheTable::HolidayDates);
    AddQuery(DBCacheTable::GameEvent);
    AddQuery(DBCacheTable::GameEventPrerequisite);
    AddQuery(DBCacheTable::GameEventCreature);
    AddQuery(DBCacheTable::GameEventGameobject);
    AddQuery(DBCacheTable::GameEventModelEquip);
    AddQuery(DBCacheTable::GameEventCreatureQuest);
    AddQuery(DBCacheTable::GameEventGameobjectQuest);
    AddQuery(DBCacheTable::GameEventQuestCondition);
    AddQuery(DBCacheTable::GameEventCondition);
    AddQuery(DBCacheTable::GameEventNpcFlag);
    AddQuery(DBCacheTable::GameEventSeasonalQuestrelation);
    AddQuery(DBCacheTable::GameEventNpcVendor);
    AddQuery(DBCacheTable::GameEventBattlegroundHoliday);
    AddQuery(DBCacheTable::GameEventPool);
    AddQuery(DBCacheTable::NpcSpellClickSpells);
    AddQuery(DBCacheTable::VehicleTemplateAccessory);
    AddQuery(DBCacheTable::VehicleAccessory);
    AddQuery(DBCacheTable::SpellArea);
    AddQuery(DBCacheTable::Areatrigger);
    AddQuery(DBCacheTable::AreatriggerTeleport);
    AddQuery(DBCacheTable::AreatriggerInvolvedrelation);
    AddQuery(DBCacheTable::AreatriggerTavern);
    AddQuery(DBCacheTable::AreatriggerScripts);
    AddQuery(DBCacheTable::LfgDungeonTemplate);
    AddQuery(DBCacheTable::InstanceEncounters);
    AddQuery(DBCacheTable::LfgDungeonRewards);
    AddQuery(DBCacheTable::GraveyardZone);
    AddQuery(DBCacheTable::SpellPetAuras);
    AddQuery(DBCacheTable::SpellTargetPosition);
    AddQuery(DBCacheTable::SpellLinkedSpell);
    AddQuery(DBCacheTable::PlayerCreateInfo);
    AddQuery(DBCacheTable::PlayerCreateInfoItem);
    AddQuery(DBCacheTable::PlayerCreateInfoSkills);
    AddQuery(DBCacheTable::PlayerCreateInfoSpellCustom);
    AddQuery(DBCacheTable::PlayerCreateInfoCastSpell);
    AddQuery(DBCacheTable::PlayerCreateInfoAction);
    AddQuery(DBCacheTable::PlayerClassLevelStats);
    AddQuery(DBCacheTable::PlayerLevelStats);
    AddQuery(DBCacheTable::PlayerXpForLevel);
    AddQuery(DBCacheTable::ExplorationBasexp);
    AddQuery(DBCacheTable::PetNameGeneration);
    AddQuery(DBCacheTable::PetLevelstats);
    AddQuery(DBCacheTable::MailLevelReward);
    AddQuery(DBCacheTable::SkillDiscoveryTemplate);
    AddQuery(DBCacheTable::SkillExtraItemTemplate);
    AddQuery(DBCacheTable::SkillPerfectItemTemplate);
    AddQuery(DBCacheTable::SkillFishingBaseLevel);
    AddQuery(DBCacheTable::AchievementCriteriaData);
    AddQuery(DBCacheTable::AchievementReward);
    AddQuery(DBCacheTable::BattlemasterEntry);
    AddQuery(DBCacheTable::GameTele);
    AddQuery(DBCacheTable::GossipMenu);
    AddQuery(DBCacheTable::GossipMenuOption);
    AddQuery(DBCacheTable::NpcVendor);
    AddQuery(DBCacheTable::NpcTrainer);
    AddQuery(DBCacheTable::WaypointData);
    AddQuery(DBCacheTable::CreatureFormations);
    AddQuery(DBCacheTable::Conditions);
    AddQuery(DBCacheTable::PlayerFactionchangeAchievement);
    AddQuery(DBCacheTable::PlayerFactionchangeSpells);
    AddQuery(DBCacheTable::PlayerFactionchangeItems);
    AddQuery(DBCacheTable::PlayerFactionchangeReputations);
    AddQuery(DBCacheTable::PlayerFactionchangeTitles);
    AddQuery(DBCacheTable::PlayerFactionchangeQuest);
    AddQuery(DBCacheTable::SpellScriptNames);
    AddQuery(DBCacheTable::CreatureTextLocale);
    AddQuery(DBCacheTable::BattlegroundTemplate);
    AddQuery(DBCacheTable::OutdoorpvpTemplate);
}

void DBCacheMgr::InitializeDefines()
{
    // Prepare
    _queryStrings.rehash(AsUnderlyingType(DBCacheTable::Max));

    InitGameLocaleStrings();
    InitGameEventStrings();
    InitSpellsStrings();
    InitScriptStrings();
    InitGossipStrings();
    InitGameObjectStrings();
    InitItemStrings();
    InitCreatureStrings();
    InitMiscStrings();
    InitQuestStrings();
    InitPoolStrings();
}
