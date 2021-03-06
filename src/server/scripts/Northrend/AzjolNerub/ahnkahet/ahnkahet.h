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

#ifndef DEF_AHNKAHET_H
#define DEF_AHNKAHET_H

#define MAX_ENCOUNTER           5

#include "CreatureAIImpl.h"

#define AhnahetScriptName "instance_ahnkahet"

enum Data64
{
    DATA_ELDER_NADOX,
    DATA_PRINCE_TALDARAM,
    DATA_JEDOGA_SHADOWSEEKER,
    DATA_HERALD_VOLAZJ,
    DATA_AMANITAR,
    DATA_PRINCE_TALDARAM_PLATFORM,
};

enum Data
{
    DATA_ELDER_NADOX_EVENT,
    DATA_PRINCE_TALDARAM_EVENT,
    DATA_JEDOGA_SHADOWSEEKER_EVENT,
    DATA_HERALD_VOLAZJ_EVENT,
    DATA_AMANITAR_EVENT,
    DATA_SPHERE_EVENT,

    DATA_NADOX_ACHIEVEMENT,
    DATA_JEDOGA_ACHIEVEMENT,
};

enum Npc
{
    NPC_ELDER_NADOX                 = 29309,
    NPC_PRINCE_TALDARAM             = 29308,
    NPC_JEDOGA_SHADOWSEEKER         = 29310,
    NPC_HERALD_JOLAZJ               = 29311,
    NPC_AMANITAR                    = 30258,

    //spells
    SPELL_SHADOW_SICKLE             = 56701, // Shadow Sickle Normal
    SPELL_SHADOW_SICKLE_H           = 59104  // Shadow Sickle Heroic
};

template <class AI, class T>
inline AI* GetAhnkahetAI(T* obj)
{
    return GetInstanceAI<AI>(obj, AhnahetScriptName);
}

#endif
