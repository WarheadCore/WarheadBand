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

#include "DetourExtended.h"
#include "DetourCommon.h"
#include "Geometry.h"

float dtQueryFilterExt::getCost(const float* pa, const float* pb,
                const dtPolyRef /*prevRef*/, const dtMeshTile* /*prevTile*/, const dtPoly* /*prevPoly*/,
                const dtPolyRef /*curRef*/, const dtMeshTile* /*curTile*/, const dtPoly* curPoly,
                const dtPolyRef /*nextRef*/, const dtMeshTile* /*nextTile*/, const dtPoly* /*nextPoly*/) const
{
    float startX = pa[2], startY = pa[0], startZ = pa[1];
    float destX = pb[2], destY = pb[0], destZ = pb[1];
    float slopeAngle = getSlopeAngle(startX, startY, startZ, destX, destY, destZ);
    float slopeAngleDegree = (slopeAngle * 180.0f / M_PI);
    float cost = slopeAngleDegree > 0 ? 1.0f + (1.0f * (slopeAngleDegree / 100)) : 1.0f;
    float dist = dtVdist(pa, pb);
    auto totalCost = dist * cost * getAreaCost(curPoly->getArea());
    return totalCost;
}
