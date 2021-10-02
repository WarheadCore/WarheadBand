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

/// \addtogroup world
/// @{
/// \file

#ifndef __WEATHERMGR_H
#define __WEATHERMGR_H

#include "Define.h"

class Weather;
class Player;

namespace WeatherMgr
{
    WH_GAME_API void LoadWeatherData();

    WH_GAME_API Weather* FindWeather(uint32 id);
    WH_GAME_API Weather* AddWeather(uint32 zone_id);
    WH_GAME_API void RemoveWeather(uint32 zone_id);

    WH_GAME_API void SendFineWeatherUpdateToPlayer(Player* player);

    WH_GAME_API void Update(uint32 diff);
}

#endif
