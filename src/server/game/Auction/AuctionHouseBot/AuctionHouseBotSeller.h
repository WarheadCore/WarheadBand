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

#ifndef AUCTION_HOUSE_BOT_SELLER_H
#define AUCTION_HOUSE_BOT_SELLER_H

#include "Define.h"
#include "ItemTemplate.h"
#include "AuctionHouseBot.h"

struct ItemToSell
{
    uint32 Color{};
    uint32 Itemclass{};
};

using ItemsToSellArray = std::vector<ItemToSell>;
using AllItemsArray = std::vector<std::vector<uint32>>;

struct SellerItemInfo
{
    uint32 AmountOfItems{};
    uint32 MissItems{};
};

struct SellerItemClassSharedInfo
{
    uint32 PriceRatio{};
    uint32 RandomStackRatio{ 100 };
};

struct SellerItemQualitySharedInfo
{
    uint32 AmountOfItems{};
    uint32 PriceRatio{};
};

class SellerConfiguration
{
public:
    inline void Initialize(AuctionHouseType houseType)
    {
        _houseType = houseType;
    }

    [[nodiscard]] AuctionHouseType GetHouseType() const { return _houseType; }

    uint32 LastMissedItem{};

    void SetMinTime(Seconds value)
    {
        _minTime = value;
    }

    [[nodiscard]] Seconds GetMinTime() const
    {
        return std::min(1s, std::min(_minTime, _maxTime));
    }

    void SetMaxTime(Seconds value) { _maxTime = value; }
    [[nodiscard]] Seconds GetMaxTime() const { return _maxTime; }

    // Data access classified by item class and item quality
    void SetItemsAmountPerClass(AuctionQuality quality, ItemClass itemClass, uint32 amount) { _itemInfo[quality][itemClass].AmountOfItems = amount; }
    [[nodiscard]] uint32 GetItemsAmountPerClass(AuctionQuality quality, ItemClass itemClass) const { return _itemInfo[quality][itemClass].AmountOfItems; }

    void SetMissedItemsPerClass(AuctionQuality quality, ItemClass itemClass, uint32 found)
    {
        if (_itemInfo[quality][itemClass].AmountOfItems > found)
            _itemInfo[quality][itemClass].MissItems = _itemInfo[quality][itemClass].AmountOfItems - found;
        else
            _itemInfo[quality][itemClass].MissItems = 0;
    }

    [[nodiscard]] uint32 GetMissedItemsPerClass(AuctionQuality quality, ItemClass itemClass) const { return _itemInfo[quality][itemClass].MissItems; }

    // Data for every quality of item
    void SetItemsAmountPerQuality(AuctionQuality quality, uint32 cnt) { _itemSharedQualityInfo[quality].AmountOfItems = cnt; }
    [[nodiscard]] uint32 GetItemsAmountPerQuality(AuctionQuality quality) const { return _itemSharedQualityInfo[quality].AmountOfItems; }

    void SetPriceRatioPerQuality(AuctionQuality quality, uint32 value) { _itemSharedQualityInfo[quality].PriceRatio = value; }
    [[nodiscard]] uint32 GetPriceRatioPerQuality(AuctionQuality quality) const { return _itemSharedQualityInfo[quality].PriceRatio; }

    // data for every class of item
    void SetPriceRatioPerClass(ItemClass itemClass, uint32 value) { _itemSharedClassInfo[itemClass].PriceRatio = value; }
    [[nodiscard]] uint32 GetPriceRatioPerClass(ItemClass itemClass) const { return _itemSharedClassInfo[itemClass].PriceRatio; }

    void SetRandomStackRatioPerClass(ItemClass itemClass, uint32 value) { _itemSharedClassInfo[itemClass].RandomStackRatio = value; }
    [[nodiscard]] uint32 GetRandomStackRatioPerClass(ItemClass itemClass) const { return _itemSharedClassInfo[itemClass].RandomStackRatio; }

private:
    AuctionHouseType _houseType{ AUCTION_HOUSE_NEUTRAL };
    Seconds _minTime{ 1h };
    Seconds _maxTime{ 72h };

    std::array<std::array<SellerItemInfo, MAX_ITEM_CLASS>, MAX_AUCTION_QUALITY> _itemInfo{};
    std::array<SellerItemQualitySharedInfo, MAX_ITEM_QUALITY> _itemSharedQualityInfo{};
    std::array<SellerItemClassSharedInfo, MAX_ITEM_CLASS> _itemSharedClassInfo{};
};

// This class handle all Selling method
// (holder of AHB_Seller_Config data for each auction house type)
class WH_GAME_API AuctionBotSeller : public AuctionBotAgent
{
public:
    AuctionBotSeller();
    ~AuctionBotSeller() override = default;

    bool Initialize() override;
    bool Update(AuctionHouseType houseType) override;

    void AddNewAuctions(SellerConfiguration& config);
    void SetItemsRatio(uint32 al, uint32 ho, uint32 ne);
    void SetItemsRatioForHouse(AuctionHouseType house, uint32 val);
    void SetItemsAmount(std::array<uint32, MAX_AUCTION_QUALITY> const& amounts);
    void SetItemsAmountForQuality(AuctionQuality quality, uint32 val);
    void LoadConfig();

private:
    void LoadSellerValues(SellerConfiguration& config);
    uint32 SetStat(SellerConfiguration& config);
    bool GetItemsToSell(SellerConfiguration& config, ItemsToSellArray& itemsToSellArray, AllItemsArray const& addedItem);
    void SetPricesOfItem(ItemTemplate const* itemProto, SellerConfiguration& config, uint32& buyp, uint32& bidp, uint32 stackcnt);
    uint32 GetStackSizeForItem(ItemTemplate const* itemProto, SellerConfiguration& config) const;
    void LoadItemsQuantity(SellerConfiguration& config);
    static uint32 GetBuyModifier(ItemTemplate const* prototype);
    static uint32 GetSellModifier(ItemTemplate const* itemProto);

    std::array<SellerConfiguration, MAX_AUCTION_HOUSE_TYPE> _houseConfig{};
    std::array<std::array<std::vector<uint32>, MAX_ITEM_CLASS>, MAX_AUCTION_QUALITY> _itemPool{};
    float _rateMoney{ 1.f };
};

#endif
