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

#include "AuctionHouseBotSeller.h"
#include "AuctionHouseMgr.h"
#include "Containers.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "GameConfig.h"
#include "GameTime.h"
#include "Item.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Random.h"
#include "SmartEnum.h"
#include "StopWatch.h"
#include "StringConvert.h"
#include "Tokenize.h"

AuctionBotSeller::AuctionBotSeller()
{
    // Define faction for our main data class.
    for (auto type : EnumUtils::Iterate<AuctionHouseType>())
        _houseConfig[type].Initialize(type);
}

bool AuctionBotSeller::Initialize()
{
    StopWatch sw;

    _rateMoney = CONF_GET_FLOAT("Rate.Drop.Money");

    std::unordered_set<uint32> npcItems;
    std::unordered_set<uint32> lootItems;
    std::unordered_set<uint32> includeItems;
    std::unordered_set<uint32> excludeItems;

    LOG_DEBUG("ahbot", "AHBot seller filters:");

    {
        auto itemsInclude{ sAuctionBotConfig->GetAHBotIncludes() };
        for (auto itemStr : Warhead::Tokenize(itemsInclude, ',', false))
        {
            auto itemID{ Warhead::StringTo<uint32>(itemStr) };
            if (!itemID)
                continue;

            includeItems.emplace(*itemID);
        }
    }

    {
        auto itemsExclude{ sAuctionBotConfig->GetAHBotExcludes() };
        for (auto itemStr : Warhead::Tokenize(itemsExclude, ',', false))
        {
            auto itemID{ Warhead::StringTo<uint32>(itemStr) };
            if (!itemID)
                continue;

            excludeItems.emplace(*itemID);
        }
    }

    LOG_DEBUG("ahbot", "Forced Inclusion {} items", includeItems.size());
    LOG_DEBUG("ahbot", "Forced Exclusion {} items", excludeItems.size());
    LOG_DEBUG("ahbot", "Loading npc vendor items for filter...");

    for (auto const& creatureTemplatePair : *sObjectMgr->GetCreatureTemplates())
        if (VendorItemData const* data = sObjectMgr->GetNpcVendorItemList(creatureTemplatePair.first))
            for (auto const& vendorItem : data->m_items)
                npcItems.emplace(vendorItem->item);

    LOG_DEBUG("ahbot", "Npc vendor filter has {} items", npcItems.size());
    LOG_DEBUG("ahbot", "Loading loot items for filter...");

    QueryResult result = WorldDatabase.Query(
        "SELECT `item` FROM `creature_loot_template` WHERE `Reference` = 0 UNION "
        "SELECT `item` FROM `disenchant_loot_template` WHERE `Reference` = 0 UNION "
        "SELECT `item` FROM `fishing_loot_template` WHERE `Reference` = 0 UNION "
        "SELECT `item` FROM `gameobject_loot_template` WHERE `Reference` = 0 UNION "
        "SELECT `item` FROM `item_loot_template` WHERE `Reference` = 0 UNION "
        "SELECT `item` FROM `milling_loot_template` WHERE `Reference` = 0 UNION "
        "SELECT `item` FROM `pickpocketing_loot_template` WHERE `Reference` = 0 UNION "
        "SELECT `item` FROM `prospecting_loot_template` WHERE `Reference` = 0 UNION "
        "SELECT `item` FROM `reference_loot_template` WHERE `Reference` = 0 UNION "
        "SELECT `item` FROM `skinning_loot_template` WHERE `Reference` = 0 UNION "
        "SELECT `item` FROM `spell_loot_template` WHERE `Reference` = 0");

    if (result)
    {
        for (auto const& fields : *result)
        {
            uint32 entry = fields[0].Get<uint32>();
            if (!entry)
                continue;

            lootItems.emplace(entry);
        }
    }

    LOG_DEBUG("ahbot", "Loot filter has {} items", lootItems.size());
    LOG_DEBUG("ahbot", "Sorting and cleaning items for AHBot seller...");

    uint32 itemsAdded = 0;

    for (auto const& [itemId, itemTemplate] : *sObjectMgr->GetItemTemplateStore())
    {
        // skip items with too high quality (code can't properly work with its)
        if (itemTemplate.Quality >= MAX_AUCTION_QUALITY)
            continue;

        // forced exclude filter
        if (excludeItems.contains(itemId))
            continue;

        // forced include filter
        if (includeItems.contains(itemId))
        {
            _itemPool[itemTemplate.Quality][itemTemplate.Class].emplace_back(itemId);
            ++itemsAdded;
            continue;
        }

        // bounding filters
        switch (itemTemplate.Bonding)
        {
            case NO_BIND:
                if (!sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BIND_NO))
                    continue;
                break;
            case BIND_WHEN_PICKED_UP:
                if (!sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BIND_PICKUP))
                    continue;
                break;
            case BIND_WHEN_EQUIPED:
                if (!sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BIND_EQUIP))
                    continue;
                break;
            case BIND_WHEN_USE:
                if (!sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BIND_USE))
                    continue;
                break;
            case BIND_QUEST_ITEM:
                if (!sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BIND_QUEST))
                    continue;
                break;
            default:
                continue;
        }

        bool allowZero = false;
        switch (itemTemplate.Class)
        {
            case ITEM_CLASS_CONSUMABLE:
                allowZero = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_CONSUMABLE_ALLOW_ZERO); break;
            case ITEM_CLASS_CONTAINER:
                allowZero = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_CONTAINER_ALLOW_ZERO); break;
            case ITEM_CLASS_WEAPON:
                allowZero = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_WEAPON_ALLOW_ZERO); break;
            case ITEM_CLASS_GEM:
                allowZero = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_GEM_ALLOW_ZERO); break;
            case ITEM_CLASS_ARMOR:
                allowZero = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_ARMOR_ALLOW_ZERO); break;
            case ITEM_CLASS_REAGENT:
                allowZero = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_REAGENT_ALLOW_ZERO); break;
            case ITEM_CLASS_PROJECTILE:
                allowZero = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_PROJECTILE_ALLOW_ZERO); break;
            case ITEM_CLASS_TRADE_GOODS:
                allowZero = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_TRADEGOOD_ALLOW_ZERO); break;
            case ITEM_CLASS_RECIPE:
                allowZero = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RECIPE_ALLOW_ZERO); break;
            case ITEM_CLASS_QUIVER:
                allowZero = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_QUIVER_ALLOW_ZERO); break;
            case ITEM_CLASS_QUEST:
                allowZero = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_QUEST_ALLOW_ZERO); break;
            case ITEM_CLASS_KEY:
                allowZero = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_KEY_ALLOW_ZERO); break;
            case ITEM_CLASS_MISC:
                allowZero = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_MISC_ALLOW_ZERO); break;
            case ITEM_CLASS_GLYPH:
                allowZero = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_GLYPH_ALLOW_ZERO); break;
            default:
                allowZero = false;
        }

        // Filter out items with no buy/sell price unless otherwise flagged in the config.
        if (!allowZero)
        {
            if (sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYPRICE_SELLER))
            {
                if (!itemTemplate.SellPrice)
                    continue;
            }
            else
            {
                if (!itemTemplate.BuyPrice)
                    continue;
            }
        }

        // vendor filter
        if (!sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEMS_VENDOR) && npcItems.contains(itemId))
            continue;

        // loot filter
        if (!sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEMS_LOOT) && lootItems.contains(itemId))
            continue;

        // not vendor/loot filter
        if (!sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEMS_MISC))
        {
            bool const isVendorItem = npcItems.contains(itemId);
            bool const isLootItem = lootItems.contains(itemId);

            if (!isLootItem && !isVendorItem)
                continue;
        }

        // item class/subclass specific filters
        switch (itemTemplate.Class)
        {
            case ITEM_CLASS_ARMOR:
            case ITEM_CLASS_WEAPON:
            {
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_MIN_ITEM_LEVEL))
                    if (itemTemplate.ItemLevel && itemTemplate.ItemLevel < value)
                        continue;
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_MAX_ITEM_LEVEL))
                    if (itemTemplate.ItemLevel && itemTemplate.ItemLevel > value)
                        continue;
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_MIN_REQ_LEVEL))
                    if (itemTemplate.RequiredLevel && itemTemplate.RequiredLevel < value)
                        continue;
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_MAX_REQ_LEVEL))
                    if (itemTemplate.RequiredLevel && itemTemplate.RequiredLevel > value)
                        continue;
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_MIN_SKILL_RANK))
                    if (itemTemplate.RequiredSkillRank && itemTemplate.RequiredSkillRank < value)
                        continue;
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_MAX_SKILL_RANK))
                    if (itemTemplate.RequiredSkillRank && itemTemplate.RequiredSkillRank > value)
                        continue;
                break;
            }
            case ITEM_CLASS_RECIPE:
            case ITEM_CLASS_CONSUMABLE:
            case ITEM_CLASS_PROJECTILE:
            {
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_MIN_REQ_LEVEL))
                    if (itemTemplate.RequiredLevel && itemTemplate.RequiredLevel < value)
                        continue;
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_MAX_REQ_LEVEL))
                    if (itemTemplate.RequiredLevel && itemTemplate.RequiredLevel > value)
                        continue;
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_MIN_SKILL_RANK))
                    if (itemTemplate.RequiredSkillRank && itemTemplate.RequiredSkillRank < value)
                        continue;
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_MAX_SKILL_RANK))
                    if (itemTemplate.RequiredSkillRank && itemTemplate.RequiredSkillRank > value)
                        continue;
                break;
            }
            case ITEM_CLASS_MISC:
                if (itemTemplate.SubClass == ITEM_SUBCLASS_JUNK_MOUNT)
                {
                    if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_MISC_MOUNT_MIN_REQ_LEVEL))
                        if (itemTemplate.RequiredLevel && itemTemplate.RequiredLevel < value)
                            continue;
                    if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_MISC_MOUNT_MAX_REQ_LEVEL))
                        if (itemTemplate.RequiredLevel && itemTemplate.RequiredLevel > value)
                            continue;
                    if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_MISC_MOUNT_MIN_SKILL_RANK))
                        if (itemTemplate.RequiredSkillRank && itemTemplate.RequiredSkillRank < value)
                            continue;
                    if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_MISC_MOUNT_MAX_SKILL_RANK))
                        if (itemTemplate.RequiredSkillRank && itemTemplate.RequiredSkillRank > value)
                            continue;
                }

                if (itemTemplate.Flags & ITEM_FLAG_HAS_LOOT)
                {
                    // skip any not locked lootable items (mostly quest specific or reward cases)
                    if (!itemTemplate.LockID)
                        continue;

                    if (!sAuctionBotConfig->GetConfig(CONFIG_AHBOT_LOCKBOX_ENABLED))
                        continue;
                }

                break;
            case ITEM_CLASS_GLYPH:
            {
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_GLYPH_MIN_REQ_LEVEL))
                    if (itemTemplate.RequiredLevel && itemTemplate.RequiredLevel < value)
                        continue;
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_GLYPH_MAX_REQ_LEVEL))
                    if (itemTemplate.RequiredLevel && itemTemplate.RequiredLevel > value)
                        continue;
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_GLYPH_MIN_ITEM_LEVEL))
                    if (itemTemplate.RequiredLevel && itemTemplate.RequiredLevel < value)
                        continue;
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_GLYPH_MAX_ITEM_LEVEL))
                    if (itemTemplate.RequiredLevel && itemTemplate.RequiredLevel > value)
                        continue;
                break;
            }
            case ITEM_CLASS_TRADE_GOODS:
            {
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_TRADEGOOD_MIN_ITEM_LEVEL))
                    if (itemTemplate.ItemLevel && itemTemplate.ItemLevel < value)
                        continue;
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_TRADEGOOD_MAX_ITEM_LEVEL))
                    if (itemTemplate.ItemLevel && itemTemplate.ItemLevel > value)
                        continue;
                break;
            }
            case ITEM_CLASS_CONTAINER:
            case ITEM_CLASS_QUIVER:
            {
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_CONTAINER_MIN_ITEM_LEVEL))
                    if (itemTemplate.ItemLevel && itemTemplate.ItemLevel < value)
                        continue;
                if (uint32 value = sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_CONTAINER_MAX_ITEM_LEVEL))
                    if (itemTemplate.ItemLevel && itemTemplate.ItemLevel > value)
                        continue;
                break;
            }
        }

        _itemPool[itemTemplate.Quality][itemTemplate.Class].emplace_back(itemId);
        ++itemsAdded;
    }

    if (!itemsAdded)
    {
        LOG_ERROR("ahbot", "AHBotSeller: Not have items. Disabled. Elapsed: {}", sw);
        sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ALLIANCE_ITEM_AMOUNT_RATIO, 0);
        sAuctionBotConfig->SetConfig(CONFIG_AHBOT_HORDE_ITEM_AMOUNT_RATIO, 0);
        sAuctionBotConfig->SetConfig(CONFIG_AHBOT_NEUTRAL_ITEM_AMOUNT_RATIO, 0);
        return false;
    }

    LOG_DEBUG("ahbot", "AHBotSeller: Will use {} items to fill auction house (according your config choices)", itemsAdded);

    LoadConfig();

    LOG_DEBUG("ahbot", "Items loaded:");
    LOG_DEBUG("ahbot", "Item class              | Total | Gray | White | Green | Blue | Purple | Orange | Yellow |");
    LOG_DEBUG("ahbot", "........................|.......|......|.......|.......|......|........|........|........|");

    std::size_t totalCount{};
    std::size_t grayCount{};
    std::size_t whiteCount{};
    std::size_t greenCount{};
    std::size_t blueCount{};
    std::size_t purpleCount{};
    std::size_t orangeCount{};
    std::size_t yellowCount{};

    for (ItemClass itemClass : EnumUtils::Iterate<ItemClass>())
    {
        auto itemClassStr{ EnumUtils::ToTitle(itemClass) };
        auto grayCountClass{ _itemPool[AUCTION_QUALITY_GRAY][itemClass].size() };
        auto whiteCountClass{ _itemPool[AUCTION_QUALITY_WHITE][itemClass].size() };
        auto greenCountClass{ _itemPool[AUCTION_QUALITY_GREEN][itemClass].size() };
        auto blueCountClass{ _itemPool[AUCTION_QUALITY_BLUE][itemClass].size() };
        auto purpleCountClass{ _itemPool[AUCTION_QUALITY_PURPLE][itemClass].size() };
        auto orangeCountClass{ _itemPool[AUCTION_QUALITY_ORANGE][itemClass].size() };
        auto yellowCountClass{ _itemPool[AUCTION_QUALITY_YELLOW][itemClass].size() };
        auto allCountClass{ grayCountClass + whiteCountClass + greenCountClass + blueCountClass + purpleCountClass + orangeCountClass + yellowCountClass };

        totalCount += allCountClass;
        grayCount += grayCountClass;
        whiteCount += whiteCountClass;
        greenCount += greenCountClass;
        blueCount += blueCountClass;
        purpleCount += purpleCountClass;
        orangeCount += orangeCountClass;
        yellowCount += yellowCountClass;

        LOG_DEBUG("ahbot", "{:<23} | {:5} | {:4} | {:5} | {:5} | {:4} | {:6} | {:6} | {:6} |",
            itemClassStr, allCountClass, grayCountClass, whiteCountClass, greenCountClass, blueCountClass, purpleCountClass, orangeCountClass, yellowCountClass);

        LOG_DEBUG("ahbot", "........................|.......|......|.......|.......|......|........|........|........|");
    }

    LOG_DEBUG("ahbot", "{:<23} | {:5} | {:4} | {:5} | {:5} | {:4} | {:6} | {:6} | {:6} |",
              "Total count", totalCount, grayCount, whiteCount, greenCount, blueCount, purpleCount, orangeCount, yellowCount);

    LOG_DEBUG("ahbot", "........................|.......|......|.......|.......|......|........|........|........|");
    LOG_DEBUG("ahbot", "AHBotSeller: Configuration data loaded and initialized in {}", sw);
    LOG_DEBUG("ahbot", "");
    return true;
}

