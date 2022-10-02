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

#include "GameLocale.h"
#include "AccountMgr.h"
#include "Chat.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "GameConfig.h"
#include "LocaleCommon.h"
#include "Log.h"
#include "ModuleLocale.h"
#include "ObjectMgr.h"
#include "Realm.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "StopWatch.h"
#include "World.h"

namespace TimeDiff // in us
{
    constexpr uint64 MILLISECONDS = 1000;
    constexpr uint64 SECONDS = 1000 * MILLISECONDS;
    constexpr uint64 MINUTES = 60 * SECONDS;
    constexpr uint64 HOURS = 60 * MINUTES;
    constexpr uint64 DAYS = 24 * HOURS;
}

constexpr std::string_view GetruRUTimeEnding(uint64 number, std::string_view ending0, std::string_view ending1, std::string_view ending2)
{
    uint16 num100 = number % 100;
    uint16 num10 = number % 10;

    if (num100 >= 5 && num100 <= 20)
        return ending0; // дней
    else if (num10 == 0)
        return ending0; // дней
    else if (num10 == 1)
        return ending1; // день
    else if (num10 >= 2 && num10 <= 4)
        return ending2; // дня
    else if (num10 >= 5 && num10 <= 9)
        return ending0; // дней

    return ending2; // дня
}

GameLocale* GameLocale::instance()
{
    static GameLocale instance;
    return &instance;
}

void GameLocale::LoadAllLocales()
{
    // Get once for all the locale index of DBC language (console/broadcasts)
    SetDBCLocaleIndex(sWorld->GetDefaultDbcLocale());

    StopWatch sw;

    LoadBroadcastTexts();
    LoadBroadcastTextLocales();
    LoadAchievementRewardLocales();
    LoadCreatureLocales();
    LoadGameObjectLocales();
    LoadItemLocales();
    LoadItemSetNameLocales();
    LoadNpcTextLocales();
    LoadPageTextLocales();
    LoadGossipMenuItemsLocales();
    LoadPointOfInterestLocales();
    LoadQuestLocales();
    LoadQuestOfferRewardLocale();
    LoadQuestRequestItemsLocale();
    LoadChatCommandsLocales();
    LoadAutoBroadCastLocales();
    //LoadQuestGreetingLocales(); // not implement

    // Load new strings
    LoadRaceStrings();
    LoadClassStrings();
    LoadCommonStrings();

    // Load modules strings
    sModuleLocale->Init();

    LOG_INFO("server.loading", ">> Localization strings loaded in {}", sw);
    LOG_INFO("server.loading", "");
}

bool GameLocale::LoadWarheadStrings()
{
    StopWatch sw;

    _warheadStringStore.clear(); // for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry, content_default, locale_koKR, locale_frFR, locale_deDE, locale_zhCN, locale_zhTW, locale_esES, locale_esMX, locale_ruRU FROM acore_string");
    if (!result)
    {
        LOG_WARN("db.query", ">> Loaded 0 warhead strings. DB table `warhead_strings` is empty.");
        LOG_WARN("db.query", "");
        return false;
    }

    _warheadStringStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 entry = fields[0].Get<uint32>();

        auto& data = _warheadStringStore[entry];

        data.Content.resize(MAX_LOCALES);

        for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
            Warhead::Locale::AddLocaleString(fields[i + 1].Get<std::string_view>(), LocaleConstant(i), data.Content);
    }

    LOG_INFO("server.loading", ">> Loaded {} warhead strings in {}", _warheadStringStore.size(), sw);
    LOG_INFO("server.loading", "");
    return true;
}

std::string GameLocale::GetWarheadString(uint32 entry, LocaleConstant locale) const
{
    if (auto as = GetWarheadString(entry))
    {
        if (as->Content.size() > size_t(locale) && !as->Content[locale].empty())
            return as->Content[locale];

        return as->Content[DEFAULT_LOCALE];
    }

    LOG_ERROR("db.query", "Warhead string entry {} not found in DB.", entry);
    return "<error>";
}

