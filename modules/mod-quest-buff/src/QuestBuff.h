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

#ifndef _QUEST_BUFF_H_
#define _QUEST_BUFF_H_

#include "Define.h"
#include <unordered_map>

class Player;

struct QuestBuffStruct
{
    uint32 SpellID{};
    uint32 Rank{};
    uint32 Category{};
};

class QuestBuffMgr
{
public:
    QuestBuffMgr() = default;
    ~QuestBuffMgr() = default;

    static QuestBuffMgr* instance();

    void Initialize();
    void LoadConfig(bool reload);
    void OnPlayerLogin(Player* player);

private:
    uint32 GetHighRankByCategory(Player* player, uint32 category);
    uint32 GetHighSpellByCategory(Player* player, uint32 category);

    bool _isEnable{ false };
    std::unordered_map<uint32/*quest id*/, QuestBuffStruct> _buffs;

    QuestBuffMgr(QuestBuffMgr const&) = delete;
    QuestBuffMgr(QuestBuffMgr&&) = delete;
    QuestBuffMgr& operator=(QuestBuffMgr const&) = delete;
    QuestBuffMgr& operator=(QuestBuffMgr&&) = delete;
};

#define sQuestBuffMgr QuestBuffMgr::instance()

#endif // _QUEST_BUFF_H_