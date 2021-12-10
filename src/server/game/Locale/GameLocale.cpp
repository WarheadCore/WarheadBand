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
#include "World.h"

GameLocale* GameLocale::instance()
{
    static GameLocale instance;
    return &instance;
}

void GameLocale::LoadAllLocales()
{
    // Get once for all the locale index of DBC language (console/broadcasts)
    SetDBCLocaleIndex(sWorld->GetDefaultDbcLocale());

    uint32 oldMSTime = getMSTime();

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

    // Load modules strings
    sModuleLocale->Init();

    LOG_INFO("server.loading", ">> Localization strings loaded in {} ms", GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", "");
}

bool GameLocale::LoadWarheadStrings()
{
    uint32 oldMSTime = getMSTime();

    _warheadStringStore.clear(); // for reload case

    QueryResult result = WorldDatabase.PQuery("SELECT entry, content_default, locale_koKR, locale_frFR, locale_deDE, locale_zhCN, locale_zhTW, locale_esES, locale_esMX, locale_ruRU FROM acore_string");
    if (!result)
    {
        LOG_WARN("sql.sql", ">> Loaded 0 warhead strings. DB table `warhead_strings` is empty.");
        LOG_WARN("sql.sql", "");
        return false;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        auto& data = _warheadStringStore[entry];

        data.Content.resize(DEFAULT_LOCALE + 1);

        for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
            Warhead::Locale::AddLocaleString(fields[i + 1].GetString(), LocaleConstant(i), data.Content);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} warhead strings in {} ms", static_cast<uint32>(_warheadStringStore.size()), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", "");

    return true;
}