WarheadString const* GameLocale::GetWarheadString(uint32 entry) const
{
    auto const& itr = _warheadStringStore.find(entry);
    if (itr == _warheadStringStore.end())
        return nullptr;

    return &itr->second;
}

void GameLocale::LoadAchievementRewardLocales()
{
    StopWatch sw;

    _achievementRewardLocales.clear();                       // need for reload case

    //                                               0   1       2        3
    QueryResult result = WorldDatabase.Query("SELECT ID, Locale, Subject, Text FROM achievement_reward_locale");

    if (!result)
    {
        LOG_WARN("db.query", ">> Loaded 0 achievement reward locale strings. DB table `achievement_reward_locale` is empty");
        LOG_WARN("db.query", "");
        return;
    }

    _achievementRewardLocales.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 ID = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        AchievementRewardLocale& data = _achievementRewardLocales[ID];

        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.Subject);
        Warhead::Locale::AddLocaleString(fields[3].Get<std::string_view>(), locale, data.Text);
    }

    LOG_INFO("server.loading", ">> Loaded {} Achievement Reward Locale strings in {}", _achievementRewardLocales.size(), sw);
}

void GameLocale::LoadBroadcastTexts()
{
    StopWatch sw;

    _broadcastTextStore.clear(); // for reload case

    //                                               0   1           2     3      4         5         6         7            8            9            10              11        12
    QueryResult result = WorldDatabase.Query("SELECT ID, LanguageID, Text, Text1, EmoteID1, EmoteID2, EmoteID3, EmoteDelay1, EmoteDelay2, EmoteDelay3, SoundEntriesID, EmotesID, Flags FROM broadcast_text");
    if (!result)
    {
        LOG_WARN("db.query", ">> Loaded 0 broadcast texts. DB table `broadcast_text` is empty.");
        LOG_WARN("db.query", "");
        return;
    }

    _broadcastTextStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        BroadcastText bct;

        bct.Id = fields[0].Get<uint32>();
        bct.LanguageID = fields[1].Get<uint32>();
        bct.Text[DEFAULT_LOCALE] = fields[2].Get<std::string>();
        bct.Text1[DEFAULT_LOCALE] = fields[3].Get<std::string>();
        bct.EmoteId1 = fields[4].Get<uint32>();
        bct.EmoteId2 = fields[5].Get<uint32>();
        bct.EmoteId3 = fields[6].Get<uint32>();
        bct.EmoteDelay1 = fields[7].Get<uint32>();
        bct.EmoteDelay2 = fields[8].Get<uint32>();
        bct.EmoteDelay3 = fields[9].Get<uint32>();
        bct.SoundEntriesID = fields[10].Get<uint32>();
        bct.EmotesID = fields[11].Get<uint32>();
        bct.Flags = fields[12].Get<uint32>();

        if (bct.SoundEntriesID && !sSoundEntriesStore.LookupEntry(bct.SoundEntriesID))
        {
            LOG_DEBUG("broadcasttext", "BroadcastText (Id: {}) in table `broadcast_text` has SoundEntriesID {} but sound does not exist.", bct.Id, bct.SoundEntriesID);
            bct.SoundEntriesID = 0;
        }

        if (!GetLanguageDescByID(bct.LanguageID))
        {
            LOG_DEBUG("broadcasttext", "BroadcastText (Id: {}) in table `broadcast_text` using LanguageID {} but Language does not exist.", bct.Id, bct.LanguageID);
            bct.LanguageID = LANG_UNIVERSAL;
        }

        if (bct.EmoteId1 && !sEmotesStore.LookupEntry(bct.EmoteId1))
        {
            LOG_DEBUG("broadcasttext", "BroadcastText (Id: {}) in table `broadcast_text` has EmoteId1 {} but emote does not exist.", bct.Id, bct.EmoteId1);
            bct.EmoteId1 = 0;
        }

        if (bct.EmoteId2 && !sEmotesStore.LookupEntry(bct.EmoteId2))
        {
            LOG_DEBUG("broadcasttext", "BroadcastText (Id: {}) in table `broadcast_text` has EmoteId2 {} but emote does not exist.", bct.Id, bct.EmoteId2);
            bct.EmoteId2 = 0;
        }

        if (bct.EmoteId3 && !sEmotesStore.LookupEntry(bct.EmoteId3))
        {
            LOG_DEBUG("broadcasttext", "BroadcastText (Id: {}) in table `broadcast_text` has EmoteId3 {} but emote does not exist.", bct.Id, bct.EmoteId3);
            bct.EmoteId3 = 0;
        }

        _broadcastTextStore.emplace(bct.Id, bct);
    }

    LOG_INFO("server.loading", ">> Loaded {} broadcast texts in {}", _broadcastTextStore.size(), sw);
}

