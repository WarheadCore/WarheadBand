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

#ifndef _GAME_LOCALE_H_
#define _GAME_LOCALE_H_

#include "Common.h"
#include "Player.h"
#include <vector>

struct WarheadString
{
    std::vector<std::string> Content;
};

// Default locales
struct AchievementRewardLocale
{
    std::vector<std::string> Subject;
    std::vector<std::string> Text;
};

struct CreatureLocale
{
    std::vector<std::string> Name;
    std::vector<std::string> Title;
};

struct GameObjectLocale
{
    std::vector<std::string> Name;
    std::vector<std::string> CastBarCaption;
};

struct GossipMenuItemsLocale
{
    std::vector<std::string> OptionText;
    std::vector<std::string> BoxText;
};

struct ItemLocale
{
    std::vector<std::string> Name;
    std::vector<std::string> Description;
};

struct ItemSetNameLocale
{
    std::vector<std::string> Name;
};

struct NpcTextLocale
{
    NpcTextLocale() { Text_0.resize(8); Text_1.resize(8); }

    std::vector<std::vector<std::string>> Text_0;
    std::vector<std::vector<std::string>> Text_1;
};

struct PageTextLocale
{
    std::vector<std::string> Text;
};

struct PointOfInterestLocale
{
    std::vector<std::string> Name;
};

struct QuestLocale
{
    QuestLocale() { ObjectiveText.resize(4); }

    std::vector<std::string> Title;
    std::vector<std::string> Details;
    std::vector<std::string> Objectives;
    std::vector<std::string> OfferRewardText;
    std::vector<std::string> RequestItemsText;
    std::vector<std::string> AreaDescription;
    std::vector<std::string> CompletedText;
    std::vector<std::vector<std::string>> ObjectiveText;
};

struct QuestRequestItemsLocale
{
    std::vector<std::string> CompletionText;
};

struct QuestOfferRewardLocale
{
    std::vector<std::string> RewardText;
};

struct QuestGreetingLocale
{
    std::vector<std::string> greeting;
};

struct BroadcastText
{
    BroadcastText() : Id(0), LanguageID(0), EmoteId1(0), EmoteId2(0), EmoteId3(0),
        EmoteDelay1(0), EmoteDelay2(0), EmoteDelay3(0), SoundEntriesID(0), EmotesID(0), Flags(0)
    {
        Text.resize(DEFAULT_LOCALE + 1);
        Text1.resize(DEFAULT_LOCALE + 1);
    }

    uint32 Id;
    uint32 LanguageID;
    std::vector<std::string> Text;
    std::vector<std::string> Text1;
    uint32 EmoteId1;
    uint32 EmoteId2;
    uint32 EmoteId3;
    uint32 EmoteDelay1;
    uint32 EmoteDelay2;
    uint32 EmoteDelay3;
    uint32 SoundEntriesID;
    uint32 EmotesID;
    uint32 Flags;
    // uint32 VerifiedBuild;

    std::string const& GetText(LocaleConstant locale = DEFAULT_LOCALE, uint8 gender = GENDER_MALE, bool forceGender = false) const
    {
        if ((gender == GENDER_FEMALE || gender == GENDER_NONE) && (forceGender || !Text1[DEFAULT_LOCALE].empty()))
        {
            if (Text1.size() > size_t(locale) && !Text1[locale].empty())
                return Text1[locale];

            return Text1[DEFAULT_LOCALE];
        }
        // else if (gender == GENDER_MALE)
        {
            if (Text.size() > size_t(locale) && !Text[locale].empty())
                return Text[locale];

            return Text[DEFAULT_LOCALE];
        }
    }
};

// New strings and locales
struct RaceString
{
    RaceString()
    {
        NameMale.resize(DEFAULT_LOCALE + 1);
        NameFemale.resize(DEFAULT_LOCALE + 1);
    }

    std::vector<std::string> NameMale;
    std::vector<std::string> NameFemale;

