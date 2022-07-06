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

#ifndef _REFORGE_H_
#define _REFORGE_H_

#include "Define.h"

class ReforgeMgr
{
    ReforgeMgr() = default;
    ~ReforgeMgr() = default;

    ReforgeMgr(ReforgeMgr const&) = delete;
    ReforgeMgr(ReforgeMgr&&) = delete;
    ReforgeMgr& operator=(ReforgeMgr const&) = delete;
    ReforgeMgr& operator=(ReforgeMgr&&) = delete;

public:
    static ReforgeMgr* instance();

private:

};

#define sReforgeMgr ReforgeMgr::instance()

#endif /* _REFORGE_H_ */
