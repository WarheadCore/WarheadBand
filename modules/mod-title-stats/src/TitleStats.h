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

#ifndef _TITLE_STATS_H_
#define _TITLE_STATS_H_

#include "Define.h"
#include <unordered_map>
#include <vector>

class Player;

struct TitleStats
{
    uint32 TitleID{};
    std::vector<std::pair<uint32, uint32>> TitleStats;
};

class TitleStatsMgr
{
public:
    static TitleStatsMgr* instance();

    void Initialize();
    void LoadConfig(bool reload);

    // DB data
    void LoadDataFromDB();

    // Hooks
    void OnChangeTitle(Player* player, int32 title);
    void OnLogin(Player* player);

private:
    TitleStats const* GetTitleStats(uint32 title);
    void ApplyStats(Player* player, uint32 title, bool isApply);

    bool _isEnable{ false };

    std::unordered_map<uint32, TitleStats> _titleStats;

    TitleStatsMgr() = default;
    ~TitleStatsMgr() = default;

    TitleStatsMgr(TitleStatsMgr const&) = delete;
    TitleStatsMgr(TitleStatsMgr&&) = delete;
    TitleStatsMgr& operator=(TitleStatsMgr const&) = delete;
    TitleStatsMgr& operator=(TitleStatsMgr&&) = delete;
};

#define sTitleStatsMgr TitleStatsMgr::instance()

#endif // _TITLE_STATS_H_