void GameLocale::LoadBroadcastTextLocales()
{
    StopWatch sw;

    //                                               0   1       2     3
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Text, Text1 FROM broadcast_text_locale");

    if (!result)
    {
        LOG_WARN("db.query", ">> Loaded 0 broadcast text locales. DB table `broadcast_text_locale` is empty.");
        LOG_WARN("db.query", "");
        return;
    }

    for (auto const& fields : *result)
    {
        uint32 id = fields[0].Get<uint32>();

        auto const& bct = _broadcastTextStore.find(id);
        if (bct == _broadcastTextStore.end())
        {
            LOG_ERROR("db.query", "BroadcastText (Id: {}) in table `broadcast_text_locale` does not exist. Skipped!", id);
            continue;
        }

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, bct->second.Text);
        Warhead::Locale::AddLocaleString(fields[3].Get<std::string_view>(), locale, bct->second.Text1);
    }

    LOG_INFO("server.loading", ">> Loaded {} broadcast text locales in {}", _broadcastTextStore.size(), sw);
}

void GameLocale::LoadCreatureLocales()
{
    StopWatch sw;

    _creatureLocaleStore.clear();                              // need for reload case

    //                                               0      1       2     3
    QueryResult result = WorldDatabase.Query("SELECT entry, locale, Name, Title FROM creature_template_locale");
    if (!result)
        return;

    _creatureLocaleStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 id = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        CreatureLocale& data = _creatureLocaleStore[id];

        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.Name);
        Warhead::Locale::AddLocaleString(fields[3].Get<std::string_view>(), locale, data.Title);
    }

    LOG_INFO("server.loading", ">> Loaded {} creature Locale strings in {}", _creatureLocaleStore.size(), sw);
}

void GameLocale::LoadGossipMenuItemsLocales()
{
    StopWatch sw;
    _gossipMenuItemsLocaleStore.clear();                              // need for reload case

    //                                               0       1            2       3           4
    QueryResult result = WorldDatabase.Query("SELECT MenuID, OptionID, Locale, OptionText, BoxText FROM gossip_menu_option_locale");

    if (!result)
        return;

    _gossipMenuItemsLocaleStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint16 menuId = fields[0].Get<uint16>();
        uint16 optionId = fields[1].Get<uint16>();

        LocaleConstant locale = GetLocaleByName(fields[2].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        GossipMenuItemsLocale& data = _gossipMenuItemsLocaleStore[MAKE_PAIR32(menuId, optionId)];

        Warhead::Locale::AddLocaleString(fields[3].Get<std::string_view>(), locale, data.OptionText);
        Warhead::Locale::AddLocaleString(fields[4].Get<std::string_view>(), locale, data.BoxText);
    }

    LOG_INFO("server.loading", ">> Loaded {} Gossip Menu Option Locale strings in {}", _gossipMenuItemsLocaleStore.size(), sw);
}

void GameLocale::LoadGameObjectLocales()
{
    StopWatch sw;
    _gameObjectLocaleStore.clear(); // need for reload case

    //                                               0      1       2     3
    QueryResult result = WorldDatabase.Query("SELECT entry, locale, name, castBarCaption FROM gameobject_template_locale");
    if (!result)
        return;

    _gameObjectLocaleStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 id = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        GameObjectLocale& data = _gameObjectLocaleStore[id];

        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.Name);
        Warhead::Locale::AddLocaleString(fields[3].Get<std::string_view>(), locale, data.CastBarCaption);
    }

    LOG_INFO("server.loading", ">> Loaded {} Gameobject Locale strings in {}", _gameObjectLocaleStore.size(), sw);
}

