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

#ifndef WARHEAD_CREATUREAISELECTOR_H
#define WARHEAD_CREATUREAISELECTOR_H

#include "Define.h"

class CreatureAI;
class Creature;
class MovementGenerator;
class GameObjectAI;
class GameObject;

namespace FactorySelector
{
    WH_GAME_API CreatureAI* selectAI(Creature*);
    WH_GAME_API MovementGenerator* selectMovementGenerator(Creature*);
    WH_GAME_API GameObjectAI* SelectGameObjectAI(GameObject*);
}

#endif