void AuctionBotSeller::LoadConfig()
{
    for (uint8 i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
        if (sAuctionBotConfig->GetConfigItemAmountRatio(AuctionHouseType(i)))
            LoadSellerValues(_houseConfig[i]);
}

void AuctionBotSeller::LoadItemsQuantity(SellerConfiguration& config)
{
    uint32 ratio = sAuctionBotConfig->GetConfigItemAmountRatio(config.GetHouseType());

    for (uint32 i = 0; i < MAX_AUCTION_QUALITY; ++i)
    {
        uint32 amount = sAuctionBotConfig->GetConfig(AuctionBotConfigUInt32Values(CONFIG_AHBOT_ITEM_GRAY_AMOUNT + i));
        config.SetItemsAmountPerQuality(AuctionQuality(i), std::lroundf(amount * ratio / 100.f));
    }

    // Set Stack Quantities
    config.SetRandomStackRatioPerClass(ITEM_CLASS_CONSUMABLE, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_CONSUMABLE));
    config.SetRandomStackRatioPerClass(ITEM_CLASS_CONTAINER, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_CONTAINER));
    config.SetRandomStackRatioPerClass(ITEM_CLASS_WEAPON, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_WEAPON));
    config.SetRandomStackRatioPerClass(ITEM_CLASS_GEM, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_GEM));
    config.SetRandomStackRatioPerClass(ITEM_CLASS_ARMOR, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_ARMOR));
    config.SetRandomStackRatioPerClass(ITEM_CLASS_REAGENT, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_REAGENT));
    config.SetRandomStackRatioPerClass(ITEM_CLASS_PROJECTILE, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_PROJECTILE));
    config.SetRandomStackRatioPerClass(ITEM_CLASS_TRADE_GOODS, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_TRADEGOOD));
    config.SetRandomStackRatioPerClass(ITEM_CLASS_GENERIC, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_GENERIC));
    config.SetRandomStackRatioPerClass(ITEM_CLASS_RECIPE, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_RECIPE));
    config.SetRandomStackRatioPerClass(ITEM_CLASS_QUIVER, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_QUIVER));
    config.SetRandomStackRatioPerClass(ITEM_CLASS_QUEST, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_QUEST));
    config.SetRandomStackRatioPerClass(ITEM_CLASS_KEY, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_KEY));
    config.SetRandomStackRatioPerClass(ITEM_CLASS_MISC, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_MISC));
    config.SetRandomStackRatioPerClass(ITEM_CLASS_GLYPH, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RANDOMSTACKRATIO_GLYPH));

    // Set the best value to get nearest amount of items wanted
    auto getPriorityForClass = [](uint32 itemClass) -> uint32
    {
        AuctionBotConfigUInt32Values index;
        switch (itemClass)
        {
            case ITEM_CLASS_CONSUMABLE:
                index = CONFIG_AHBOT_CLASS_CONSUMABLE_PRIORITY; break;
            case ITEM_CLASS_CONTAINER:
                index = CONFIG_AHBOT_CLASS_CONTAINER_PRIORITY; break;
            case ITEM_CLASS_WEAPON:
                index = CONFIG_AHBOT_CLASS_WEAPON_PRIORITY; break;
            case ITEM_CLASS_GEM:
                index = CONFIG_AHBOT_CLASS_GEM_PRIORITY; break;
            case ITEM_CLASS_ARMOR:
                index = CONFIG_AHBOT_CLASS_ARMOR_PRIORITY; break;
            case ITEM_CLASS_REAGENT:
                index = CONFIG_AHBOT_CLASS_REAGENT_PRIORITY; break;
            case ITEM_CLASS_PROJECTILE:
                index = CONFIG_AHBOT_CLASS_PROJECTILE_PRIORITY; break;
            case ITEM_CLASS_TRADE_GOODS:
                index = CONFIG_AHBOT_CLASS_TRADEGOOD_PRIORITY; break;
            case ITEM_CLASS_GENERIC:
                index = CONFIG_AHBOT_CLASS_GENERIC_PRIORITY; break;
            case ITEM_CLASS_RECIPE:
                index = CONFIG_AHBOT_CLASS_RECIPE_PRIORITY; break;
            case ITEM_CLASS_QUIVER:
                index = CONFIG_AHBOT_CLASS_QUIVER_PRIORITY; break;
            case ITEM_CLASS_QUEST:
                index = CONFIG_AHBOT_CLASS_QUEST_PRIORITY; break;
            case ITEM_CLASS_KEY:
                index = CONFIG_AHBOT_CLASS_KEY_PRIORITY; break;
            case ITEM_CLASS_MISC:
                index = CONFIG_AHBOT_CLASS_MISC_PRIORITY; break;
            case ITEM_CLASS_GLYPH:
                index = CONFIG_AHBOT_CLASS_GLYPH_PRIORITY; break;
            default:
                return 0;
        }

        return sAuctionBotConfig->GetConfig(index);
    };

    std::vector<uint32> totalPrioPerQuality(MAX_AUCTION_QUALITY);
    for (uint32 j = 0; j < MAX_AUCTION_QUALITY; ++j)
    {
        for (uint32 i = 0; i < MAX_ITEM_CLASS; ++i)
        {
            // skip empty pools
            if (_itemPool[j][i].empty())
                continue;

            totalPrioPerQuality[j] += getPriorityForClass(i);
        }
    }

    for (uint32 j = 0; j < MAX_AUCTION_QUALITY; ++j)
    {
        uint32 qualityAmount = config.GetItemsAmountPerQuality(AuctionQuality(j));
        if (!totalPrioPerQuality[j])
            continue;

        for (uint32 i = 0; i < MAX_ITEM_CLASS; ++i)
        {
            uint32 classPrio = getPriorityForClass(i);
            if (_itemPool[j][i].empty())
                classPrio = 0;

            uint32 weightedAmount = std::lroundf(classPrio / float(totalPrioPerQuality[j]) * qualityAmount);
            config.SetItemsAmountPerClass(AuctionQuality(j), ItemClass(i), weightedAmount);
        }
    }

    // do some assert checking, GetItemAmount must always return 0 if selected _itemPool is empty
    for (uint32 j = 0; j < MAX_AUCTION_QUALITY; ++j)
    {
        for (uint32 i = 0; i < MAX_ITEM_CLASS; ++i)
        {
            if (_itemPool[j][i].empty())
                ASSERT(config.GetItemsAmountPerClass(AuctionQuality(j), ItemClass(i)) == 0);
        }
    }
}

