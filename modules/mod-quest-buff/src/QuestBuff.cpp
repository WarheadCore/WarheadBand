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

#include "QuestBuff.h"
#include "Log.h"
#include "ModulesConfig.h"
#include "DatabaseEnv.h"
#include "StopWatch.h"
#include "ObjectMgr.h"
#include "DBCStores.h"
#include "Player.h"

QuestBuffMgr* QuestBuffMgr::instance()
{
    static QuestBuffMgr instance;
    return &instance;
}

void QuestBuffMgr::LoadConfig(bool /*reload*/)
{
    _isEnable = MOD_CONF_GET_BOOL("QuestBuff.Enable");
}

void QuestBuffMgr::Initialize()
{
    if (!_isEnable)
        return;

    StopWatch sw;

    LOG_INFO("module.questbuff", "Loading quest buffs...");

    QueryResult result = WorldDatabase.Query("SELECT `QuestID`, `SpellID`, `SpellRank`, `Category` FROM `wh_quest_buff` ORDER BY `Category`, `SpellRank`");
    if (!result)
    {
        LOG_INFO("module.questbuff", ">> In DB table `quest_buff` not data. Loading canceled");
        LOG_INFO("module.questbuff", "");
        return;
    }

    for (auto const& row : *result)
    {
        uint32 questID = row[0].Get<uint32>();

        Quest const* quest = sObjectMgr->GetQuestTemplate(questID);
        if (!quest)
        {
            LOG_ERROR("module.questbuff", "QB: Not found quest with id: {}", questID);
            continue;
        }

        QuestBuffStruct data{};
        data.SpellID = row[1].Get<uint32>();
        data.Rank = row[2].Get<uint32>();
        data.Category = row[3].Get<uint32>();

        auto spell = sSpellStore.LookupEntry(data.SpellID);
        if (!spell)
        {
            LOG_ERROR("module.questbuff", "QB: Spell with number ({}) not found. Skip.", data.SpellID);
            continue;
        }

        _buffs.emplace(questID, data);
    }

    LOG_INFO("module.questbuff", ">> Loaded {} quest buffs in {}", _buffs.size(), sw);
    LOG_INFO("module.questbuff", "");
}

uint32 QuestBuffMgr::GetHighRankByCategory(Player* player, uint32 category)
{
    uint32 highRank{};

    for (auto const& [questId, buff] : _buffs)
    {
        if (buff.Category != category)
            continue;

        if (player->GetQuestStatus(questId) != QUEST_STATUS_REWARDED)
            continue;

        highRank++;
    }

    return highRank;
}

uint32 QuestBuffMgr::GetHighSpellByCategory(Player* player, uint32 category)
{
    if (!player)
        return 0;

    uint32 highSpellRank = GetHighRankByCategory(player, category);

    for (auto const& [questId, buff] : _buffs)
    {
        if (buff.Category != category)
            continue;

        if (player->GetQuestStatus(questId) != QUEST_STATUS_REWARDED)
            continue;

        if (buff.Rank != highSpellRank)
            continue;

        return buff.SpellID;
    }

    return 0;
}

void QuestBuffMgr::OnPlayerLogin(Player* player)
{
    if (!_isEnable || !player)
        return;

    for (uint8 i = 1; i < 4; i++)
        if (auto spellId = GetHighSpellByCategory(player, i))
            player->CastSpell(player, spellId, true);
}