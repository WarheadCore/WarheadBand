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

    // Game event
    GameEventMaxEvent,

    // Game graveyard
    GameGraveyard,

    // Spells
    SpellRank,
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

    // Scripts
    ScriptNames,

    // Instances
    InstanceTemplate,
    InstanceEncounters,

    // Gossip
    NpcText,
    PageText,

    // Items
    ItemTemplate,
    ItemEnchantmentTemplate,
    ItemSetNames,

    // Disables
    Disables,

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

    // GameObject
    Gameobject,
    GameobjectAddon,
    GameobjectTemplate,
    GameobjectTemplateAddon,
    GameObjectQuestItem,
    TransportTemplates,

    // Misc
    ReputationRewardRate,
    ReputationSpilloverTemplate,
    PointsOfInterest,
    LinkedRespawn,
    GameWeather,

    // Quest
    QuestTemplate,
    QuestPOI,
    QuestGreeting,
    QuestMoneyReward,

    Max
};

#endif