void AuctionBotSeller::LoadSellerValues(SellerConfiguration& config)
{
    LoadItemsQuantity(config);
    uint32 ratio = sAuctionBotConfig->GetConfigPriceRatio(config.GetHouseType());

    for (uint32 i = 0; i < MAX_AUCTION_QUALITY; ++i)
    {
        uint32 amount = sAuctionBotConfig->GetConfig(AuctionBotConfigUInt32Values(CONFIG_AHBOT_ITEM_GRAY_PRICE_RATIO + i));
        config.SetPriceRatioPerQuality(AuctionQuality(i), std::lroundf(amount * ratio / 100.f));
    }

    config.SetPriceRatioPerClass(ITEM_CLASS_CONSUMABLE, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_CONSUMABLE_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_CONTAINER, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_CONTAINER_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_WEAPON, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_WEAPON_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_GEM, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_GEM_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_ARMOR, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_ARMOR_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_REAGENT, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_REAGENT_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_PROJECTILE, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_PROJECTILE_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_TRADE_GOODS, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_TRADEGOOD_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_GENERIC, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_GENERIC_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_RECIPE, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_RECIPE_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_MONEY, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_MONEY_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_QUIVER, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_QUIVER_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_QUEST, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_QUEST_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_KEY, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_KEY_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_PERMANENT, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_PERMANENT_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_MISC, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_MISC_PRICE_RATIO));
    config.SetPriceRatioPerClass(ITEM_CLASS_GLYPH, sAuctionBotConfig->GetConfig(CONFIG_AHBOT_CLASS_GLYPH_PRICE_RATIO));

    //load min and max auction times
    config.SetMinTime(Hours{ sAuctionBotConfig->GetConfig(CONFIG_AHBOT_MINTIME) });
    config.SetMaxTime(Hours{ sAuctionBotConfig->GetConfig(CONFIG_AHBOT_MAXTIME) });
}