char const* GameLocale::GetWarheadString(uint32 entry, LocaleConstant locale) const
{
    if (auto as = GetWarheadString(entry))
    {
        if (as->Content.size() > size_t(locale) && !as->Content[locale].empty())
            return as->Content[locale].c_str();

        return as->Content[DEFAULT_LOCALE].c_str();
    }

    LOG_ERROR("sql.sql", "Warhead string entry {} not found in DB.", entry);

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
    uint32 oldMSTime = getMSTime();

    _achievementRewardLocales.clear();                       // need for reload case

    //                                               0   1       2        3
    QueryResult result = WorldDatabase.Query("SELECT ID, Locale, Subject, Text FROM achievement_reward_locale");

    if (!result)
    {
        LOG_WARN("sql.sql", ">> Loaded 0 achievement reward locale strings. DB table `achievement_reward_locale` is empty");
        LOG_WARN("sql.sql", "");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 ID = fields[0].GetUInt32();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        AchievementRewardLocale& data = _achievementRewardLocales[ID];

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, data.Subject);
        Warhead::Locale::AddLocaleString(fields[3].GetString(), locale, data.Text);

    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Achievement Reward Locale strings in {} ms", static_cast<uint32>(_achievementRewardLocales.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadBroadcastTexts()
{
    uint32 oldMSTime = getMSTime();

    _broadcastTextStore.clear(); // for reload case

    //                                               0   1           2     3      4         5         6         7            8            9            10              11        12
    QueryResult result = WorldDatabase.Query("SELECT ID, LanguageID, Text, Text1, EmoteID1, EmoteID2, EmoteID3, EmoteDelay1, EmoteDelay2, EmoteDelay3, SoundEntriesID, EmotesID, Flags FROM broadcast_text");
    if (!result)
    {
        LOG_WARN("sql.sql", ">> Loaded 0 broadcast texts. DB table `broadcast_text` is empty.");
        LOG_WARN("sql.sql", "");
        return;
    }

    _broadcastTextStore.rehash(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();

        BroadcastText bct;

        bct.Id = fields[0].GetUInt32();
        bct.LanguageID = fields[1].GetUInt32();
        bct.Text[DEFAULT_LOCALE] = fields[2].GetString();
        bct.Text1[DEFAULT_LOCALE] = fields[3].GetString();
        bct.EmoteId1 = fields[4].GetUInt32();
        bct.EmoteId2 = fields[5].GetUInt32();
        bct.EmoteId3 = fields[6].GetUInt32();
        bct.EmoteDelay1 = fields[7].GetUInt32();
        bct.EmoteDelay2 = fields[8].GetUInt32();
        bct.EmoteDelay3 = fields[9].GetUInt32();
        bct.SoundEntriesID = fields[10].GetUInt32();
        bct.EmotesID = fields[11].GetUInt32();
        bct.Flags = fields[12].GetUInt32();

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
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} broadcast texts in {} ms", static_cast<uint32>(_broadcastTextStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadBroadcastTextLocales()
{
    uint32 oldMSTime = getMSTime();

    //                                               0   1       2     3
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Text, Text1 FROM broadcast_text_locale");

    if (!result)
    {
        LOG_WARN("sql.sql", ">> Loaded 0 broadcast text locales. DB table `broadcast_text_locale` is empty.");
        LOG_WARN("sql.sql", "");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetUInt32();

        auto const& bct = _broadcastTextStore.find(id);
        if (bct == _broadcastTextStore.end())
        {
            LOG_ERROR("sql.sql", "BroadcastText (Id: {}) in table `broadcast_text_locale` does not exist. Skipped!", id);
            continue;
        }

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, bct->second.Text);
        Warhead::Locale::AddLocaleString(fields[3].GetString(), locale, bct->second.Text1);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} broadcast text locales in {} ms", static_cast<uint32>(_broadcastTextStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadCreatureLocales()
{
    uint32 oldMSTime = getMSTime();

    _creatureLocaleStore.clear();                              // need for reload case

    //                                               0      1       2     3
    QueryResult result = WorldDatabase.Query("SELECT entry, locale, Name, Title FROM creature_template_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetUInt32();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        CreatureLocale& data = _creatureLocaleStore[id];

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, data.Name);
        Warhead::Locale::AddLocaleString(fields[3].GetString(), locale, data.Title);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} creature Locale strings in {} ms", static_cast<uint32>(_creatureLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadGossipMenuItemsLocales()
{
    uint32 oldMSTime = getMSTime();

    _gossipMenuItemsLocaleStore.clear();                              // need for reload case

    //                                               0       1            2       3           4
    QueryResult result = WorldDatabase.Query("SELECT MenuID, OptionID, Locale, OptionText, BoxText FROM gossip_menu_option_locale");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint16 menuId = fields[0].GetUInt16();
        uint16 optionId = fields[1].GetUInt16();

        LocaleConstant locale = GetLocaleByName(fields[2].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        GossipMenuItemsLocale& data = _gossipMenuItemsLocaleStore[MAKE_PAIR32(menuId, optionId)];

        Warhead::Locale::AddLocaleString(fields[3].GetString(), locale, data.OptionText);
        Warhead::Locale::AddLocaleString(fields[4].GetString(), locale, data.BoxText);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Gossip Menu Option Locale strings in {} ms", static_cast<uint32>(_gossipMenuItemsLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadGameObjectLocales()
{
    uint32 oldMSTime = getMSTime();

    _gameObjectLocaleStore.clear(); // need for reload case

    //                                               0      1       2     3
    QueryResult result = WorldDatabase.Query("SELECT entry, locale, name, castBarCaption FROM gameobject_template_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetUInt32();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        GameObjectLocale& data = _gameObjectLocaleStore[id];

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, data.Name);
        Warhead::Locale::AddLocaleString(fields[3].GetString(), locale, data.CastBarCaption);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Gameobject Locale strings in {} ms", static_cast<uint32>(_gameObjectLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadItemLocales()
{
    uint32 oldMSTime = getMSTime();

    _itemLocaleStore.clear();                                 // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Name, Description FROM item_template_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetUInt32();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        ItemLocale& data = _itemLocaleStore[id];

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, data.Name);
        Warhead::Locale::AddLocaleString(fields[3].GetString(), locale, data.Description);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Item Locale strings in {} ms", static_cast<uint32>(_itemLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadItemSetNameLocales()
{
    uint32 oldMSTime = getMSTime();

    _itemSetNameLocaleStore.clear();                                 // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Name FROM item_set_names_locale");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetUInt32();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        ItemSetNameLocale& data = _itemSetNameLocaleStore[id];

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, data.Name);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Item Set Name Locale strings in {} ms", static_cast<uint32>(_itemSetNameLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadNpcTextLocales()
{
    uint32 oldMSTime = getMSTime();

    _npcTextLocaleStore.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID, Locale, "
                         //   2        3        4        5        6        7        8        9        10       11       12       13       14       15       16       17
                         "Text0_0, Text0_1, Text1_0, Text1_1, Text2_0, Text2_1, Text3_0, Text3_1, Text4_0, Text4_1, Text5_0, Text5_1, Text6_0, Text6_1, Text7_0, Text7_1 "
                         "FROM npc_text_locale");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetUInt32();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        NpcTextLocale& data = _npcTextLocaleStore[id];

        for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
        {
            Warhead::Locale::AddLocaleString(fields[2 + i * 2].GetString(), locale, data.Text_0[i]);
            Warhead::Locale::AddLocaleString(fields[3 + i * 2].GetString(), locale, data.Text_1[i]);
        }
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Npc Text Locale strings in {} ms", static_cast<uint32>(_npcTextLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadPageTextLocales()
{
    uint32 oldMSTime = getMSTime();

    _pageTextLocaleStore.clear();                             // need for reload case

    //                                               0   1       2
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Text FROM page_text_locale");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetUInt32();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        PageTextLocale& data = _pageTextLocaleStore[id];

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, data.Text);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Page Text Locale strings in {} ms", static_cast<uint32>(_pageTextLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadPointOfInterestLocales()
{
    uint32 oldMSTime = getMSTime();

    _pointOfInterestLocaleStore.clear();                              // need for reload case

    //                                               0   1       2
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Name FROM points_of_interest_locale");

    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetUInt32();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        PointOfInterestLocale& data = _pointOfInterestLocaleStore[id];

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, data.Name);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Points Of Interest Locale strings in {} ms", static_cast<uint32>(_pointOfInterestLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadQuestLocales()
{
    uint32 oldMSTime = getMSTime();

    _questLocaleStore.clear();                                // need for reload case

    //                                               0   1       2      3        4           5        6              7               8               9               10
    QueryResult result = WorldDatabase.Query("SELECT ID, locale, Title, Details, Objectives, EndText, CompletedText, ObjectiveText1, ObjectiveText2, ObjectiveText3, ObjectiveText4 FROM quest_template_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetUInt32();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        QuestLocale& data = _questLocaleStore[id];

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, data.Title);
        Warhead::Locale::AddLocaleString(fields[3].GetString(), locale, data.Details);
        Warhead::Locale::AddLocaleString(fields[4].GetString(), locale, data.Objectives);
        Warhead::Locale::AddLocaleString(fields[5].GetString(), locale, data.AreaDescription);
        Warhead::Locale::AddLocaleString(fields[6].GetString(), locale, data.CompletedText);

        for (uint8 i = 0; i < 4; ++i)
            Warhead::Locale::AddLocaleString(fields[i + 7].GetString(), locale, data.ObjectiveText[i]);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest Locale strings in {} ms", static_cast<uint32>(_questLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadQuestOfferRewardLocale()
{
    uint32 oldMSTime = getMSTime();

    _questOfferRewardLocaleStore.clear(); // need for reload case

    //                                               0     1          2
    QueryResult result = WorldDatabase.Query("SELECT Id, locale, RewardText FROM quest_offer_reward_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetUInt32();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        QuestOfferRewardLocale& data = _questOfferRewardLocaleStore[id];

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, data.RewardText);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest Offer Reward locale strings in {} ms", static_cast<uint32>(_questOfferRewardLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadQuestRequestItemsLocale()
{
    uint32 oldMSTime = getMSTime();

    _questRequestItemsLocaleStore.clear(); // need for reload case

    //                                               0     1          2
    QueryResult result = WorldDatabase.Query("SELECT Id, locale, CompletionText FROM quest_request_items_locale");
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetUInt32();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        QuestRequestItemsLocale& data = _questRequestItemsLocaleStore[id];

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, data.CompletionText);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest Request Items locale strings in {} ms", static_cast<uint32>(_questRequestItemsLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadChatCommandsLocales()
{
    uint32 oldMSTime = getMSTime();

    _chatCommandStringStore.clear(); // need for reload case

    //                                                     0       1        2
    QueryResult result = WorldDatabase.Query("SELECT `Command`, `Locale`, `Content` FROM commands_help_locale");
    if (!result)
    {
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        std::string commandName = fields[0].GetString();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        ChatCommandHelpLocale& data = _chatCommandStringStore[commandName];

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, data.Content);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Chat commands help strings locale in {} ms", _chatCommandStringStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadAutoBroadCastLocales()
{
    uint32 oldMSTime = getMSTime();

    _autobroadLocaleStore.clear(); // need for reload case

    //                                                 0          1       2
    QueryResult result = LoginDatabase.PQuery("SELECT `ID`, `Locale`, `Text` FROM `autobroadcast_locale` WHERE `RealmID` = -1 OR RealmID = '{}'", realm.Id.Realm);
    if (!result)
    {
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetUInt32();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        auto& data = _autobroadLocaleStore[id];

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, data.Text);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} autobroadcast text locale in {} ms", _chatCommandStringStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadQuestGreetingLocales()
{
    uint32 oldMSTime = getMSTime();

    _questGreetingLocaleStore.clear();                              // need for reload case

    //                                               0     1      2       3
    QueryResult result = WorldDatabase.Query("SELECT ID, Type, Locale, Greeting FROM quest_greeting_locale");
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 quest_greeting locales. DB table `quest_greeting_locale` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 id = fields[0].GetUInt32();
        uint8 type = fields[1].GetUInt8();

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

        LocaleConstant locale = GetLocaleByName(fields[2].GetString());
        if (locale == LOCALE_enUS)
            continue;

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        QuestGreetingLocale& data = _questGreetingLocaleStore[MAKE_PAIR32(id, type)];

        Warhead::Locale::AddLocaleString(fields[3].GetString(), locale, data.greeting);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} quest greeting locale strings in {} ms", static_cast<uint32>(_questGreetingLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
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
        //LOG_ERROR("sql.sql", "> Missing help text localisation for commnd '{}'", commandName);
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
    uint32 oldMSTime = getMSTime();

    _raceStringStore.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID, Locale, NameMale, NameFemale FROM `string_race`");
    if (!result)
    {
        LOG_WARN("sql.sql", "> DB table `string_race` is empty");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 ID = fields[0].GetUInt32();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        auto& data = _raceStringStore[ID];

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, data.NameMale);
        Warhead::Locale::AddLocaleString(fields[3].GetString(), locale, data.NameFemale);

    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Race strings in {} ms", static_cast<uint32>(_raceStringStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void GameLocale::LoadClassStrings()
{
    uint32 oldMSTime = getMSTime();

    _classStringStore.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID, Locale, NameMale, NameFemale FROM `string_class`");
    if (!result)
    {
        LOG_WARN("sql.sql", "> DB table `string_class` is empty");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 ID = fields[0].GetUInt32();

        LocaleConstant locale = GetLocaleByName(fields[1].GetString());

        /*if (CONF_GET_BOOL("Language.SupportOnlyDefault") && locale != GetDBCLocaleIndex())
            continue;*/

        auto& data = _classStringStore[ID];

        Warhead::Locale::AddLocaleString(fields[2].GetString(), locale, data.NameMale);
        Warhead::Locale::AddLocaleString(fields[3].GetString(), locale, data.NameFemale);

    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Class strings in {} ms", static_cast<uint32>(_classStringStore.size()), GetMSTimeDiffToNow(oldMSTime));
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

std::string const GameLocale::GetItemNameLocale(uint32 itemID, int8 index_loc /*= DEFAULT_LOCALE*/)
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

    return Warhead::StringFormat("|c%08x|Hitem:{}:0:0:0:0:0:0:0:0|h[{}]|h|r", color, itemID, name);
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

std::string const GameLocale::GetCreatureNamelocale(uint32 creatureEntry, int8 index_loc /*= DEFAULT_LOCALE*/)
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
