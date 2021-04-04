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

#ifndef ACORE_LOOTMGR_H
#define ACORE_LOOTMGR_H

#include "ByteBuffer.h"
#include "ConditionMgr.h"
#include "ItemEnchantmentMgr.h"
#include "RefManager.h"
#include "SharedDefines.h"
#include <list>
#include <map>
#include <vector>

enum RollType
{
    ROLL_PASS         = 0,
    ROLL_NEED         = 1,
    ROLL_GREED        = 2,
    ROLL_DISENCHANT   = 3,
    MAX_ROLL_TYPE     = 4
};

enum RollMask
{
    ROLL_FLAG_TYPE_PASS         = 0x01,
    ROLL_FLAG_TYPE_NEED         = 0x02,
    ROLL_FLAG_TYPE_GREED        = 0x04,
    ROLL_FLAG_TYPE_DISENCHANT   = 0x08,

    ROLL_ALL_TYPE_NO_DISENCHANT = 0x07,
    ROLL_ALL_TYPE_MASK          = 0x0F
};

#define MAX_NR_LOOT_ITEMS 18
// note: the client cannot show more than 18 items in the loot window on 3.3.5a
#define MAX_NR_QUEST_ITEMS 32
// unrelated to the number of quest items shown, just for reserve

enum LootMethod
{
    FREE_FOR_ALL      = 0,
    ROUND_ROBIN       = 1,
    MASTER_LOOT       = 2,
    GROUP_LOOT        = 3,
    NEED_BEFORE_GREED = 4
};

enum PermissionTypes
{
    ALL_PERMISSION              = 0,
    GROUP_PERMISSION            = 1,
    MASTER_PERMISSION           = 2,
    RESTRICTED_PERMISSION       = 3,
    ROUND_ROBIN_PERMISSION      = 4,
    OWNER_PERMISSION            = 5,
    NONE_PERMISSION             = 6
};

enum LootType
{
    LOOT_NONE                   = 0,

    LOOT_CORPSE                 = 1,
    LOOT_PICKPOCKETING          = 2,
    LOOT_FISHING                = 3,
    LOOT_DISENCHANTING          = 4,
    // ignored always by client
    LOOT_SKINNING               = 6,
    LOOT_PROSPECTING            = 7,
    LOOT_MILLING                = 8,

    LOOT_FISHINGHOLE            = 20,                       // unsupported by client, sending LOOT_FISHING instead
    LOOT_INSIGNIA               = 21,                       // unsupported by client, sending LOOT_CORPSE instead
    LOOT_FISHING_JUNK           = 22                        // unsupported by client, sending LOOT_FISHING instead
};

enum LootError
{
    LOOT_ERROR_DIDNT_KILL               = 0,    // You don't have permission to loot that corpse.
    LOOT_ERROR_TOO_FAR                  = 4,    // You are too far away to loot that corpse.
    LOOT_ERROR_BAD_FACING               = 5,    // You must be facing the corpse to loot it.
    LOOT_ERROR_LOCKED                   = 6,    // Someone is already looting that corpse.
    LOOT_ERROR_NOTSTANDING              = 8,    // You need to be standing up to loot something!
    LOOT_ERROR_STUNNED                  = 9,    // You can't loot anything while stunned!
    LOOT_ERROR_PLAYER_NOT_FOUND         = 10,   // Player not found
    LOOT_ERROR_PLAY_TIME_EXCEEDED       = 11,   // Maximum play time exceeded
    LOOT_ERROR_MASTER_INV_FULL          = 12,   // That player's inventory is full
    LOOT_ERROR_MASTER_UNIQUE_ITEM       = 13,   // Player has too many of that item already
    LOOT_ERROR_MASTER_OTHER             = 14,   // Can't assign item to that player
    LOOT_ERROR_ALREADY_PICKPOCKETED     = 15,   // Your target has already had its pockets picked
    LOOT_ERROR_NOT_WHILE_SHAPESHIFTED   = 16    // You can't do that while shapeshifted.
};

// type of Loot Item in Loot View
enum LootSlotType
{
    LOOT_SLOT_TYPE_ALLOW_LOOT   = 0,                        // player can loot the item.
    LOOT_SLOT_TYPE_ROLL_ONGOING = 1,                        // roll is ongoing. player cannot loot.
    LOOT_SLOT_TYPE_MASTER       = 2,                        // item can only be distributed by group loot master.
    LOOT_SLOT_TYPE_LOCKED       = 3,                        // item is shown in red. player cannot loot.
    LOOT_SLOT_TYPE_OWNER        = 4,                        // ignore binding confirmation and etc, for single player looting
};

class Player;
class LootStore;
class ConditionMgr;
struct Loot;