// Set static of items on one AH faction.
// Fill ItemInfos object with real content of AH.
uint32 AuctionBotSeller::SetStat(SellerConfiguration& config)
{
    AllItemsArray itemsSaved(MAX_AUCTION_QUALITY, std::vector<uint32>(MAX_ITEM_CLASS));
    auto auctionObject{ sAuctionMgr->GetAuctionsMap(config.GetHouseType()) };

    auctionObject->ForEachAuctions([this, &config, &itemsSaved](AuctionEntry* auction)
    {
        Item* item = sAuctionMgr->GetAuctionItem(auction->ItemGuid);
        if (item)
        {
            ItemTemplate const* prototype = item->GetTemplate();
            if (prototype)
                if (!auction->PlayerOwner || sAuctionBotConfig->IsBotChar(auction->PlayerOwner.GetCounter())) // Add only ahbot items
                    ++itemsSaved[prototype->Quality][prototype->Class];
        }
    });

    uint32 count = 0;
    for (uint32 j = 0; j < MAX_AUCTION_QUALITY; ++j)
    {
        for (uint32 i = 0; i < MAX_ITEM_CLASS; ++i)
        {
            config.SetMissedItemsPerClass((AuctionQuality)j, (ItemClass)i, itemsSaved[j][i]);
            count += config.GetMissedItemsPerClass((AuctionQuality)j, (ItemClass)i);
        }
    }

    LOG_DEBUG("ahbot", "AHBotSeller: Missed Item       \tGray\tWhite\tGreen\tBlue\tPurple\tOrange\tYellow");

    for (uint32 i = 0; i < MAX_ITEM_CLASS; ++i)
    {
        LOG_DEBUG("ahbot", "AHBotSeller: \t\t{}\t{}\t{}\t{}\t{}\t{}\t{}",
            config.GetMissedItemsPerClass(AUCTION_QUALITY_GRAY, (ItemClass)i),
            config.GetMissedItemsPerClass(AUCTION_QUALITY_WHITE, (ItemClass)i),
            config.GetMissedItemsPerClass(AUCTION_QUALITY_GREEN, (ItemClass)i),
            config.GetMissedItemsPerClass(AUCTION_QUALITY_BLUE, (ItemClass)i),
            config.GetMissedItemsPerClass(AUCTION_QUALITY_PURPLE, (ItemClass)i),
            config.GetMissedItemsPerClass(AUCTION_QUALITY_ORANGE, (ItemClass)i),
            config.GetMissedItemsPerClass(AUCTION_QUALITY_YELLOW, (ItemClass)i));
    }

    config.LastMissedItem = count;
    return count;
}

