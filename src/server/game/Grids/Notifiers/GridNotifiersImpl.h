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

#ifndef WARHEAD_GRIDNOTIFIERSIMPL_H
#define WARHEAD_GRIDNOTIFIERSIMPL_H

#include "Corpse.h"
#include "CreatureAI.h"
#include "GridNotifiers.h"
#include "Object.h"
#include "Opcodes.h"
#include "Player.h"
#include "SpellAuras.h"
#include "UpdateData.h"
#include "WorldPacket.h"
#include "WorldSession.h"

template<class T>
inline void Warhead::VisibleNotifier::Visit(GridRefMgr<T>& m)
{
    // Xinef: Update gameobjects only
    if (i_gobjOnly)
        return;

    for (typename GridRefMgr<T>::iterator iter = m.begin(); iter != m.end(); ++iter)
    {
        if (i_largeOnly != iter->GetSource()->IsVisibilityOverridden())
            continue;

        vis_guids.erase(iter->GetSource()->GetGUID());
        i_player.UpdateVisibilityOf(iter->GetSource(), i_data, i_visibleNow);
    }
}

// SEARCHERS & LIST SEARCHERS & WORKERS

// WorldObject searchers & workers

template<class Check>
void Warhead::WorldObjectSearcher<Check>::Visit(GameObjectMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_GAMEOBJECT))
        return;

    // already found
    if (i_object)
        return;

    for (GameObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
        {
            i_object = itr->GetSource();
            return;
        }
    }
}

template<class Check>
void Warhead::WorldObjectSearcher<Check>::Visit(PlayerMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_PLAYER))
        return;

    // already found
    if (i_object)
        return;

    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
        {
            i_object = itr->GetSource();
            return;
        }
    }
}

template<class Check>
void Warhead::WorldObjectSearcher<Check>::Visit(CreatureMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CREATURE))
        return;

    // already found
    if (i_object)
        return;

    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
        {
            i_object = itr->GetSource();
            return;
        }
    }
}

template<class Check>
void Warhead::WorldObjectSearcher<Check>::Visit(CorpseMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CORPSE))
        return;

    // already found
    if (i_object)
        return;

    for (CorpseMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
        {
            i_object = itr->GetSource();
            return;
        }
    }
}

template<class Check>
void Warhead::WorldObjectSearcher<Check>::Visit(DynamicObjectMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_DYNAMICOBJECT))
        return;

    // already found
    if (i_object)
        return;

    for (DynamicObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
        {
            i_object = itr->GetSource();
            return;
        }
    }
}

template<class Check>
void Warhead::WorldObjectLastSearcher<Check>::Visit(GameObjectMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_GAMEOBJECT))
        return;

    for (GameObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
            i_object = itr->GetSource();
    }
}

template<class Check>
void Warhead::WorldObjectLastSearcher<Check>::Visit(PlayerMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_PLAYER))
        return;

    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
            i_object = itr->GetSource();
    }
}

template<class Check>
void Warhead::WorldObjectLastSearcher<Check>::Visit(CreatureMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CREATURE))
        return;

    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
            i_object = itr->GetSource();
    }
}

template<class Check>
void Warhead::WorldObjectLastSearcher<Check>::Visit(CorpseMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CORPSE))
        return;

    for (CorpseMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
            i_object = itr->GetSource();
    }
}

template<class Check>
void Warhead::WorldObjectLastSearcher<Check>::Visit(DynamicObjectMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_DYNAMICOBJECT))
        return;

    for (DynamicObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
            i_object = itr->GetSource();
    }
}

template<class Check>
void Warhead::WorldObjectListSearcher<Check>::Visit(PlayerMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_PLAYER))
        return;

    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (i_check(itr->GetSource()))
            Insert(itr->GetSource());
}

template<class Check>
void Warhead::WorldObjectListSearcher<Check>::Visit(CreatureMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CREATURE))
        return;

    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (i_check(itr->GetSource()))
            Insert(itr->GetSource());
}

template<class Check>
void Warhead::WorldObjectListSearcher<Check>::Visit(CorpseMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CORPSE))
        return;

    for (CorpseMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (i_check(itr->GetSource()))
            Insert(itr->GetSource());
}

template<class Check>
void Warhead::WorldObjectListSearcher<Check>::Visit(GameObjectMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_GAMEOBJECT))
        return;

    for (GameObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (i_check(itr->GetSource()))
            Insert(itr->GetSource());
}

template<class Check>
void Warhead::WorldObjectListSearcher<Check>::Visit(DynamicObjectMapType& m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_DYNAMICOBJECT))
        return;

    for (DynamicObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (i_check(itr->GetSource()))
            Insert(itr->GetSource());
}

// Gameobject searchers

template<class Check>
void Warhead::GameObjectSearcher<Check>::Visit(GameObjectMapType& m)
{
    // already found
    if (i_object)
        return;

    for (GameObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
        {
            i_object = itr->GetSource();
            return;
        }
    }
}