struct LootStoreItem
{
    uint32  itemid;                                         // id of the item
    uint32  reference;                                      // referenced TemplateleId
    float   chance;                                         // chance to drop for both quest and non-quest items, chance to be used for refs
    bool    needs_quest : 1;                                // quest drop (quest is required for item to drop)
    uint16  lootmode;
    uint8   groupid     : 7;
    uint8   mincount;                                       // mincount for drop items
    uint8   maxcount;                                       // max drop count for the item mincount or Ref multiplicator
    ConditionList conditions;                               // additional loot condition

    // Constructor
    // displayid is filled in IsValid() which must be called after
    LootStoreItem(uint32 _itemid, uint32 _reference, float _chance, bool _needs_quest, uint16 _lootmode, uint8 _groupid, int32 _mincount, uint8 _maxcount)
        : itemid(_itemid), reference(_reference), chance(_chance), needs_quest(_needs_quest),
          lootmode(_lootmode), groupid(_groupid), mincount(_mincount), maxcount(_maxcount)
    {}

    bool Roll(bool rate, Player const* player, Loot& loot, LootStore const& store) const;                             // Checks if the entry takes it's chance (at loot generation)
    [[nodiscard]] bool IsValid(LootStore const& store, uint32 entry) const;
    // Checks correctness of values
};

typedef std::set<uint32> AllowedLooterSet;

struct LootItem
{
    uint32  itemid;
    uint32  randomSuffix;
    int32   randomPropertyId;
    ConditionList conditions;                               // additional loot condition
    AllowedLooterSet allowedGUIDs;
    uint64  rollWinnerGUID;                                 // Stores the guid of person who won loot, if his bags are full only he can see the item in loot list!
    uint8   count             : 8;
    bool    is_looted         : 1;
    bool    is_blocked        : 1;
    bool    freeforall        : 1;                          // free for all
    bool    is_underthreshold : 1;
    bool    is_counted        : 1;
    bool    needs_quest       : 1;                          // quest drop
    bool    follow_loot_rules : 1;

    // Constructor, copies most fields from LootStoreItem, generates random count and random suffixes/properties
    // Should be called for non-reference LootStoreItem entries only (reference = 0)
    explicit LootItem(LootStoreItem const& li);

    LootItem() = default;

    // Basic checks for player/item compatibility - if false no chance to see the item in the loot
    bool AllowedForPlayer(Player const* player) const;

    void AddAllowedLooter(Player const* player);
    [[nodiscard]] const AllowedLooterSet& GetAllowedLooters() const { return allowedGUIDs; }
};

struct QuestItem
{
    uint8   index{0};                                          // position in quest_items;
    bool    is_looted{false};

    QuestItem()
         {}

    QuestItem(uint8 _index, bool _islooted = false)
        : index(_index), is_looted(_islooted) {}
};

class LootTemplate;

typedef std::vector<QuestItem> QuestItemList;
typedef std::vector<LootItem> LootItemList;
typedef std::map<uint32, QuestItemList*> QuestItemMap;
typedef std::list<LootStoreItem*> LootStoreItemList;
typedef std::unordered_map<uint32, LootTemplate*> LootTemplateMap;

typedef std::set<uint32> LootIdSet;

class WH_GAME_API LootStore
{
public:
    explicit LootStore(char const* name, char const* entryName, bool ratesAllowed)
        : m_name(name), m_entryName(entryName), m_ratesAllowed(ratesAllowed) {}

    virtual ~LootStore() { Clear(); }

    uint32 LoadAndCollectLootIds(LootIdSet& ids_set);
    void ResetConditions();

    void Verify() const;
    void CheckLootRefs(LootIdSet* ref_set = nullptr) const; // check existence reference and remove it from ref_set
    void ReportUnusedIds(LootIdSet const& ids_set) const;
    void ReportNonExistingId(uint32 lootId) const;
    void ReportNonExistingId(uint32 lootId, const char* ownerType, uint32 ownerId) const;

    [[nodiscard]] bool HaveLootFor(uint32 loot_id) const { return m_LootTemplates.find(loot_id) != m_LootTemplates.end(); }
    [[nodiscard]] bool HaveQuestLootFor(uint32 loot_id) const;
    bool HaveQuestLootForPlayer(uint32 loot_id, Player const* player) const;

    [[nodiscard]] LootTemplate const* GetLootFor(uint32 loot_id) const;
    [[nodiscard]] LootTemplate* GetLootForConditionFill(uint32 loot_id) const;