// getRandomArray is used to make viable the possibility to add any of missed item in place of first one to last one.
bool AuctionBotSeller::GetItemsToSell(SellerConfiguration& config, ItemsToSellArray& itemsToSellArray, AllItemsArray const& addedItem)
{
    itemsToSellArray.clear();
    bool found = false;

    for (uint32 j = 0; j < MAX_AUCTION_QUALITY; ++j)
    {
        for (uint32 i = 0; i < MAX_ITEM_CLASS; ++i)
        {
            // if _itemPool for chosen is empty, MissedItemsPerClass will return 0 here (checked at startup)
            if (config.GetMissedItemsPerClass(AuctionQuality(j), ItemClass(i)) > addedItem[j][i])
            {
                ItemToSell miss_item;
                miss_item.Color = j;
                miss_item.Itemclass = i;
                itemsToSellArray.emplace_back(miss_item);
                found = true;
            }
        }
    }

    return found;
}

// Set items price. All important value are passed by address.
void AuctionBotSeller::SetPricesOfItem(ItemTemplate const* itemProto, SellerConfiguration& config, uint32& buyp, uint32& bidp, uint32 stackCount)
{
    uint32 classRatio = config.GetPriceRatioPerClass(ItemClass(itemProto->Class));
    uint32 qualityRatio = config.GetPriceRatioPerQuality(AuctionQuality(itemProto->Quality));
    float priceRatio = (float(classRatio) * float(qualityRatio)) / 10000.0f;

    float buyPrice = itemProto->BuyPrice;
    float sellPrice = itemProto->SellPrice;

    if (buyPrice == 0)
    {
        if (sellPrice > 0)
            buyPrice = sellPrice * GetSellModifier(itemProto);
        else
        {
            float divisor = ((itemProto->Class == ITEM_CLASS_WEAPON || itemProto->Class == ITEM_CLASS_ARMOR) ? 284.0f : 80.0f);
            float tempLevel = (itemProto->ItemLevel == 0 ? 1.0f : itemProto->ItemLevel);
            float tempQuality = (itemProto->Quality == 0 ? 1.0f : itemProto->Quality);

            buyPrice = tempLevel * tempQuality * static_cast<float>(GetBuyModifier(itemProto))* tempLevel / divisor;
        }
    }

    if (sellPrice == 0)
        sellPrice = (buyPrice > 10 ? buyPrice / GetSellModifier(itemProto) : buyPrice);

    if (sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYPRICE_SELLER))
        buyPrice = sellPrice;

    float basePriceFloat = buyPrice * stackCount / (itemProto->Class == 6 ? 200.0f : static_cast<float>(itemProto->BuyCount));
    basePriceFloat *= priceRatio;
    basePriceFloat *= _rateMoney;

    float range = basePriceFloat * 0.04f;

    buyp = static_cast<uint32>(frand(basePriceFloat - range, basePriceFloat + range) + 0.5f);
    if (buyp == 0)
        buyp = 1;

    float bidPercentage = frand(sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BIDPRICE_MIN), sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BIDPRICE_MAX));
    bidp = static_cast<uint32>(bidPercentage * buyp);
    if (bidp == 0)
        bidp = 1;
}

