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

#include "Singleton.h"
#include "ItemTemplate.h"
#include "ObjectGuid.h"
#include <array>

class Player;
class Item;

class ReforgeItem
{
public:
    ReforgeItem(ObjectGuid itemGuid, uint32 id) : _guid(itemGuid), _id(id)
    {
        Reset();
    }

    ~ReforgeItem() = default;

    void Reset();
    bool AddStat(uint32 statType, int32 value);
    bool SetStat(uint32 statType, int32 value);
    int32 GetValueForStat(uint32 statType) const;
    void SaveToDB();

    inline uint32 GetID() const { return _id; }
    inline ObjectGuid GetGUID() const { return _guid; }
    inline uint32 GetStatsCount() const { return _itemStatsCount; }
    inline std::array<_ItemStat, MAX_ITEM_PROTO_STATS> const* GetStats() const { return &_itemStats; }

    inline _ItemStat const* GetItemsStatsForIndex(uint32 index)
    {
        if (index > _itemStatsCount)
            return nullptr;

        return &_itemStats[index];
    }

private:
    ObjectGuid _guid{ ObjectGuid::Empty };
    uint32 _id{ 0 };
    std::array<_ItemStat, MAX_ITEM_PROTO_STATS> _itemStats{};
    uint32 _itemStatsCount{ 0 };
};

class ReforgeMgr : public Warhead::Singleton<ReforgeMgr>
{
public:
    void Initialize();

    // Hooks
    void OnBeforeApplyItemBonuses(Player* player, Item* item, uint32& statsCount);
    void OnApplyItemBonuses(Player* player, Item* item, uint32 statIndex, uint32& statType, int32& value);

    bool ChangeItem(Player* player, Item* item, uint32 statType, int32 value);

private:
    void UpdateItemForPlayer(Player* player, Item* item);
    void CreateBaseReforgeItem(Item* item);
    void CreateBaseReforgeItem(ObjectGuid itemGuid, uint32 id);
    ReforgeItem* GetReforgeItem(ObjectGuid rawGuid);

    std::unordered_map<uint64, ReforgeItem> _store;
};

#define sReforgeMgr ReforgeMgr::instance()

#endif /* _REFORGE_H_ */
