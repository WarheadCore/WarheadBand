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

#ifndef WARHEAD_AUCTION_FWD_H_
#define WARHEAD_AUCTION_FWD_H_

#include "ObjectGuid.h"
#include "Duration.h"
#include <vector>

constexpr Seconds MIN_AUCTION_TIME = 12h;
constexpr auto MAX_AUCTION_ITEMS = 160;

constexpr bool IsCorrectAuctionTime(Seconds auctionTime)
{
    return auctionTime == MIN_AUCTION_TIME || auctionTime == MIN_AUCTION_TIME * 2 || auctionTime == MIN_AUCTION_TIME * 4;
}

enum class AuctionSortOrder
{
    MinLevel,
    Rarity,
    BuyOut,
    TimeLeft,
    Unk4,
    Item,
    MinBidBuy,
    Owner,
    Bid,
    Stack,
    BuyOut2,

    Max
};

struct AuctionSortInfo
{
    AuctionSortOrder SortOrder{ AuctionSortOrder::Max };
    bool IsDesc{ true };
};

using AuctionSortOrderVector = std::vector<AuctionSortInfo>;

struct WH_GAME_API AuctionListItems
{
    ObjectGuid PlayerGuid;
    ObjectGuid CreatureGuid;
    std::string SearchedName;
    uint32 ListFrom{};
    uint8 LevelMin{};
    uint8 LevelMax{};
    uint8 Usable{};
    uint32 InventoryType{};
    uint32 ItemClass{};
    uint32 ItemSubClass{};
    uint32 Quality{};
    uint8 GetAll{};
    AuctionSortOrderVector SortOrder;

    uint32 Count{};
    uint32 TotalCount{};

    bool IsNoFilter() const;
};

enum AuctionError
{
    ERR_AUCTION_OK                  = 0,
    ERR_AUCTION_INVENTORY           = 1,
    ERR_AUCTION_DATABASE_ERROR      = 2,
    ERR_AUCTION_NOT_ENOUGHT_MONEY   = 3,
    ERR_AUCTION_ITEM_NOT_FOUND      = 4,
    ERR_AUCTION_HIGHER_BID          = 5,
    ERR_AUCTION_BID_INCREMENT       = 7,
    ERR_AUCTION_BID_OWN             = 10,
    ERR_AUCTION_RESTRICTED_ACCOUNT  = 13
};

enum AuctionAction
{
    AUCTION_SELL_ITEM   = 0,
    AUCTION_CANCEL      = 1,
    AUCTION_PLACE_BID   = 2
};

enum MailAuctionAnswers
{
    AUCTION_OUTBIDDED           = 0,
    AUCTION_WON                 = 1,
    AUCTION_SUCCESSFUL          = 2,
    AUCTION_EXPIRED             = 3,
    AUCTION_CANCELLED_TO_BIDDER = 4,
    AUCTION_CANCELED            = 5,
    AUCTION_SALE_PENDING        = 6
};

enum AuctionHouses
{
    AUCTIONHOUSE_ALLIANCE       = 2,
    AUCTIONHOUSE_HORDE          = 6,
    AUCTIONHOUSE_NEUTRAL        = 7
};

#endif