// Determines the stack size to use for the item
uint32 AuctionBotSeller::GetStackSizeForItem(ItemTemplate const* itemProto, SellerConfiguration& config) const
{
    if (config.GetRandomStackRatioPerClass(ItemClass(itemProto->Class)) > urand(0, 99))
        return urand(1, itemProto->GetMaxStackSize());

    return 1;
}

// Determine the multiplier for the sell price of any weapon without a buy price.
uint32 AuctionBotSeller::GetSellModifier(ItemTemplate const* prototype)
{
    switch (prototype->Class)
    {
        case ITEM_CLASS_WEAPON:
        case ITEM_CLASS_ARMOR:
        case ITEM_CLASS_REAGENT:
        case ITEM_CLASS_PROJECTILE:
            return 5;
        default:
            return 4;
    }
}

// Return the modifier by which the item's level and quality will be modified by to derive a relatively accurate price.
uint32 AuctionBotSeller::GetBuyModifier(ItemTemplate const* prototype)
{
    switch (prototype->Class)
    {
        case ITEM_CLASS_CONSUMABLE:
        {
            switch (prototype->SubClass)
            {
            case ITEM_SUBCLASS_CONSUMABLE:
                return 100;
            case ITEM_SUBCLASS_FLASK:
                return 400;
            case ITEM_SUBCLASS_SCROLL:
                return 15;
            case ITEM_SUBCLASS_ITEM_ENHANCEMENT:
                return 250;
            case ITEM_SUBCLASS_BANDAGE:
                return 125;
            default:
                return 300;
            }
        }
        case ITEM_CLASS_WEAPON:
        {
            switch (prototype->SubClass)
            {
                case ITEM_SUBCLASS_WEAPON_AXE:
                case ITEM_SUBCLASS_WEAPON_MACE:
                case ITEM_SUBCLASS_WEAPON_SWORD:
                case ITEM_SUBCLASS_WEAPON_FIST:
                case ITEM_SUBCLASS_WEAPON_DAGGER:
                    return 1200;
                case ITEM_SUBCLASS_WEAPON_AXE2:
                case ITEM_SUBCLASS_WEAPON_MACE2:
                case ITEM_SUBCLASS_WEAPON_POLEARM:
                case ITEM_SUBCLASS_WEAPON_SWORD2:
                case ITEM_SUBCLASS_WEAPON_STAFF:
                    return 1500;
                case ITEM_SUBCLASS_WEAPON_THROWN:
                    return 350;
                default:
                    return 1000;
            }
        }
        case ITEM_CLASS_ARMOR:
        {
            switch (prototype->SubClass)
            {
                case ITEM_SUBCLASS_ARMOR_MISC:
                case ITEM_SUBCLASS_ARMOR_CLOTH:
                    return 500;
                case ITEM_SUBCLASS_ARMOR_LEATHER:
                    return 600;
                case ITEM_SUBCLASS_ARMOR_MAIL:
                    return 700;
                case ITEM_SUBCLASS_ARMOR_PLATE:
                case ITEM_SUBCLASS_ARMOR_SHIELD:
                    return 800;
                default:
                    return 400;
            }
        }
        case ITEM_CLASS_REAGENT:
        case ITEM_CLASS_PROJECTILE:
            return 50;
        case ITEM_CLASS_TRADE_GOODS:
        {
            switch (prototype->SubClass)
            {
                case ITEM_SUBCLASS_TRADE_GOODS:
                case ITEM_SUBCLASS_PARTS:
                case ITEM_SUBCLASS_MEAT:
                    return 50;
                case ITEM_SUBCLASS_EXPLOSIVES:
                    return 250;
                case ITEM_SUBCLASS_DEVICES:
                    return 500;
                case ITEM_SUBCLASS_ELEMENTAL:
                case ITEM_SUBCLASS_TRADE_GOODS_OTHER:
                case ITEM_SUBCLASS_ENCHANTING:
                    return 300;
                default:
                    return 100;
            }
        }
        case ITEM_CLASS_QUEST: return 1000;
        case ITEM_CLASS_KEY: return 3000;
        default:
            return 500;
    }
}