    [[nodiscard]] char const* GetName() const { return m_name; }
    [[nodiscard]] char const* GetEntryName() const { return m_entryName; }
    [[nodiscard]] bool IsRatesAllowed() const { return m_ratesAllowed; }
protected:
    uint32 LoadLootTable();
    void Clear();
private:
    LootTemplateMap m_LootTemplates;
    char const* m_name;
    char const* m_entryName;
    bool m_ratesAllowed;
};

class WH_GAME_API LootTemplate
{
    class LootGroup;                                       // A set of loot definitions for items (refs are not allowed inside)
    typedef std::vector<LootGroup*> LootGroups;

public:
    LootTemplate() = default;
    ~LootTemplate();

    // Adds an entry to the group (at loading stage)
    void AddEntry(LootStoreItem* item);
    // Rolls for every item in the template and adds the rolled items the the loot
    void Process(Loot& loot, LootStore const& store, uint16 lootMode, Player const* player, uint8 groupId = 0) const;
    void CopyConditions(ConditionList conditions);
    void CopyConditions(LootItem* li) const;

    // True if template includes at least 1 quest drop entry
    [[nodiscard]] bool HasQuestDrop(LootTemplateMap const& store, uint8 groupId = 0) const;
    // True if template includes at least 1 quest drop for an active quest of the player
    bool HasQuestDropForPlayer(LootTemplateMap const& store, Player const* player, uint8 groupId = 0) const;

    // Checks integrity of the template
    void Verify(LootStore const& store, uint32 Id) const;
    void CheckLootRefs(LootTemplateMap const& store, LootIdSet* ref_set) const;
    bool addConditionItem(Condition* cond);
    [[nodiscard]] bool isReference(uint32 id) const;

private:
    LootStoreItemList Entries;                          // not grouped only
    LootGroups        Groups;                           // groups have own (optimised) processing, grouped entries go there

    // Objects of this class must never be copied, we are storing pointers in container
    LootTemplate(LootTemplate const&);
    LootTemplate& operator=(LootTemplate const&);
};

//=====================================================

class WH_GAME_API LootValidatorRef :  public Reference<Loot, LootValidatorRef>
{
public:
    LootValidatorRef() = default;
    void targetObjectDestroyLink() override {}
    void sourceObjectDestroyLink() override {}
};

//=====================================================

class WH_GAME_API LootValidatorRefManager : public RefManager<Loot, LootValidatorRef>
{
public:
    typedef LinkedListHead::Iterator< LootValidatorRef > iterator;

    LootValidatorRef* getFirst() { return (LootValidatorRef*)RefManager<Loot, LootValidatorRef>::getFirst(); }
    LootValidatorRef* getLast() { return (LootValidatorRef*)RefManager<Loot, LootValidatorRef>::getLast(); }

    iterator begin() { return iterator(getFirst()); }
    iterator end() { return iterator(nullptr); }
    iterator rbegin() { return iterator(getLast()); }
    iterator rend() { return iterator(nullptr); }
};

//=====================================================
struct LootView;

WH_GAME_API ByteBuffer& operator<<(ByteBuffer& b, LootItem const& li);
WH_GAME_API ByteBuffer& operator<<(ByteBuffer& b, LootView const& lv);

struct WH_GAME_API Loot
{
    friend WH_GAME_API ByteBuffer& operator<<(ByteBuffer& b, LootView const& lv);

    [[nodiscard]] QuestItemMap const& GetPlayerQuestItems() const { return PlayerQuestItems; }
    [[nodiscard]] QuestItemMap const& GetPlayerFFAItems() const { return PlayerFFAItems; }
    [[nodiscard]] QuestItemMap const& GetPlayerNonQuestNonFFAConditionalItems() const { return PlayerNonQuestNonFFAConditionalItems; }

    std::vector<LootItem> items;
    std::vector<LootItem> quest_items;
    uint32 gold;
    uint8 unlootedCount{0};
    uint64 roundRobinPlayer{0};                                // GUID of the player having the Round-Robin ownership for the loot. If 0, round robin owner has released.
    LootType loot_type{LOOT_NONE};                                     // required for achievement system

    // GUIDLow of container that holds this loot (item_instance.entry), set for items that can be looted
    uint32 containerId{0};

    Loot(uint32 _gold = 0) : gold(_gold) { }
    ~Loot() { clear(); }

    // if loot becomes invalid this reference is used to inform the listener
    void addLootValidatorRef(LootValidatorRef* pLootValidatorRef)
    {
        i_LootValidatorRefManager.insertFirst(pLootValidatorRef);
    }