    std::string const& GetText(LocaleConstant locale = DEFAULT_LOCALE, uint8 gender = GENDER_MALE) const
    {
        if (gender == GENDER_FEMALE)
        {
            if (NameFemale.size() > size_t(locale) && !NameFemale[locale].empty())
                return NameFemale[locale];

            if (NameMale.size() > size_t(locale) && !NameMale[locale].empty())
                return NameMale[locale];

            return NameFemale[DEFAULT_LOCALE];
        }

        if (NameMale.size() > size_t(locale) && !NameMale[locale].empty())
            return NameMale[locale];

        return NameMale[DEFAULT_LOCALE];
    }
};

struct ClassString
{
    ClassString()
    {
        NameMale.resize(DEFAULT_LOCALE + 1);
        NameFemale.resize(DEFAULT_LOCALE + 1);
    }

    std::vector<std::string> NameMale;
    std::vector<std::string> NameFemale;

    std::string const& GetText(LocaleConstant locale = DEFAULT_LOCALE, uint8 gender = GENDER_MALE) const
    {
        if (gender == GENDER_FEMALE)
        {
            if (NameFemale.size() > size_t(locale) && !NameFemale[locale].empty())
                return NameFemale[locale];

            if (NameMale.size() > size_t(locale) && !NameMale[locale].empty())
                return NameMale[locale];

            return NameFemale[DEFAULT_LOCALE];
        }

        if (NameMale.size() > size_t(locale) && !NameMale[locale].empty())
            return NameMale[locale];

        return NameMale[DEFAULT_LOCALE];
    }
};

class WH_GAME_API GameLocale
{
private:
    GameLocale() = default;
    ~GameLocale() = default;

public:
    static GameLocale* instance();

    void LoadAllLocales();
    bool LoadWarheadStrings();

    inline std::string_view GetLocaleString(std::vector<std::string> const& data, size_t locale)
    {
        if (locale < data.size())
            return data[locale];
        else
            return {};
    }

    inline void GetLocaleString(const std::vector<std::string>& data, int loc_idx, std::string& value)
    {
        if (data.size() > size_t(loc_idx) && !data[loc_idx].empty())
            value = data[loc_idx];
    }

    WarheadString const* GetWarheadString(uint32 entry) const;
    char const* GetWarheadString(uint32 entry, LocaleConstant locale) const;
    char const* GetWarheadStringForDBCLocale(uint32 entry) const { return GetWarheadString(entry, DBCLocaleIndex); }

    LocaleConstant GetDBCLocaleIndex() const { return DBCLocaleIndex; }
    void SetDBCLocaleIndex(LocaleConstant locale) { DBCLocaleIndex = locale; }

    void LoadAchievementRewardLocales();
    void LoadBroadcastTexts();
    void LoadBroadcastTextLocales();
    void LoadCreatureLocales();
    void LoadGameObjectLocales();
    void LoadItemLocales();
    void LoadItemSetNameLocales();
    void LoadQuestLocales();
    void LoadNpcTextLocales();
    void LoadQuestOfferRewardLocale();
    void LoadQuestRequestItemsLocale();
    void LoadPageTextLocales();
    void LoadGossipMenuItemsLocales();
    void LoadPointOfInterestLocales();
    void LoadQuestGreetingLocales();

    AchievementRewardLocale const* GetAchievementRewardLocale(uint32 entry) const;
    BroadcastText const* GetBroadcastText(uint32 id) const;
    CreatureLocale const* GetCreatureLocale(uint32 entry) const;
    GameObjectLocale const* GetGameObjectLocale(uint32 entry) const;
    GossipMenuItemsLocale const* GetGossipMenuItemsLocale(uint32 entry) const;
    ItemLocale const* GetItemLocale(uint32 entry) const;
    ItemSetNameLocale const* GetItemSetNameLocale(uint32 entry) const;
    NpcTextLocale const* GetNpcTextLocale(uint32 entry) const;
    PageTextLocale const* GetPageTextLocale(uint32 entry) const;
    PointOfInterestLocale const* GetPointOfInterestLocale(uint32 entry) const;
    QuestLocale const* GetQuestLocale(uint32 entry) const;
    QuestOfferRewardLocale const* GetQuestOfferRewardLocale(uint32 entry) const;
    QuestRequestItemsLocale const* GetQuestRequestItemsLocale(uint32 entry) const;
    QuestGreetingLocale const* GetQuestGreetingLocale(uint32 id) const;