void AuctionBotSeller::SetItemsRatio(uint32 al, uint32 ho, uint32 ne)
{
    sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ALLIANCE_ITEM_AMOUNT_RATIO, std::max(al, 100000u));
    sAuctionBotConfig->SetConfig(CONFIG_AHBOT_HORDE_ITEM_AMOUNT_RATIO, std::max(ho, 100000u));
    sAuctionBotConfig->SetConfig(CONFIG_AHBOT_NEUTRAL_ITEM_AMOUNT_RATIO, std::max(ne, 100000u));

    for (int i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
        LoadItemsQuantity(_houseConfig[i]);
}

void AuctionBotSeller::SetItemsRatioForHouse(AuctionHouseType house, uint32 val)
{
    val = std::max(val, 10000u); // apply same upper limit as used for config load

    switch (house)
    {
        case AUCTION_HOUSE_ALLIANCE: sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ALLIANCE_ITEM_AMOUNT_RATIO, val); break;
        case AUCTION_HOUSE_HORDE:    sAuctionBotConfig->SetConfig(CONFIG_AHBOT_HORDE_ITEM_AMOUNT_RATIO, val); break;
        default:                     sAuctionBotConfig->SetConfig(CONFIG_AHBOT_NEUTRAL_ITEM_AMOUNT_RATIO, val); break;
    }

    LoadItemsQuantity(_houseConfig[house]);
}

void AuctionBotSeller::SetItemsAmount(std::array<uint32, MAX_AUCTION_QUALITY> const& amounts)
{
    sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ITEM_GRAY_AMOUNT, amounts[AUCTION_QUALITY_GRAY]);
    sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ITEM_WHITE_AMOUNT, amounts[AUCTION_QUALITY_WHITE]);
    sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ITEM_GREEN_AMOUNT, amounts[AUCTION_QUALITY_GREEN]);
    sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ITEM_BLUE_AMOUNT, amounts[AUCTION_QUALITY_BLUE]);
    sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ITEM_PURPLE_AMOUNT, amounts[AUCTION_QUALITY_PURPLE]);
    sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ITEM_ORANGE_AMOUNT, amounts[AUCTION_QUALITY_ORANGE]);
    sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ITEM_YELLOW_AMOUNT, amounts[AUCTION_QUALITY_YELLOW]);

    for (int i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
        LoadItemsQuantity(_houseConfig[i]);
}

void AuctionBotSeller::SetItemsAmountForQuality(AuctionQuality quality, uint32 val)
{
    switch (quality)
    {
        case AUCTION_QUALITY_GRAY:
            sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ITEM_GRAY_AMOUNT, val); break;
        case AUCTION_QUALITY_WHITE:
            sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ITEM_WHITE_AMOUNT, val); break;
        case AUCTION_QUALITY_GREEN:
            sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ITEM_GREEN_AMOUNT, val); break;
        case AUCTION_QUALITY_BLUE:
            sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ITEM_BLUE_AMOUNT, val); break;
        case AUCTION_QUALITY_PURPLE:
            sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ITEM_PURPLE_AMOUNT, val); break;
        case AUCTION_QUALITY_ORANGE:
            sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ITEM_ORANGE_AMOUNT, val); break;
        default:
            sAuctionBotConfig->SetConfig(CONFIG_AHBOT_ITEM_YELLOW_AMOUNT, val); break;
    }

    for (int i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
        LoadItemsQuantity(_houseConfig[i]);
}

