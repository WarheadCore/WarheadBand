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

#ifndef _HARDMODE_H_
#define _HARDMODE_H_

#include "Define.h"

class HardModeMgr
{
public:
    static HardModeMgr* instance();

    void Initialize();
    void LoadConfig(bool reload);
    [[nodiscard]] bool CanResurrect() const;

private:
    bool _isEnable{ false };

    HardModeMgr() = default;
    ~HardModeMgr() = default;

    HardModeMgr(HardModeMgr const&) = delete;
    HardModeMgr(HardModeMgr&&) = delete;
    HardModeMgr& operator=(HardModeMgr const&) = delete;
    HardModeMgr& operator=(HardModeMgr&&) = delete;
};

#define sHardModeMgr HardModeMgr::instance()

#endif // _HARDMODE_H_