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

void DBCacheMgr::InitGameLocaleStrings()
{
    _queryStrings.emplace(DBCacheTable::WarheadStrings, "SELECT entry, content_default, locale_koKR, locale_frFR, locale_deDE, locale_zhCN, locale_zhTW, locale_esES, locale_esMX, locale_ruRU FROM acore_string");
    _queryStrings.emplace(DBCacheTable::BroadcastTexts, "SELECT ID, LanguageID, Text, Text1, EmoteID1, EmoteID2, EmoteID3, EmoteDelay1, EmoteDelay2, EmoteDelay3, SoundEntriesID, EmotesID, Flags FROM broadcast_text");
    _queryStrings.emplace(DBCacheTable::BroadcastTextLocales, "SELECT ID, locale, Text, Text1 FROM broadcast_text_locale");
    _queryStrings.emplace(DBCacheTable::AchievementRewardLocales, "SELECT ID, Locale, Subject, Text FROM achievement_reward_locale");
    _queryStrings.emplace(DBCacheTable::CreatureLocales, "SELECT entry, locale, Name, Title FROM creature_template_locale");
    _queryStrings.emplace(DBCacheTable::GossipMenuItemsLocales, "SELECT MenuID, OptionID, Locale, OptionText, BoxText FROM gossip_menu_option_locale");
    _queryStrings.emplace(DBCacheTable::GameObjectLocales, "SELECT entry, locale, name, castBarCaption FROM gameobject_template_locale");
    _queryStrings.emplace(DBCacheTable::ItemLocales, "SELECT ID, locale, Name, Description FROM item_template_locale");
    _queryStrings.emplace(DBCacheTable::ItemSetNameLocales, "SELECT ID, locale, Name FROM item_set_names_locale");
    _queryStrings.emplace(DBCacheTable::NpcTextLocales, "SELECT ID, Locale, "
                                                        "Text0_0, Text0_1, Text1_0, Text1_1, Text2_0, Text2_1, Text3_0, Text3_1, Text4_0, Text4_1, Text5_0, Text5_1, Text6_0, Text6_1, Text7_0, Text7_1 "
                                                        "FROM npc_text_locale");
    _queryStrings.emplace(DBCacheTable::PageTextLocales, "SELECT ID, locale, Text FROM page_text_locale");
    _queryStrings.emplace(DBCacheTable::PointOfInterestLocales, "SELECT ID, locale, Name FROM points_of_interest_locale");
    _queryStrings.emplace(DBCacheTable::QuestLocales, "SELECT ID, locale, Title, Details, Objectives, EndText, CompletedText, ObjectiveText1, ObjectiveText2, ObjectiveText3, ObjectiveText4 FROM quest_template_locale");
    _queryStrings.emplace(DBCacheTable::QuestOfferRewardLocale, "SELECT Id, locale, RewardText FROM quest_offer_reward_locale");
    _queryStrings.emplace(DBCacheTable::QuestRequestItemsLocale, "SELECT Id, locale, CompletionText FROM quest_request_items_locale");
    _queryStrings.emplace(DBCacheTable::QuestGreetingLocales, "SELECT ID, Type, Locale, Greeting FROM quest_greeting_locale");
    _queryStrings.emplace(DBCacheTable::ChatCommandsLocales, "SELECT `Command`, `Locale`, `Content` FROM commands_help_locale");
//    _queryStrings.emplace(DBCacheTable::AutoBroadCastLocales, Warhead::StringFormat("SELECT `ID`, `Locale`, `Text` FROM `autobroadcast_locale` WHERE `RealmID` = -1 OR RealmID = '{}'", realm.Id.Realm));
    _queryStrings.emplace(DBCacheTable::RaceStrings, "SELECT ID, Locale, NameMale, NameFemale FROM `string_race`");
    _queryStrings.emplace(DBCacheTable::ClassStrings, "SELECT ID, Locale, NameMale, NameFemale FROM `string_class`");
    _queryStrings.emplace(DBCacheTable::CommonStrings, "SELECT `Entry`, `Locale`, `Content` FROM `string_warhead`");
}