// Add new auction to one of the factions.
// Faction and setting associated is defined passed argument ( config )
void AuctionBotSeller::AddNewAuctions(SellerConfiguration& config)
{
    uint32 count = 0;
    uint32 items = 0;

    // If there is large amount of items missed we can use boost value to get fast filled AH
    if (config.LastMissedItem > sAuctionBotConfig->GetItemPerCycleBoost())
    {
        items = sAuctionBotConfig->GetItemPerCycleBoost();
        LOG_DEBUG("ahbot", "AHBotSeller: Boost value used to fill AH! (if this happens often adjust both ItemsPerCycle in worldserver.conf)");
    }
    else
        items = sAuctionBotConfig->GetItemPerCycleNormal();

    uint32 houseid = 0;
    switch (config.GetHouseType())
    {
        case AUCTION_HOUSE_ALLIANCE:
            houseid = AUCTIONHOUSE_ALLIANCE;
            break;
        case AUCTION_HOUSE_HORDE:
            houseid = AUCTIONHOUSE_HORDE;
            break;
        default:
            houseid = AUCTIONHOUSE_NEUTRAL;
            break;
    }

    AuctionHouseEntry const* ahEntry = sAuctionHouseStore.LookupEntry(houseid);
    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(config.GetHouseType());

    ItemsToSellArray itemsToSell;
    AllItemsArray allItems(MAX_AUCTION_QUALITY, std::vector<uint32>(MAX_ITEM_CLASS));

    // Main loop
    // getRandomArray will give what categories of items should be added (return true if there is at least 1 items missed)
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    while (GetItemsToSell(config, itemsToSell, allItems) && items > 0)
    {
        --items;

        // Select random position from missed items table
        ItemToSell const& sellItem = Warhead::Containers::SelectRandomContainerElement(itemsToSell);

        // Set itemId with random item ID for selected categories and color, from _itemPool table
        uint32 itemId = Warhead::Containers::SelectRandomContainerElement(_itemPool[sellItem.Color][sellItem.Itemclass]);
        ++allItems[sellItem.Color][sellItem.Itemclass]; // Helper table to avoid rescan from DB in this loop. (has we add item in random orders)

        if (!itemId)
        {
            LOG_DEBUG("ahbot", "AHBotSeller: Item entry 0 auction creating attempt.");
            continue;
        }

        ItemTemplate const* prototype = sObjectMgr->GetItemTemplate(itemId);
        if (!prototype)
        {
            LOG_DEBUG("ahbot", "AHBotSeller: Unknown item {} auction creating attempt.", itemId);
            continue;
        }

        uint32 stackCount = GetStackSizeForItem(prototype, config);

        Item* item = Item::CreateItem(itemId, stackCount);
        if (!item)
        {
            LOG_ERROR("ahbot", "AHBotSeller: Item::CreateItem() returned NULL for item {} (stack: {})", itemId, stackCount);
            return;
        }

        // Update the just created item so that if it needs random properties it has them.
        // Ex:  Notched Shortsword of Stamina will only generate as a Notched Shortsword without this.
        if (int32 randomPropertyId = Item::GenerateItemRandomPropertyId(itemId))
            item->SetItemRandomProperties(randomPropertyId);

        uint32 buyoutPrice;
        uint32 bidPrice = 0;

        // Price of items are set here
        SetPricesOfItem(prototype, config, buyoutPrice, bidPrice, stackCount);

        // Deposit time
        Hours etime;
        switch (urand(1, 3))
        {
            case 1:
                etime = 12h;
                break;
            case 3:
                etime = 2_days;
                break;
            default:
                etime = 1_days;
                break;
        }

        auto auctionEntry = std::make_unique<AuctionEntry>();
        auctionEntry->Id = sObjectMgr->GenerateAuctionID();
        auctionEntry->PlayerOwner = ObjectGuid::Create<HighGuid::Player>(sAuctionBotConfig->GetRandChar());
        auctionEntry->ItemGuid = item->GetGUID();
        auctionEntry->ItemID = item->GetEntry();
        auctionEntry->StartBid = bidPrice;
        auctionEntry->BuyOut = buyoutPrice;
        auctionEntry->HouseId = houseid;
        auctionEntry->Deposit = sAuctionMgr->GetAuctionDeposit(ahEntry, etime, item, stackCount);
        auctionEntry->auctionHouseEntry = ahEntry;
        auctionEntry->ExpireTime = GameTime::GetGameTime() + Hours{ urand(config.GetMinTime().count(), config.GetMaxTime().count()) };

        item->SaveToDB(trans);
        sAuctionMgr->AddAuctionItem(item);

        auctionEntry->SaveToDB(trans);
        auctionHouse->AddAuction(std::move(auctionEntry));
        ++count;
    }

    CharacterDatabase.CommitTransaction(trans);
    LOG_DEBUG("ahbot", "AHBotSeller: Added {} items to auction", count);
}

bool AuctionBotSeller::Update(AuctionHouseType houseType)
{
    if (sAuctionBotConfig->GetConfigItemAmountRatio(houseType))
    {
        LOG_DEBUG("ahbot", "AHBotSeller: {} selling...", AuctionBotConfig::GetHouseTypeName(houseType));

        if (SetStat(_houseConfig[houseType]))
            AddNewAuctions(_houseConfig[houseType]);

        return true;
    }

    return false;
}