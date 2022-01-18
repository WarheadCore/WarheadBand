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

#ifndef WARHEAD_OBJECTACCESSOR_H
#define WARHEAD_OBJECTACCESSOR_H

#include "Define.h"
#include "GridDefines.h"
#include "Object.h"
#include "UpdateData.h"
#include <mutex>
#include <set>
#include <shared_mutex>
#include <unordered_map>

class Creature;
class Corpse;
class Unit;
class GameObject;
class DynamicObject;
class WorldObject;
class Vehicle;
class Map;
class WorldRunnable;
class Transport;
class StaticTransport;
class MotionTransport;

template <class T>
class WH_GAME_API HashMapHolder
{
    //Non instanceable only static
    HashMapHolder() = default;

    HashMapHolder() { }
public:

    typedef std::unordered_map<ObjectGuid, T*> MapType;

    static void Insert(T* o);

    static void Remove(T* o);

    static T* Find(ObjectGuid guid);

    static MapType& GetContainer();

    static std::shared_mutex* GetLock();
};

namespace ObjectAccessor
{
    // these functions return objects only if in map of specified object
    WH_GAME_API WorldObject* GetWorldObject(WorldObject const&, ObjectGuid const guid);
    WH_GAME_API Object* GetObjectByTypeMask(WorldObject const&, ObjectGuid const guid, uint32 typemask);
    WH_GAME_API Corpse* GetCorpse(WorldObject const& u, ObjectGuid const guid);
    WH_GAME_API GameObject* GetGameObject(WorldObject const& u, ObjectGuid const guid);
    WH_GAME_API Transport* GetTransport(WorldObject const& u, ObjectGuid const guid);
    WH_GAME_API DynamicObject* GetDynamicObject(WorldObject const& u, ObjectGuid const guid);
    WH_GAME_API Unit* GetUnit(WorldObject const&, ObjectGuid const guid);
    WH_GAME_API Creature* GetCreature(WorldObject const& u, ObjectGuid const guid);
    WH_GAME_API Pet* GetPet(WorldObject const&, ObjectGuid const guid);
    WH_GAME_API Player* GetPlayer(Map const*, ObjectGuid const guid);
    WH_GAME_API Player* GetPlayer(WorldObject const&, ObjectGuid const guid);
    WH_GAME_API Creature* GetCreatureOrPetOrVehicle(WorldObject const&, ObjectGuid const);

    // these functions return objects if found in whole world
    // ACCESS LIKE THAT IS NOT THREAD SAFE
    WH_GAME_API Player* FindPlayer(ObjectGuid const guid);
    WH_GAME_API Player* FindPlayerByLowGUID(ObjectGuid::LowType lowguid);
    WH_GAME_API Player* FindConnectedPlayer(ObjectGuid const guid);
    WH_GAME_API Player* FindPlayerByName(std::string const& name, bool checkInWorld = true);

    // when using this, you must use the hashmapholder's lock
    WH_GAME_API HashMapHolder<Player>::MapType const& GetPlayers();

    template<class T>
    void AddObject(T* object)
    {
        HashMapHolder<T>::Insert(object);
    }

    template<class T>
    void RemoveObject(T* object)
    {
        HashMapHolder<T>::Remove(object);
    }

    WH_GAME_API void SaveAllPlayers();

    template<>
    void AddObject(Player* player);

    template<>
    void RemoveObject(Player* player);
}

#endif