    // void clear();
    void clear()
    {
        for (QuestItemMap::const_iterator itr = PlayerQuestItems.begin(); itr != PlayerQuestItems.end(); ++itr)
            delete itr->second;
        PlayerQuestItems.clear();

        for (QuestItemMap::const_iterator itr = PlayerFFAItems.begin(); itr != PlayerFFAItems.end(); ++itr)
            delete itr->second;
        PlayerFFAItems.clear();

        for (QuestItemMap::const_iterator itr = PlayerNonQuestNonFFAConditionalItems.begin(); itr != PlayerNonQuestNonFFAConditionalItems.end(); ++itr)
            delete itr->second;
        PlayerNonQuestNonFFAConditionalItems.clear();

        PlayersLooting.clear();
        items.clear();
        quest_items.clear();
        gold = 0;
        unlootedCount = 0;
        roundRobinPlayer = 0;
        i_LootValidatorRefManager.clearReferences();
        loot_type = LOOT_NONE;
    }

    [[nodiscard]] bool empty() const { return items.empty() && gold == 0; }
    [[nodiscard]] bool isLooted() const { return gold == 0 && unlootedCount == 0; }

    void NotifyItemRemoved(uint8 lootIndex);
    void NotifyQuestItemRemoved(uint8 questIndex);
    void NotifyMoneyRemoved();
    void AddLooter(uint64 GUID) { PlayersLooting.insert(GUID); }
    void RemoveLooter(uint64 GUID) { PlayersLooting.erase(GUID); }

    void generateMoneyLoot(uint32 minAmount, uint32 maxAmount);
    bool FillLoot(uint32 lootId, LootStore const& store, Player* lootOwner, bool personal, bool noEmptyError = false, uint16 lootMode = LOOT_MODE_DEFAULT);

    // Inserts the item into the loot (called by LootTemplate processors)
    void AddItem(LootStoreItem const& item);

    LootItem* LootItemInSlot(uint32 lootslot, Player* player, QuestItem** qitem = nullptr, QuestItem** ffaitem = nullptr, QuestItem** conditem = nullptr);
    uint32 GetMaxSlotInLootFor(Player* player) const;
    bool hasItemForAll() const;
    bool hasItemFor(Player* player) const;
    [[nodiscard]] bool hasOverThresholdItem() const;

private:
    void FillNotNormalLootFor(Player* player, bool presentAtLooting);
    QuestItemList* FillFFALoot(Player* player);
    QuestItemList* FillQuestLoot(Player* player);
    QuestItemList* FillNonQuestNonFFAConditionalLoot(Player* player, bool presentAtLooting);

    typedef std::set<uint64> PlayersLootingSet;
    PlayersLootingSet PlayersLooting;
    QuestItemMap PlayerQuestItems;
    QuestItemMap PlayerFFAItems;
    QuestItemMap PlayerNonQuestNonFFAConditionalItems;

    // All rolls are registered here. They need to know, when the loot is not valid anymore
    LootValidatorRefManager i_LootValidatorRefManager;
};

struct LootView
{
    Loot& loot;
    Player* viewer;
    PermissionTypes permission;
    LootView(Loot& _loot, Player* _viewer, PermissionTypes _permission = ALL_PERMISSION)
        : loot(_loot), viewer(_viewer), permission(_permission) {}
};

WH_GAME_API extern LootStore LootTemplates_Creature;
WH_GAME_API extern LootStore LootTemplates_Fishing;
WH_GAME_API extern LootStore LootTemplates_Gameobject;
WH_GAME_API extern LootStore LootTemplates_Item;
WH_GAME_API extern LootStore LootTemplates_Mail;
WH_GAME_API extern LootStore LootTemplates_Milling;
WH_GAME_API extern LootStore LootTemplates_Pickpocketing;
WH_GAME_API extern LootStore LootTemplates_Reference;
WH_GAME_API extern LootStore LootTemplates_Skinning;
WH_GAME_API extern LootStore LootTemplates_Disenchant;
WH_GAME_API extern LootStore LootTemplates_Prospecting;
WH_GAME_API extern LootStore LootTemplates_Spell;

WH_GAME_API void LoadLootTemplates_Creature();
WH_GAME_API void LoadLootTemplates_Fishing();
WH_GAME_API void LoadLootTemplates_Gameobject();
WH_GAME_API void LoadLootTemplates_Item();
WH_GAME_API void LoadLootTemplates_Mail();
WH_GAME_API void LoadLootTemplates_Milling();
WH_GAME_API void LoadLootTemplates_Pickpocketing();
WH_GAME_API void LoadLootTemplates_Skinning();
WH_GAME_API void LoadLootTemplates_Disenchant();
WH_GAME_API void LoadLootTemplates_Prospecting();

WH_GAME_API void LoadLootTemplates_Spell();
WH_GAME_API void LoadLootTemplates_Reference();

WH_GAME_API void LoadLootTables();

#endif