void GameLocale::LoadItemLocales()
{
    StopWatch sw;
    _itemLocaleStore.clear();                                 // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Name, Description FROM item_template_locale");
    if (!result)
        return;

    _itemLocaleStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 id = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        ItemLocale& data = _itemLocaleStore[id];

        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.Name);
        Warhead::Locale::AddLocaleString(fields[3].Get<std::string_view>(), locale, data.Description);
    }

    LOG_INFO("server.loading", ">> Loaded {} Item Locale strings in {}", _itemLocaleStore.size(), sw);
}

void GameLocale::LoadItemSetNameLocales()
{
    StopWatch sw;
    _itemSetNameLocaleStore.clear();                                 // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Name FROM item_set_names_locale");
    if (!result)
        return;

    _itemSetNameLocaleStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 id = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        ItemSetNameLocale& data = _itemSetNameLocaleStore[id];

        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.Name);
    }

    LOG_INFO("server.loading", ">> Loaded {} Item Set Name Locale strings in {}", _itemSetNameLocaleStore.size(), sw);
}

void GameLocale::LoadNpcTextLocales()
{
    StopWatch sw;
    _npcTextLocaleStore.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID, Locale, "
                         //   2        3        4        5        6        7        8        9        10       11       12       13       14       15       16       17
                         "Text0_0, Text0_1, Text1_0, Text1_1, Text2_0, Text2_1, Text3_0, Text3_1, Text4_0, Text4_1, Text5_0, Text5_1, Text6_0, Text6_1, Text7_0, Text7_1 "
                         "FROM npc_text_locale");
    if (!result)
        return;

    _npcTextLocaleStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 id = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        NpcTextLocale& data = _npcTextLocaleStore[id];

        for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
        {
            Warhead::Locale::AddLocaleString(fields[2 + i * 2].Get<std::string_view>(), locale, data.Text_0[i]);
            Warhead::Locale::AddLocaleString(fields[3 + i * 2].Get<std::string_view>(), locale, data.Text_1[i]);
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} Npc Text Locale strings in {}", _npcTextLocaleStore.size(), sw);
}

void GameLocale::LoadPageTextLocales()
{
    StopWatch sw;
    _pageTextLocaleStore.clear();                             // need for reload case

    //                                               0   1       2
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Text FROM page_text_locale");
    if (!result)
        return;

    _pageTextLocaleStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 id = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        PageTextLocale& data = _pageTextLocaleStore[id];
        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.Text);
    }

    LOG_INFO("server.loading", ">> Loaded {} Page Text Locale strings in {}", _pageTextLocaleStore.size(), sw);
}

void GameLocale::LoadPointOfInterestLocales()
{
    StopWatch sw;
    _pointOfInterestLocaleStore.clear();                              // need for reload case

    //                                               0   1       2
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Name FROM points_of_interest_locale");
    if (!result)
        return;

    _pointOfInterestLocaleStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 id = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        PointOfInterestLocale& data = _pointOfInterestLocaleStore[id];
        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.Name);
    }

    LOG_INFO("server.loading", ">> Loaded {} Points Of Interest Locale strings in {}", _pointOfInterestLocaleStore.size(), sw);
}

void GameLocale::LoadQuestLocales()
{
    StopWatch sw;
    _questLocaleStore.clear();                                // need for reload case

    //                                               0   1       2      3        4           5        6              7               8               9               10
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Title, Details, Objectives, EndText, CompletedText, ObjectiveText1, ObjectiveText2, ObjectiveText3, ObjectiveText4 FROM quest_template_locale");
    if (!result)
        return;

    _questLocaleStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 id = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        QuestLocale& data = _questLocaleStore[id];
        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.Title);
        Warhead::Locale::AddLocaleString(fields[3].Get<std::string_view>(), locale, data.Details);
        Warhead::Locale::AddLocaleString(fields[4].Get<std::string_view>(), locale, data.Objectives);
        Warhead::Locale::AddLocaleString(fields[5].Get<std::string_view>(), locale, data.AreaDescription);
        Warhead::Locale::AddLocaleString(fields[6].Get<std::string_view>(), locale, data.CompletedText);

        for (uint8 i = 0; i < 4; ++i)
            Warhead::Locale::AddLocaleString(fields[i + 7].Get<std::string_view>(), locale, data.ObjectiveText[i]);
    }

    LOG_INFO("server.loading", ">> Loaded {} Quest Locale strings in {}", _questLocaleStore.size(), sw);
}

