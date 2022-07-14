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

#ifndef _STAT_CONTROL_H_
#define _STAT_CONTROL_H_

#include "Define.h"
#include <unordered_map>

enum class StatControlType : uint8
{
    AttackPowerFromAgility,
    DoodgeFromAgility,
    ArmorFromAgility,
    CritFromAgility,
    CritFromIntellect,
    ManaFromIntellect,
    BlockFromStrenght,

    Max
};

class StatControlMgr
{
public:
    static StatControlMgr* instance();

    void Initialize();
    void LoadConfig(bool reload);

    // Hooks
    void CorrectStat(float& value, StatControlType type);

private:
    void LoadDBData();
    float const* GetMultiplier(StatControlType type);

    bool _isEnable{ false };
    std::unordered_map<uint8, float> _store;
};

#define sStatControlMgr StatControlMgr::instance()

#endif /* _STAT_CONTROL_H_ */
