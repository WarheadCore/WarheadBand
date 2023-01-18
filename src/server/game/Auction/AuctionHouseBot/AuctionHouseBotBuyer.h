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

#ifndef AUCTION_HOUSE_BOT_BUYER_H
#define AUCTION_HOUSE_BOT_BUYER_H

#include "Define.h"
#include "AuctionHouseMgr.h"
#include "AuctionHouseBot.h"

struct BuyerAuctionEval
{
    uint32 AuctionId{};
    time_t LastChecked{};
    time_t LastExist{};
};

struct BuyerItemInfo
{
    uint32 BidItemCount{};
    uint32 BuyItemCount{};
    uint32 MinBuyPrice{};
    uint32 MinBidPrice{};
    double TotalBuyPrice{};
    double TotalBidPrice{};
};

using BuyerItemInfoMap = std::map<uint32, BuyerItemInfo>;
using CheckEntryMap = std::map<uint32, BuyerAuctionEval>;

struct BuyerConfiguration
{
    inline void Initialize(AuctionHouseType houseType)
    {
        _houseType = houseType;
    }

    [[nodiscard]] AuctionHouseType GetHouseType() const { return _houseType; }

    BuyerItemInfoMap SameItemInfo;
    CheckEntryMap EligibleItems;
    bool BuyerEnabled{};

private:
    AuctionHouseType _houseType{ AUCTION_HOUSE_NEUTRAL };
};

// This class handle all Buyer method
// (holder of AuctionBotConfig for each auction house type)
class WH_GAME_API AuctionBotBuyer : public AuctionBotAgent
{
public:
    AuctionBotBuyer();
    ~AuctionBotBuyer() override = default;

    bool Initialize() override;
    bool Update(AuctionHouseType houseType) override;

    void LoadConfig();
    void BuyAndBidItems(BuyerConfiguration& config);

private:
    uint32 _checkInterval;
    BuyerConfiguration _houseConfig[MAX_AUCTION_HOUSE_TYPE];

    void LoadBuyerValues(BuyerConfiguration& config);

    // ahInfo can be NULL
    bool RollBuyChance(BuyerItemInfo const* ahInfo, Item const* item, AuctionEntry const* auction, uint32 bidPrice);
    bool RollBidChance(BuyerItemInfo const* ahInfo, Item const* item, AuctionEntry const* auction, uint32 bidPrice);
    static void PlaceBidToEntry(AuctionEntry* auction, uint32 bidPrice);
    static void BuyEntry(AuctionEntry* auction, AuctionHouseObject* auctionHouse);
    void PrepareListOfEntry(BuyerConfiguration& config);
    uint32 GetItemInformation(BuyerConfiguration& config);
    static uint32 GetVendorPrice(uint32 quality);
    static uint32 GetChanceMultiplier(uint32 quality);
};

#endif