void GameLocale::LoadQuestOfferRewardLocale()
{
    StopWatch sw;
    _questOfferRewardLocaleStore.clear(); // need for reload case

    //                                               0     1          2
    QueryResult result = WorldDatabase.Query("SELECT Id, locale, RewardText FROM quest_offer_reward_locale");
    if (!result)
        return;

    _questOfferRewardLocaleStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 id = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        QuestOfferRewardLocale& data = _questOfferRewardLocaleStore[id];
        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.RewardText);
    }

    LOG_INFO("server.loading", ">> Loaded {} Quest Offer Reward locale strings in {}", _questOfferRewardLocaleStore.size(), sw);
}

void GameLocale::LoadQuestRequestItemsLocale()
{
    StopWatch sw;
    _questRequestItemsLocaleStore.clear(); // need for reload case

    //                                               0     1          2
    QueryResult result = WorldDatabase.Query("SELECT Id, locale, CompletionText FROM quest_request_items_locale");
    if (!result)
        return;

    _questRequestItemsLocaleStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 id = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        QuestRequestItemsLocale& data = _questRequestItemsLocaleStore[id];
        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.CompletionText);
    }

    LOG_INFO("server.loading", ">> Loaded {} Quest Request Items locale strings in {}", _questRequestItemsLocaleStore.size(), sw);
}

void GameLocale::LoadChatCommandsLocales()
{
    StopWatch sw;
    _chatCommandStringStore.clear(); // need for reload case

    //                                                     0       1        2
    QueryResult result = WorldDatabase.Query("SELECT `Command`, `Locale`, `Content` FROM commands_help_locale");
    if (!result)
        return;

    _chatCommandStringStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        std::string commandName = fields[0].Get<std::string>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        ChatCommandHelpLocale& data = _chatCommandStringStore[commandName];
        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.Content);
    }

    LOG_INFO("server.loading", ">> Loaded {} Chat commands help strings locale in {}", _chatCommandStringStore.size(), sw);
}

void GameLocale::LoadAutoBroadCastLocales()
{
    StopWatch sw;
    _autobroadLocaleStore.clear(); // need for reload case

    //                                                 0          1       2
    QueryResult result = AuthDatabase.Query("SELECT `ID`, `Locale`, `Text` FROM `autobroadcast_locale` WHERE `RealmID` = -1 OR RealmID = '{}'", realm.Id.Realm);
    if (!result)
        return;

    _autobroadLocaleStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 id = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        auto& data = _autobroadLocaleStore[id];
        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.Text);
    }

    LOG_INFO("server.loading", ">> Loaded {} autobroadcast text locale in {}", _chatCommandStringStore.size(), sw);
}

void GameLocale::LoadQuestGreetingLocales()
{
    StopWatch sw;
    _questGreetingLocaleStore.clear();                              // need for reload case

    //                                               0     1      2       3
    QueryResult result = WorldDatabase.Query("SELECT ID, Type, Locale, Greeting FROM quest_greeting_locale");
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 quest_greeting locales. DB table `quest_greeting_locale` is empty.");
        return;
    }

    _questGreetingLocaleStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 id = fields[0].Get<uint32>();
        uint8 type = fields[1].Get<uint8>();

        // overwrite
        switch (type)
        {
        case 0: // Creature
            type = TYPEID_UNIT;
            break;
        case 1: // GameObject
            type = TYPEID_GAMEOBJECT;
            break;
        default:
            break;
        }

        LocaleConstant locale = GetLocaleByName(fields[2].Get<std::string>());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        QuestGreetingLocale& data = _questGreetingLocaleStore[MAKE_PAIR32(id, type)];
        Warhead::Locale::AddLocaleString(fields[3].Get<std::string_view>(), locale, data.Greeting);
    }

    LOG_INFO("server.loading", ">> Loaded {} quest greeting locale strings in {}", _questGreetingLocaleStore.size(), sw);
}

