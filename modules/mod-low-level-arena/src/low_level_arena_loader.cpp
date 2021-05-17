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

#ifndef _LOW_LEVEL_ARENA_LOADER_H_
#define _LOW_LEVEL_ARENA_LOADER_H_

// From SC
void AddSC_LowLevelArena();

// Old api for AC
//void AddLowLevelArenaScripts()
//{
//    AddSC_LowLevelArena();
//}

// New api for WH
void Addmod_low_level_arenaScripts()
{
    AddSC_LowLevelArena();
}

#endif /* _LOW_LEVEL_ARENA_LOADER_H_ */