template<class Check>
void Warhead::GameObjectLastSearcher<Check>::Visit(GameObjectMapType& m)
{
    for (GameObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
            i_object = itr->GetSource();
    }
}

template<class Check>
void Warhead::GameObjectListSearcher<Check>::Visit(GameObjectMapType& m)
{
    for (GameObjectMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (itr->GetSource()->InSamePhase(i_phaseMask))
            if (i_check(itr->GetSource()))
                Insert(itr->GetSource());
}

// Unit searchers

template<class Check>
void Warhead::UnitSearcher<Check>::Visit(CreatureMapType& m)
{
    // already found
    if (i_object)
        return;

    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
        {
            i_object = itr->GetSource();
            return;
        }
    }
}

template<class Check>
void Warhead::UnitSearcher<Check>::Visit(PlayerMapType& m)
{
    // already found
    if (i_object)
        return;

    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
        {
            i_object = itr->GetSource();
            return;
        }
    }
}

template<class Check>
void Warhead::UnitLastSearcher<Check>::Visit(CreatureMapType& m)
{
    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
            i_object = itr->GetSource();
    }
}

template<class Check>
void Warhead::UnitLastSearcher<Check>::Visit(PlayerMapType& m)
{
    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
            i_object = itr->GetSource();
    }
}

template<class Check>
void Warhead::UnitListSearcher<Check>::Visit(PlayerMapType& m)
{
    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (itr->GetSource()->InSamePhase(i_phaseMask))
            if (i_check(itr->GetSource()))
                Insert(itr->GetSource());
}

template<class Check>
void Warhead::UnitListSearcher<Check>::Visit(CreatureMapType& m)
{
    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (itr->GetSource()->InSamePhase(i_phaseMask))
            if (i_check(itr->GetSource()))
                Insert(itr->GetSource());
}

// Creature searchers

template<class Check>
void Warhead::CreatureSearcher<Check>::Visit(CreatureMapType& m)
{
    // already found
    if (i_object)
        return;

    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
        {
            i_object = itr->GetSource();
            return;
        }
    }
}

template<class Check>
void Warhead::CreatureLastSearcher<Check>::Visit(CreatureMapType& m)
{
    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
            i_object = itr->GetSource();
    }
}

template<class Check>
void Warhead::CreatureListSearcher<Check>::Visit(CreatureMapType& m)
{
    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (itr->GetSource()->InSamePhase(i_phaseMask))
            if (i_check(itr->GetSource()))
                Insert(itr->GetSource());
}

template<class Check>
void Warhead::PlayerListSearcher<Check>::Visit(PlayerMapType& m)
{
    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (itr->GetSource()->InSamePhase(i_phaseMask))
            if (i_check(itr->GetSource()))
                Insert(itr->GetSource());
}

template<class Check>
void Warhead::PlayerListSearcherWithSharedVision<Check>::Visit(PlayerMapType& m)
{
    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (itr->GetSource()->InSamePhase(i_phaseMask))
            if (i_check(itr->GetSource(), true))
                i_objects.push_back(itr->GetSource());
}

template<class Check>
void Warhead::PlayerListSearcherWithSharedVision<Check>::Visit(CreatureMapType& m)
{
    for (CreatureMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
        if (itr->GetSource()->InSamePhase(i_phaseMask) && itr->GetSource()->HasSharedVision())
            for (SharedVisionList::const_iterator i = itr->GetSource()->GetSharedVisionList().begin(); i != itr->GetSource()->GetSharedVisionList().end(); ++i)
                if (i_check(*i, false))
                    i_objects.push_back(*i);
}

template<class Check>
void Warhead::PlayerSearcher<Check>::Visit(PlayerMapType& m)
{
    // already found
    if (i_object)
        return;

    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
        {
            i_object = itr->GetSource();
            return;
        }
    }
}

template<class Check>
void Warhead::PlayerLastSearcher<Check>::Visit(PlayerMapType& m)
{
    for (PlayerMapType::iterator itr = m.begin(); itr != m.end(); ++itr)
    {
        if (!itr->GetSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->GetSource()))
            i_object = itr->GetSource();
    }
}

template<class Builder>
void Warhead::LocalizedPacketDo<Builder>::operator()(Player* p)
{
    LocaleConstant loc_idx = p->GetSession()->GetSessionDbLocaleIndex();
    uint32 cache_idx = loc_idx + 1;
    WorldPacket* data;

    // create if not cached yet
    if (i_data_cache.size() < cache_idx + 1 || !i_data_cache[cache_idx])
    {
        if (i_data_cache.size() < cache_idx + 1)
            i_data_cache.resize(cache_idx + 1);

        data = new WorldPacket();

        i_builder(*data, loc_idx);

        i_data_cache[cache_idx] = data;
    }
    else
        data = i_data_cache[cache_idx];

    p->SendDirectMessage(data);
}

#endif                                                      // WARHEAD_GRIDNOTIFIERSIMPL_H
