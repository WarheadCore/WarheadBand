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

#include "TitleStats.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "Player.h"
#include "StopWatch.h"
#include "DatabaseEnv.h"
#include "Chat.h"

namespace
{
    void ChangeStats(Player* player, uint32 statType, uint32 val, bool isApply)
    {
        switch (statType)
        {
            case ITEM_MOD_MANA:
                player->HandleStatModifier(UNIT_MOD_MANA, BASE_VALUE, float(val), isApply);
                break;
            case ITEM_MOD_HEALTH:                           // modify HP
                player->HandleStatModifier(UNIT_MOD_HEALTH, BASE_VALUE, float(val), isApply);
                break;
            case ITEM_MOD_AGILITY:                          // modify agility
                player->HandleStatModifier(UNIT_MOD_STAT_AGILITY, BASE_VALUE, float(val), isApply);
                player->ApplyStatBuffMod(STAT_AGILITY, float(val), isApply);
                break;
            case ITEM_MOD_STRENGTH:                         //modify strength
                player->HandleStatModifier(UNIT_MOD_STAT_STRENGTH, BASE_VALUE, float(val), isApply);
                player->ApplyStatBuffMod(STAT_STRENGTH, float(val), isApply);
                break;
            case ITEM_MOD_INTELLECT:                        //modify intellect
                player->HandleStatModifier(UNIT_MOD_STAT_INTELLECT, BASE_VALUE, float(val), isApply);
                player->ApplyStatBuffMod(STAT_INTELLECT, float(val), isApply);
                break;
            case ITEM_MOD_SPIRIT:                           //modify spirit
                player->HandleStatModifier(UNIT_MOD_STAT_SPIRIT, BASE_VALUE, float(val), isApply);
                player->ApplyStatBuffMod(STAT_SPIRIT, float(val), isApply);
                break;
            case ITEM_MOD_STAMINA:                          //modify stamina
                player->HandleStatModifier(UNIT_MOD_STAT_STAMINA, BASE_VALUE, float(val), isApply);
                player->ApplyStatBuffMod(STAT_STAMINA, float(val), isApply);
                break;
            case ITEM_MOD_DEFENSE_SKILL_RATING:
                player->ApplyRatingMod(CR_DEFENSE_SKILL, int32(val), isApply);
                break;
            case ITEM_MOD_DODGE_RATING:
                player->ApplyRatingMod(CR_DODGE, int32(val), isApply);
                break;
            case ITEM_MOD_PARRY_RATING:
                player->ApplyRatingMod(CR_PARRY, int32(val), isApply);
                break;
            case ITEM_MOD_BLOCK_RATING:
                player->ApplyRatingMod(CR_BLOCK, int32(val), isApply);
                break;
            case ITEM_MOD_HIT_MELEE_RATING:
                player->ApplyRatingMod(CR_HIT_MELEE, int32(val), isApply);
                break;
            case ITEM_MOD_HIT_RANGED_RATING:
                player->ApplyRatingMod(CR_HIT_RANGED, int32(val), isApply);
                break;
            case ITEM_MOD_HIT_SPELL_RATING:
                player->ApplyRatingMod(CR_HIT_SPELL, int32(val), isApply);
                break;
            case ITEM_MOD_CRIT_MELEE_RATING:
                player->ApplyRatingMod(CR_CRIT_MELEE, int32(val), isApply);
                break;
            case ITEM_MOD_CRIT_RANGED_RATING:
                player->ApplyRatingMod(CR_CRIT_RANGED, int32(val), isApply);
                break;
            case ITEM_MOD_CRIT_SPELL_RATING:
                player->ApplyRatingMod(CR_CRIT_SPELL, int32(val), isApply);
                break;
            case ITEM_MOD_HIT_TAKEN_MELEE_RATING:
                player->ApplyRatingMod(CR_HIT_TAKEN_MELEE, int32(val), isApply);
                break;
            case ITEM_MOD_HIT_TAKEN_RANGED_RATING:
                player->ApplyRatingMod(CR_HIT_TAKEN_RANGED, int32(val), isApply);
                break;
            case ITEM_MOD_HIT_TAKEN_SPELL_RATING:
                player->ApplyRatingMod(CR_HIT_TAKEN_SPELL, int32(val), isApply);
                break;
            case ITEM_MOD_CRIT_TAKEN_MELEE_RATING:
                player->ApplyRatingMod(CR_CRIT_TAKEN_MELEE, int32(val), isApply);
                break;
            case ITEM_MOD_CRIT_TAKEN_RANGED_RATING:
                player->ApplyRatingMod(CR_CRIT_TAKEN_RANGED, int32(val), isApply);
                break;
            case ITEM_MOD_CRIT_TAKEN_SPELL_RATING:
                player->ApplyRatingMod(CR_CRIT_TAKEN_SPELL, int32(val), isApply);
                break;
            case ITEM_MOD_HASTE_MELEE_RATING:
                player->ApplyRatingMod(CR_HASTE_MELEE, int32(val), isApply);
                break;
            case ITEM_MOD_HASTE_RANGED_RATING:
                player->ApplyRatingMod(CR_HASTE_RANGED, int32(val), isApply);
                break;
            case ITEM_MOD_HASTE_SPELL_RATING:
                player->ApplyRatingMod(CR_HASTE_SPELL, int32(val), isApply);
                break;
            case ITEM_MOD_HIT_RATING:
                player->ApplyRatingMod(CR_HIT_MELEE, int32(val), isApply);
                player->ApplyRatingMod(CR_HIT_RANGED, int32(val), isApply);
                player->ApplyRatingMod(CR_HIT_SPELL, int32(val), isApply);
                break;
            case ITEM_MOD_CRIT_RATING:
                player->ApplyRatingMod(CR_CRIT_MELEE, int32(val), isApply);
                player->ApplyRatingMod(CR_CRIT_RANGED, int32(val), isApply);
                player->ApplyRatingMod(CR_CRIT_SPELL, int32(val), isApply);
                break;
            case ITEM_MOD_HIT_TAKEN_RATING:
                player->ApplyRatingMod(CR_HIT_TAKEN_MELEE, int32(val), isApply);
                player->ApplyRatingMod(CR_HIT_TAKEN_RANGED, int32(val), isApply);
                player->ApplyRatingMod(CR_HIT_TAKEN_SPELL, int32(val), isApply);
                break;
            case ITEM_MOD_CRIT_TAKEN_RATING:
            case ITEM_MOD_RESILIENCE_RATING:
                player->ApplyRatingMod(CR_CRIT_TAKEN_MELEE, int32(val), isApply);
                player->ApplyRatingMod(CR_CRIT_TAKEN_RANGED, int32(val), isApply);
                player->ApplyRatingMod(CR_CRIT_TAKEN_SPELL, int32(val), isApply);
                break;
            case ITEM_MOD_HASTE_RATING:
                player->ApplyRatingMod(CR_HASTE_MELEE, int32(val), isApply);
                player->ApplyRatingMod(CR_HASTE_RANGED, int32(val), isApply);
                player->ApplyRatingMod(CR_HASTE_SPELL, int32(val), isApply);
                break;
            case ITEM_MOD_EXPERTISE_RATING:
                player->ApplyRatingMod(CR_EXPERTISE, int32(val), isApply);
                break;
            case ITEM_MOD_ATTACK_POWER:
                player->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(val), isApply);
                player->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(val), isApply);
                break;
            case ITEM_MOD_RANGED_ATTACK_POWER:
                player->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(val), isApply);
                break;
            case ITEM_MOD_MANA_REGENERATION:
                player->ApplyManaRegenBonus(int32(val), isApply);
                break;
            case ITEM_MOD_ARMOR_PENETRATION_RATING:
                player->ApplyRatingMod(CR_ARMOR_PENETRATION, int32(val), isApply);
                break;
            case ITEM_MOD_SPELL_POWER:
                player->ApplySpellPowerBonus(int32(val), isApply);
                break;
            case ITEM_MOD_HEALTH_REGEN:
                player->ApplyHealthRegenBonus(int32(val), isApply);
                break;
            case ITEM_MOD_SPELL_PENETRATION:
                player->ApplySpellPenetrationBonus(int32(val), isApply);
                break;
            case ITEM_MOD_BLOCK_VALUE:
                player->HandleBaseModValue(SHIELD_BLOCK_VALUE, FLAT_MOD, float(val), isApply);
                break;
                // deprecated item mods
            case ITEM_MOD_SPELL_HEALING_DONE:
            case ITEM_MOD_SPELL_DAMAGE_DONE:
            default:
                break;
        }
    }
}

