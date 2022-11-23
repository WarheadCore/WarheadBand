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

#ifndef WARHEAD_ASYNC_DB_LOAD_STRINGS_H_
#define WARHEAD_ASYNC_DB_LOAD_STRINGS_H_

#include "Define.h"

enum class DBCacheTable : std::size_t
{
    // Game locale
    WarheadStrings,
    BroadcastTexts,
    BroadcastTextLocales,
    AchievementRewardLocales,
    CreatureLocales,
    GameObjectLocales,
    ItemLocales,
    ItemSetNameLocales,
    NpcTextLocales,
    PageTextLocales,
    GossipMenuItemsLocales,
    PointOfInterestLocales,
    QuestLocales,
    QuestOfferRewardLocale,
    QuestRequestItemsLocale,
    QuestGreetingLocales,
    ChatCommandsLocales,
    RaceStrings,
    ClassStrings,
    CommonStrings,
    CreatureTextLocale,

    // Game event
    GameEvent,
    GameEventMaxEvent,
    GameEventPrerequisite,
    GameEventCreature,
    GameEventGameobject,
    GameEventModelEquip,
    GameEventCreatureQuest,
    GameEventGameobjectQuest,
    GameEventQuestCondition,
    GameEventCondition,
    GameEventNpcFlag,
    GameEventSeasonalQuestrelation,
    GameEventNpcVendor,
    GameEventBattlegroundHoliday,
    GameEventPool,
    HolidayDates,

    // Spells
    SpellRanks,
    SpellCustomAttr,
    SpellRequired,
    SpellGroup,
    SpellProc,
    SpellProcEvent,
    SpellBonusData,
    SpellThreat,
    SpellMixology,
    SpellGroupStackRules,
    SpellEnchantProcData,
    SpellArea,
    SpellPetAuras,
    SpellTargetPosition,
    SpellLinkedSpell,

    // Scripts
    ScriptNames,
    AreatriggerScripts,
    SpellScriptNames,

    // Gossip
    NpcText,
    PageText,
    GossipMenu,
    GossipMenuOption,
    NpcVendor,
    NpcTrainer,

    // Items
    ItemTemplate,
    ItemEnchantmentTemplate,
    ItemSetNames,

    // Creature
    Creature,
    CreatureModelInfo,
    CreatureTemplate,
    CreatureEquipTemplate,
    CreatureTemplateAddon,
    CreatureOnkillReputation,
    CreatureClassLevelStats,
    CreatureSummonGroups,
    CreatureAddon,
    CreatureMovementOverride,
    CreatureQuestItem,
    NpcSpellClickSpells,
    CreatureFormations,

    // GameObject
    Gameobject,
    GameobjectAddon,
    GameobjectTemplate,
    GameobjectTemplateAddon,
    GameObjectQuestItem,
    TransportTemplates,

    // Quest
    QuestTemplate,
    QuestPOI,
    QuestGreeting,
    QuestMoneyReward,
    QuestPOIPoints,

    // Pool
    PoolTemplate,
    PoolCreature,
    PoolGameobject,
    PoolPool,
    PoolObjects,

    // Misc
    ReputationRewardRate,
    ReputationSpilloverTemplate,
    PointsOfInterest,
    LinkedRespawn,
    GameWeather,
    VehicleAccessory,
    VehicleTemplateAccessory,
    Areatrigger,
    AreatriggerTeleport,
    AreatriggerInvolvedrelation,
    AreatriggerTavern,
    LfgDungeonTemplate,
    LfgDungeonRewards,
    GraveyardZone,
    GameGraveyard,
    PlayerCreateInfo,
    PlayerCreateInfoItem,
    PlayerCreateInfoSkills,
    PlayerCreateInfoSpellCustom,
    PlayerCreateInfoCastSpell,
    PlayerCreateInfoAction,
    PlayerClassLevelStats,
    PlayerLevelStats,
    PlayerXpForLevel,
    ExplorationBasexp,
    PetNameGeneration,
    PetLevelstats,
    MailLevelReward,
    SkillDiscoveryTemplate,
    SkillExtraItemTemplate,
    SkillPerfectItemTemplate,
    SkillFishingBaseLevel,
    AchievementCriteriaData,
    AchievementReward,
    BattlemasterEntry,
    GameTele,
    WaypointData,
    Conditions,
    PlayerFactionchangeAchievement,
    PlayerFactionchangeSpells,
    PlayerFactionchangeItems,
    PlayerFactionchangeReputations,
    PlayerFactionchangeTitles,
    PlayerFactionchangeQuest,
    BattlegroundTemplate,
    OutdoorpvpTemplate,
    Disables,
    InstanceTemplate,
    InstanceEncounters,

    Max
};

#endif
