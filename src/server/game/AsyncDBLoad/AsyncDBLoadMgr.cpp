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

#include "AsyncDBLoadMgr.h"
#include "DatabaseEnv.h"
#include "Util.h"
#include "StopWatch.h"
#include "Realm.h"
#include "StringFormat.h"

/*static*/ AsyncDBLoadMgr* AsyncDBLoadMgr::instance()
{
    static AsyncDBLoadMgr instance;
    return &instance;
}

void AsyncDBLoadMgr::Initialize()
{
    LOG_INFO("server.loading", "Initialize async db query list");

    StopWatch sw;

    InitializeDefines();
    InitializeQuery();

    LOG_INFO("server.loading", ">> Initialized async db query list in {}", sw);
    LOG_INFO("server.loading", "");
}

void AsyncDBLoadMgr::AddQuery(AsyncDBTable index)
{
    if (_queryList.contains(index))
    {
        LOG_ERROR("db.async", "Query with index {} exist!");
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
    _queryList.emplace(index, std::move(result));
}

QueryResult AsyncDBLoadMgr::GetResult(AsyncDBTable index)
{
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

std::string_view AsyncDBLoadMgr::GetStringQuery(AsyncDBTable index)
{
    auto const& itr = _queryStrings.find(index);
    if (itr == _queryStrings.end())
        return {};

    return itr->second;
}

void AsyncDBLoadMgr::InitializeQuery()
{
    for (std::size_t i{}; i < AsUnderlyingType(AsyncDBTable::Max); i++)
        AddQuery(static_cast<AsyncDBTable>(i));
}

void AsyncDBLoadMgr::InitializeDefines()
{
    // Prepare
    _queryStrings.rehash(AsUnderlyingType(AsyncDBTable::Max));

    _queryStrings.emplace(AsyncDBTable::WarheadStrings, "SELECT entry, content_default, locale_koKR, locale_frFR, locale_deDE, locale_zhCN, locale_zhTW, locale_esES, locale_esMX, locale_ruRU FROM acore_string");

    //                                                         0   1           2     3      4         5         6         7            8            9            10              11        12
    _queryStrings.emplace(AsyncDBTable::BroadcastTexts, "SELECT ID, LanguageID, Text, Text1, EmoteID1, EmoteID2, EmoteID3, EmoteDelay1, EmoteDelay2, EmoteDelay3, SoundEntriesID, EmotesID, Flags FROM broadcast_text");

    //                                                               0   1       2     3
    _queryStrings.emplace(AsyncDBTable::BroadcastTextLocales, "SELECT ID, locale, Text, Text1 FROM broadcast_text_locale");

    //                                                             0   1       2        3
    _queryStrings.emplace(AsyncDBTable::AchievementRewardLocales, "SELECT ID, Locale, Subject, Text FROM achievement_reward_locale");

    //                                                           0      1       2     3
    _queryStrings.emplace(AsyncDBTable::CreatureLocales, "SELECT entry, locale, Name, Title FROM creature_template_locale");

    //                                                                       0       1            2       3           4
    _queryStrings.emplace(AsyncDBTable::GossipMenuItemsLocales, "SELECT MenuID, OptionID, Locale, OptionText, BoxText FROM gossip_menu_option_locale");

    //                                                                 0      1       2     3
    _queryStrings.emplace(AsyncDBTable::GameObjectLocales, "SELECT entry, locale, name, castBarCaption FROM gameobject_template_locale");

    _queryStrings.emplace(AsyncDBTable::ItemLocales, "SELECT ID, locale, Name, Description FROM item_template_locale");
    _queryStrings.emplace(AsyncDBTable::ItemSetNameLocales, "SELECT ID, locale, Name FROM item_set_names_locale");

    _queryStrings.emplace(AsyncDBTable::NpcTextLocales, "SELECT ID, Locale, "
                                             //   2        3        4        5        6        7        8        9        10       11       12       13       14       15       16       17
                                             "Text0_0, Text0_1, Text1_0, Text1_1, Text2_0, Text2_1, Text3_0, Text3_1, Text4_0, Text4_1, Text5_0, Text5_1, Text6_0, Text6_1, Text7_0, Text7_1 "
                                             "FROM npc_text_locale");

    //                                                           0   1       2
    _queryStrings.emplace(AsyncDBTable::PageTextLocales, "SELECT ID, locale, Text FROM page_text_locale");

    //                                                                  0   1       2
    _queryStrings.emplace(AsyncDBTable::PointOfInterestLocales, "SELECT ID, locale, Name FROM points_of_interest_locale");

    //                                                        0   1       2      3        4           5        6              7               8               9               10
    _queryStrings.emplace(AsyncDBTable::QuestLocales, "SELECT ID, locale, Title, Details, Objectives, EndText, CompletedText, ObjectiveText1, ObjectiveText2, ObjectiveText3, ObjectiveText4 FROM quest_template_locale");

    //                                                                  0     1          2
    _queryStrings.emplace(AsyncDBTable::QuestOfferRewardLocale, "SELECT Id, locale, RewardText FROM quest_offer_reward_locale");

    //                                                                   0     1          2
    _queryStrings.emplace(AsyncDBTable::QuestRequestItemsLocale, "SELECT Id, locale, CompletionText FROM quest_request_items_locale");

    //                                                                0     1      2       3
    _queryStrings.emplace(AsyncDBTable::QuestGreetingLocales, "SELECT ID, Type, Locale, Greeting FROM quest_greeting_locale");

    //                                                                0          1         2
    _queryStrings.emplace(AsyncDBTable::ChatCommandsLocales, "SELECT `Command`, `Locale`, `Content` FROM commands_help_locale");

    //                                                                                       0     1         2
//    _queryStrings.emplace(AsyncDBTable::AutoBroadCastLocales, Warhead::StringFormat("SELECT `ID`, `Locale`, `Text` FROM `autobroadcast_locale` WHERE `RealmID` = -1 OR RealmID = '{}'", realm.Id.Realm));

    _queryStrings.emplace(AsyncDBTable::RaceStrings, "SELECT ID, Locale, NameMale, NameFemale FROM `string_race`");
    _queryStrings.emplace(AsyncDBTable::ClassStrings, "SELECT ID, Locale, NameMale, NameFemale FROM `string_class`");
    _queryStrings.emplace(AsyncDBTable::CommonStrings, "SELECT `Entry`, `Locale`, `Content` FROM `string_warhead`");
}