TitleStatsMgr* TitleStatsMgr::instance()
{
    static TitleStatsMgr instance;
    return &instance;
}

void TitleStatsMgr::LoadConfig(bool /*reload*/)
{
    _isEnable = MOD_CONF_GET_BOOL("TitleStats.Enable");
}

void TitleStatsMgr::Initialize()
{
    if (!_isEnable)
        return;

    LoadDataFromDB();
}

void TitleStatsMgr::LoadDataFromDB()
{
    LOG_INFO("server.loading", "Loading warhead title stats...");

    StopWatch sw;
    _titleStats.clear();

    QueryResult result = WorldDatabase.Query("SELECT `ID`, "
                                             "`StatType1`, `StatValue1`, "
                                             "`StatType2`, `StatValue2`, "
                                             "`StatType3`, `StatValue3`, "
                                             "`StatType4`, `StatValue4`, "
                                             "`StatType5`, `StatValue5`, "
                                             "`StatType6`, `StatValue6`, "
                                             "`StatType7`, `StatValue7`, "
                                             "`StatType8`, `StatValue8`, "
                                             "`StatType9`, `StatValue9`, "
                                             "`StatType10`, `StatValue10` "
                                             "FROM `wh_title_stats`");
    if (!result)
    {
        LOG_INFO("module", ">> Loaded 0 warhead title stats. DB table `wh_title_stats` is empty.");
        LOG_WARN("module", "> Disable this module");
        LOG_INFO("module", "");
        _isEnable = false;
        return;
    }

    for (auto const& fields : *result)
    {
        auto titleID{ fields[0].Get<uint32>() };

        if (GetTitleStats(titleID))
        {
            LOG_ERROR("module", "> Stats for title {} is exist", titleID);
            continue;
        }

        TitleStats titleStats{};
        titleStats.TitleID = titleID;

        for (uint8 i = 0; i < 10; i++)
        {
            auto statType{ fields[1 + i * 2].Get<uint8>() };
            if (!statType)
                continue;

            titleStats.TitleStats.emplace_back(statType, fields[2 + i * 2].Get<int32>());
        }

        _titleStats.emplace(titleStats.TitleID, std::move(titleStats));
    }

    if (_titleStats.empty())
    {
        LOG_INFO("server.loading", ">> Loaded 0 warhead title stats in {}. Disable module", sw);
        LOG_INFO("server.loading", "");
        _isEnable = false;
        return;
    }

    LOG_INFO("server.loading", ">> Loaded {} warhead title stats in {}", _titleStats.size(), sw);
    LOG_INFO("server.loading", "");
}

void TitleStatsMgr::OnChangeTitle(Player* player, int32 title)
{
    // Remove stats
    ApplyStats(player, player->GetTitle(), false);

    // Apply stats
    ApplyStats(player, title, true);

    // Set full health for player after change stats
    player->SetFullHealth();
}

void TitleStatsMgr::OnLogin(Player* player)
{
    // Apply stats
    ApplyStats(player, player->GetTitle(), true);

    // Set full health for player after change stats
    player->SetFullHealth();
}

TitleStats const* TitleStatsMgr::GetTitleStats(uint32 title)
{
    return Warhead::Containers::MapGetValuePtr(_titleStats, title);
}

void TitleStatsMgr::ApplyStats(Player* player, uint32 title, bool isApply)
{
    auto titleStats = GetTitleStats(title);
    if (!titleStats)
        return;

    if (isApply)
    {
        ChatHandler handler{ player->GetSession() };
        handler.PSendSysMessage("Вы поменяли своё звание, ваши характеристики изменились");
    }

    for (auto const& [statType, statValue] : titleStats->TitleStats)
        ChangeStats(player, statType, statValue, isApply);
}