AchievementRewardLocale const* GameLocale::GetAchievementRewardLocale(uint32 entry) const
{
    auto const& itr = _achievementRewardLocales.find(entry);
    return itr != _achievementRewardLocales.end() ? &itr->second : nullptr;
}

BroadcastText const* GameLocale::GetBroadcastText(uint32 id) const
{
    auto const& itr = _broadcastTextStore.find(id);
    return itr != _broadcastTextStore.end() ? &itr->second : nullptr;
}

CreatureLocale const* GameLocale::GetCreatureLocale(uint32 entry) const
{
    auto const& itr = _creatureLocaleStore.find(entry);
    return itr != _creatureLocaleStore.end() ? &itr->second : nullptr;
}

GameObjectLocale const* GameLocale::GetGameObjectLocale(uint32 entry) const
{
    auto const& itr = _gameObjectLocaleStore.find(entry);
    return itr != _gameObjectLocaleStore.end() ? &itr->second : nullptr;
}

GossipMenuItemsLocale const* GameLocale::GetGossipMenuItemsLocale(uint32 entry) const
{
    auto const& itr = _gossipMenuItemsLocaleStore.find(entry);
    return itr != _gossipMenuItemsLocaleStore.end() ? &itr->second : nullptr;
}

ItemLocale const* GameLocale::GetItemLocale(uint32 entry) const
{
    auto const& itr = _itemLocaleStore.find(entry);
    return itr != _itemLocaleStore.end() ? &itr->second : nullptr;
}

ItemSetNameLocale const* GameLocale::GetItemSetNameLocale(uint32 entry) const
{
    auto const& itr = _itemSetNameLocaleStore.find(entry);
    return itr != _itemSetNameLocaleStore.end() ? &itr->second : nullptr;
}

NpcTextLocale const* GameLocale::GetNpcTextLocale(uint32 entry) const
{
    auto const& itr = _npcTextLocaleStore.find(entry);
    return itr != _npcTextLocaleStore.end() ? &itr->second : nullptr;
}

PageTextLocale const* GameLocale::GetPageTextLocale(uint32 entry) const
{
    auto const& itr = _pageTextLocaleStore.find(entry);
    return itr != _pageTextLocaleStore.end() ? &itr->second : nullptr;
}

PointOfInterestLocale const* GameLocale::GetPointOfInterestLocale(uint32 entry) const
{
    auto const& itr = _pointOfInterestLocaleStore.find(entry);
    return itr != _pointOfInterestLocaleStore.end() ? &itr->second : nullptr;
}

QuestLocale const* GameLocale::GetQuestLocale(uint32 entry) const
{
    auto const& itr = _questLocaleStore.find(entry);
    return itr != _questLocaleStore.end() ? &itr->second : nullptr;
}

QuestOfferRewardLocale const* GameLocale::GetQuestOfferRewardLocale(uint32 entry) const
{
    auto const& itr = _questOfferRewardLocaleStore.find(entry);
    return itr != _questOfferRewardLocaleStore.end() ? &itr->second : nullptr;
}

QuestRequestItemsLocale const* GameLocale::GetQuestRequestItemsLocale(uint32 entry) const
{
    auto const& itr = _questRequestItemsLocaleStore.find(entry);
    return itr != _questRequestItemsLocaleStore.end() ? &itr->second : nullptr;
}

QuestGreetingLocale const* GameLocale::GetQuestGreetingLocale(uint32 id) const
{
    auto const& itr = _questGreetingLocaleStore.find(id);
    return itr != _questGreetingLocaleStore.end() ? &itr->second : nullptr;
}

