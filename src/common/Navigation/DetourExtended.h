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

#ifndef _WARHEAD_DETOUR_EXTENDED_H
#define _WARHEAD_DETOUR_EXTENDED_H

#include "Define.h"
#include "DetourNavMeshQuery.h"

class WH_COMMON_API dtQueryFilterExt : public dtQueryFilter
{
public:
    float getCost(const float* pa, const float* pb,
        const dtPolyRef prevRef, const dtMeshTile* prevTile, const dtPoly* prevPoly,
        const dtPolyRef curRef, const dtMeshTile* curTile, const dtPoly* curPoly,
        const dtPolyRef nextRef, const dtMeshTile* nextTile, const dtPoly* nextPoly) const override;
};

#endif // _WARHEAD_DETOUR_EXTENDED_H
