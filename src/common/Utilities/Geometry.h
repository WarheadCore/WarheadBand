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

/**
 *
 * Utility library to define some global function for geometric calculations
 *
 */

#ifndef _WARHEAD_GEOMETRY_H
#define _WARHEAD_GEOMETRY_H

#include "Define.h"
#include <cstdlib>
#include <iostream>
#include <math.h>

[[nodiscard]] inline float getAngle(float startX, float startY, float destX, float destY)
{
    auto dx = destX - startX;
    auto dy = destY - startY;

    auto ang = atan2(dy, dx);
    ang = (ang >= 0) ? ang : 2 * float(M_PI) + ang;
    return ang;
}

[[nodiscard]] inline float getSlopeAngle(float startX, float startY, float startZ, float destX, float destY, float destZ)
{
    float floorDist = sqrt(pow(startY - destY, 2.0f) + pow(startX - destX, 2.0f));
    return atan(abs(destZ - startZ) / abs(floorDist));
}

[[nodiscard]] inline float getSlopeAngleAbs(float startX, float startY, float startZ, float destX, float destY, float destZ)
{
    return abs(getSlopeAngle(startX, startY, startZ, destX, destY, destZ));
}

[[nodiscard]] inline double getCircleAreaByRadius(double radius)
{
    return radius * radius * M_PI;
}

[[nodiscard]] inline double getCirclePerimeterByRadius(double radius)
{
    return radius * M_PI;
}

[[nodiscard]] inline double getCylinderVolume(double height, double radius)
{
    return height * getCircleAreaByRadius(radius);
}

#endif // _WARHEAD_GEOMETRY_H