AutobroadcastLocale const* GameLocale::GetAutoBroadCastLocale(uint32 id) const
{
    auto const& itr = _autobroadLocaleStore.find(id);
    return itr != _autobroadLocaleStore.end() ? &itr->second : nullptr;
}

Optional<std::string> GameLocale::GetChatCommandStringHelpLocale(std::string const& commandName, LocaleConstant locale) const
{
    auto const& itr = _chatCommandStringStore.find(commandName);
    if (itr == _chatCommandStringStore.end())
    {
        //LOG_ERROR("db.query", "> Missing help text localisation for command '{}'", commandName);
        return std::nullopt;
    }

    if (itr->second.Content.size() > size_t(locale) && !itr->second.Content[locale].empty())
    {
        return itr->second.Content.at(locale);
    }

    return itr->second.Content[DEFAULT_LOCALE];
}

// New locale
void GameLocale::LoadRaceStrings()
{
    StopWatch sw;

    _raceStringStore.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID, Locale, NameMale, NameFemale FROM `string_race`");
    if (!result)
    {
        LOG_WARN("db.query", "> DB table `string_race` is empty");
        return;
    }

    _raceStringStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 ID = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        auto& data = _raceStringStore[ID];
        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.NameMale);
        Warhead::Locale::AddLocaleString(fields[3].Get<std::string_view>(), locale, data.NameFemale);
    }

    LOG_INFO("server.loading", ">> Loaded {} Race strings in {}", _raceStringStore.size(), sw);
}

void GameLocale::LoadClassStrings()
{
    StopWatch sw;
    _classStringStore.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID, Locale, NameMale, NameFemale FROM `string_class`");
    if (!result)
    {
        LOG_WARN("db.query", "> DB table `string_class` is empty");
        return;
    }

    _classStringStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        uint32 ID = fields[0].Get<uint32>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        auto& data = _classStringStore[ID];
        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.NameMale);
        Warhead::Locale::AddLocaleString(fields[3].Get<std::string_view>(), locale, data.NameFemale);
    }

    LOG_INFO("server.loading", ">> Loaded {} Class strings in {}", _classStringStore.size(), sw);
}

void GameLocale::LoadCommonStrings()
{
    StopWatch sw;
    _commonStringStore.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT `Entry`, `Locale`, `Content` FROM `string_warhead`");
    if (!result)
    {
        LOG_WARN("db.query", "> DB table `string_warhead` is empty");
        return;
    }

    _commonStringStore.rehash(result->GetRowCount());

    for (auto const& fields : *result)
    {
        auto entry = fields[0].Get<std::string>();

        LocaleConstant locale = GetLocaleByName(fields[1].Get<std::string>());

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        auto& data = _commonStringStore[entry];
        Warhead::Locale::AddLocaleString(fields[2].Get<std::string_view>(), locale, data.Context);
    }

    LOG_INFO("server.loading", ">> Loaded {} common strings in {}", _commonStringStore.size(), sw);
}

RaceString const* GameLocale::GetRaseString(uint32 id) const
{
    auto const& itr = _raceStringStore.find(id);
    return itr != _raceStringStore.end() ? &itr->second : nullptr;
}

ClassString const* GameLocale::GetClassString(uint32 id) const
{
    auto const& itr = _classStringStore.find(id);
    return itr != _classStringStore.end() ? &itr->second : nullptr;
}

CommonString const* GameLocale::GetCommonStringLocale(std::string_view entry) const
{
    auto const& itr = _commonStringStore.find(std::string{ entry });
    return itr != _commonStringStore.end() ? &itr->second : nullptr;
}

std::string const GameLocale::GetItemNameLocale(uint32 itemID, int8 index_loc /*= DEFAULT_LOCALE*/) const
{
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemID);
    ItemLocale const* itemLocale = GetItemLocale(itemID);
    std::string name;

    if (itemLocale)
        name = itemLocale->Name[index_loc];

    if (name.empty() && itemTemplate)
        name = itemTemplate->Name1;

    return name;
}

