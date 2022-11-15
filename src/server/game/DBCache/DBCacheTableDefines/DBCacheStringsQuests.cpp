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

void DBCacheMgr::InitQuestStrings()
{
    _queryStrings.emplace(DBCacheTable::QuestTemplate, "SELECT ID, QuestType, QuestLevel, MinLevel, QuestSortID, QuestInfoID, SuggestedGroupNum, TimeAllowed, AllowableRaces,"
                                                   "RequiredFactionId1, RequiredFactionId2, RequiredFactionValue1, RequiredFactionValue2, "
                                                   "RewardNextQuest, RewardXPDifficulty, RewardMoney, RewardMoneyDifficulty, RewardBonusMoney,  RewardDisplaySpell, RewardSpell, RewardHonor, RewardKillHonor, "
                                                   "StartItem, Flags, RewardTitle, RequiredPlayerKills, RewardTalents, RewardArenaPoints, "
                                                   "RewardItem1, RewardAmount1, RewardItem2, RewardAmount2, RewardItem3, RewardAmount3, RewardItem4, RewardAmount4, "
                                                   "RewardChoiceItemID1, RewardChoiceItemQuantity1, RewardChoiceItemID2, RewardChoiceItemQuantity2, RewardChoiceItemID3, RewardChoiceItemQuantity3, RewardChoiceItemID4, RewardChoiceItemQuantity4, RewardChoiceItemID5, RewardChoiceItemQuantity5, RewardChoiceItemID6, RewardChoiceItemQuantity6, "
                                                   "RewardFactionID1, RewardFactionValue1, RewardFactionOverride1, RewardFactionID2, RewardFactionValue2, RewardFactionOverride2, RewardFactionID3, RewardFactionValue3, RewardFactionOverride3, RewardFactionID4, RewardFactionValue4, RewardFactionOverride4, RewardFactionID5, RewardFactionValue5,  RewardFactionOverride5,"
                                                   "POIContinent, POIx, POIy, POIPriority, "
                                                   "LogTitle, LogDescription, QuestDescription, AreaDescription, QuestCompletionLog, "
                                                   "RequiredNpcOrGo1, RequiredNpcOrGo2, RequiredNpcOrGo3, RequiredNpcOrGo4, RequiredNpcOrGoCount1, RequiredNpcOrGoCount2, RequiredNpcOrGoCount3, RequiredNpcOrGoCount4, "
                                                   "ItemDrop1, ItemDrop2, ItemDrop3, ItemDrop4, ItemDropQuantity1, ItemDropQuantity2, ItemDropQuantity3, ItemDropQuantity4, "
                                                   "RequiredItemId1, RequiredItemId2, RequiredItemId3, RequiredItemId4, RequiredItemId5, RequiredItemId6, RequiredItemCount1, RequiredItemCount2, RequiredItemCount3, RequiredItemCount4, RequiredItemCount5, RequiredItemCount6, "
                                                   "Unknown0, ObjectiveText1, ObjectiveText2, ObjectiveText3, ObjectiveText4 FROM quest_template");
    _queryStrings.emplace(DBCacheTable::QuestPOI, "SELECT QuestID, id, ObjectiveIndex, MapID, WorldMapAreaId, Floor, Priority, Flags FROM quest_poi order by QuestID");
    _queryStrings.emplace(DBCacheTable::QuestGreeting, "SELECT ID, Type, GreetEmoteType, GreetEmoteDelay, Greeting FROM quest_greeting");
    _queryStrings.emplace(DBCacheTable::QuestMoneyReward, "SELECT `Level`, Money0, Money1, Money2, Money3, Money4, Money5, Money6, Money7, Money8, Money9 FROM `quest_money_reward` ORDER BY `Level`");
}