    //
    std::string const GetItemNameLocale(uint32 itemID, int8 index_loc = DEFAULT_LOCALE);
    std::string const GetItemLink(uint32 itemID, int8 index_loc = DEFAULT_LOCALE);
    std::string const GetSpellLink(uint32 spellID, int8 index_loc = DEFAULT_LOCALE);
    std::string const GetSpellNamelocale(uint32 spellID, int8 index_loc = DEFAULT_LOCALE);

    // New strings and locales
    void LoadRaceStrings();
    void LoadClassStrings();

    RaceString const* GetRaseString(uint32 id) const;
    ClassString const* GetClassString(uint32 id) const;

private:
    using WarheadStringContainer = std::unordered_map<uint32, WarheadString>;

    WarheadStringContainer _warheadStringStore;
    LocaleConstant DBCLocaleIndex = LOCALE_enUS;

    using AchievementRewardLocales = std::unordered_map<uint32, AchievementRewardLocale>;
    using BroadcastTextContainer = std::unordered_map<uint32, BroadcastText>;
    using CreatureLocaleContainer = std::unordered_map<uint32, CreatureLocale>;
    using GameObjectLocaleContainer = std::unordered_map<uint32, GameObjectLocale>;
    using GossipMenuItemsLocaleContainer = std::unordered_map<uint32, GossipMenuItemsLocale>;
    using ItemLocaleContainer = std::unordered_map<uint32, ItemLocale>;
    using ItemSetNameLocaleContainer = std::unordered_map<uint32, ItemSetNameLocale>;
    using NpcTextLocaleContainer = std::unordered_map<uint32, NpcTextLocale>;
    using PageTextLocaleContainer = std::unordered_map<uint32, PageTextLocale>;
    using PointOfInterestLocaleContainer = std::unordered_map<uint32, PointOfInterestLocale>;
    using QuestLocaleContainer = std::unordered_map<uint32, QuestLocale>;
    using QuestGreetingLocaleContainer = std::unordered_map<uint32, QuestGreetingLocale>;
    using QuestOfferRewardLocaleContainer = std::unordered_map<uint32, QuestOfferRewardLocale>;
    using QuestRequestItemsLocaleContainer = std::unordered_map<uint32, QuestRequestItemsLocale>;

    AchievementRewardLocales _achievementRewardLocales;
    BroadcastTextContainer _broadcastTextStore;
    CreatureLocaleContainer _creatureLocaleStore;
    GameObjectLocaleContainer _gameObjectLocaleStore;
    GossipMenuItemsLocaleContainer _gossipMenuItemsLocaleStore;
    ItemLocaleContainer _itemLocaleStore;
    ItemSetNameLocaleContainer _itemSetNameLocaleStore;
    NpcTextLocaleContainer _npcTextLocaleStore;
    PageTextLocaleContainer _pageTextLocaleStore;
    PointOfInterestLocaleContainer _pointOfInterestLocaleStore;
    QuestLocaleContainer _questLocaleStore;
    QuestGreetingLocaleContainer _questGreetingLocaleStore;
    QuestOfferRewardLocaleContainer _questOfferRewardLocaleStore;
    QuestRequestItemsLocaleContainer _questRequestItemsLocaleStore;

    // New strings and locales
    using RaceStringContainer = std::unordered_map<uint32, RaceString>;
    using ClassStringContainer = std::unordered_map<uint32, ClassString>;

    RaceStringContainer _raceStringStore;
    ClassStringContainer _classStringStore;
};

#define sGameLocale GameLocale::instance()

#endif // _GAME_LOCALE_H_