std::string const GameLocale::GetItemLink(uint32 itemID, int8 index_loc /*= DEFAULT_LOCALE*/)
{
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemID);
    if (!itemTemplate)
        return "";

    std::string name = GetItemNameLocale(itemID, index_loc);
    uint32 color = ItemQualityColors[itemTemplate ? itemTemplate->Quality : uint32(ITEM_QUALITY_POOR)];

    return Warhead::StringFormat("|c{:08x}|Hitem:{}:0:0:0:0:0:0:0:0|h[{}]|h|r", color, itemID, name);
}

std::string const GameLocale::GetSpellLink(uint32 spellID, int8 index_loc /*= DEFAULT_LOCALE*/)
{
    auto const& spell = sSpellMgr->GetSpellInfo(spellID);
    if (!spell)
        return "";

    return Warhead::StringFormat("|cffffffff|Hspell:{}|h[{}]|h|r", spell->Id, spell->SpellName[index_loc]);
}

std::string const GameLocale::GetSpellNamelocale(uint32 spellID, int8 index_loc /*= DEFAULT_LOCALE*/)
{
    auto const& spell = sSpellMgr->GetSpellInfo(spellID);
    if (!spell)
        return "";

    return spell->SpellName[index_loc];
}

std::string const GameLocale::GetCreatureNamelocale(uint32 creatureEntry, int8 index_loc /*= DEFAULT_LOCALE*/) const
{
    auto creatureTemplate = sObjectMgr->GetCreatureTemplate(creatureEntry);
    auto cretureLocale = GetCreatureLocale(creatureEntry);
    std::string name;

    if (cretureLocale)
        name = cretureLocale->Name[index_loc].c_str();

    if (name.empty() && creatureTemplate)
        name = creatureTemplate->Name.c_str();

    if (name.empty())
        name = "Unknown creature";

    return name;
}

std::string GameLocale::ToTimeString(Microseconds durationTime, int8 indexLoc /*= DEFAULT_LOCALE*/, bool isShortFormat /*= true*/)
{
    if (indexLoc != LOCALE_ruRU)
        return Warhead::Time::ToTimeString(durationTime, 3, isShortFormat ? TimeFormat::ShortText : TimeFormat::FullText);

    uint64 microsecs = durationTime.count() % 1000;
    uint64 millisecs = (durationTime.count() / TimeDiff::MILLISECONDS) % 1000;
    uint64 secs = (durationTime.count() / TimeDiff::SECONDS) % 60;
    uint64 minutes = (durationTime.count() / TimeDiff::MINUTES) % 60;
    uint64 hours = (durationTime.count() / TimeDiff::HOURS) % 24;
    uint64 days = durationTime.count() / TimeDiff::DAYS;

    std::string out;
    uint8 count = 0;
    bool isFirst = false;

    auto AddOut = [&out, &count, isShortFormat](uint32 timeCount, std::string_view shortText, std::string_view fullText)
    {
        if (count >= 3)
            return;

        out.append(Warhead::ToString(timeCount));

        if (isShortFormat)
            out.append(shortText);
        else
            out.append(fullText);

        count++;
    };

    if (days)
        AddOut(days, "д ", GetruRUTimeEnding(days, " Дней ", " День ", " Дня "));

    if (hours)
        AddOut(hours, "ч ", GetruRUTimeEnding(hours, " Часов ", " Час ", " Часа "));

    if (minutes)
        AddOut(minutes, "м ", GetruRUTimeEnding(minutes, " Минут ", " Минуту ", " Минуты "));

    if (secs)
        AddOut(secs, "с ", GetruRUTimeEnding(secs, " Секунд ", " Секунду ", " Секунды "));

    if (millisecs)
        AddOut(millisecs, "мс ", GetruRUTimeEnding(millisecs, " Милисекунд ", " Милисекунду ", " Милисекунды "));

    if (microsecs)
        AddOut(microsecs, "мкс ", GetruRUTimeEnding(microsecs, " Микросекунд ", " Микросекунду ", " Микросекунды "));

    return std::string{ Warhead::String::TrimRight(out) };
}