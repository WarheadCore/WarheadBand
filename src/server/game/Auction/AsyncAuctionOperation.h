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

#ifndef WARHEAD_ASYNC_AUCTION_OPERATION_H_
#define WARHEAD_ASYNC_AUCTION_OPERATION_H_

#include "ObjectGuid.h"
#include <memory>
#include <utility>

class Player;

struct AuctionListItems;
struct AuctionSellItem;

class WH_GAME_API AsyncAuctionOperation
{
public:
    explicit AsyncAuctionOperation(ObjectGuid playerGuid) :
        _playerGuid(playerGuid) { }

    virtual ~AsyncAuctionOperation() = default;

    virtual void Execute() = 0;

    [[nodiscard]] ObjectGuid GetPlayerGUID() const { return _playerGuid; }
    [[nodiscard]] Player* GetPlayer() const;

private:
    ObjectGuid _playerGuid;

    AsyncAuctionOperation(AsyncAuctionOperation const& right) = delete;
    AsyncAuctionOperation& operator=(AsyncAuctionOperation const& right) = delete;
};

class WH_GAME_API SellItemTask : public AsyncAuctionOperation
{
public:
    SellItemTask(ObjectGuid playerGuid, std::shared_ptr<AuctionSellItem> packet) :
        AsyncAuctionOperation(playerGuid), _packet(std::move(packet)) { }

    ~SellItemTask() override = default;

    void Execute() override;

private:
    std::shared_ptr<AuctionSellItem> _packet;
};

class WH_GAME_API PlaceBidTask : public AsyncAuctionOperation
{
public:
    PlaceBidTask(ObjectGuid playerGuid, ObjectGuid auctioneer, uint32 auctionID, uint32 price) :
        AsyncAuctionOperation(playerGuid), _auctioneer(auctioneer), _auctionID(auctionID), _price(price) { }

    ~PlaceBidTask() override = default;

    void Execute() override;

private:
    ObjectGuid _auctioneer;
    uint32 _auctionID{};
    uint32 _price{};
};

class WH_GAME_API ListBidderItemsTask : public AsyncAuctionOperation
{
public:
    ListBidderItemsTask(ObjectGuid playerGuid, ObjectGuid auctioneer, uint32 listFrom, uint32 outbiddedCount, std::vector<uint32>& outbiddedAuctionIds) :
        AsyncAuctionOperation(playerGuid),
        _auctioneer(auctioneer),
        _listFrom(listFrom),
        _outbiddedCount(outbiddedCount),
        _outbiddedAuctionIds(std::move(outbiddedAuctionIds)) { }

    ~ListBidderItemsTask() override = default;

    void Execute() override;

private:
    ObjectGuid _auctioneer;
    uint32 _listFrom{};
    uint32 _outbiddedCount{};
    std::vector<uint32> _outbiddedAuctionIds{};
};

class WH_GAME_API ListOwnerTask : public AsyncAuctionOperation
{
public:
    ListOwnerTask(ObjectGuid playerGuid, ObjectGuid creatureGuid) :
        AsyncAuctionOperation(playerGuid), _creatureGuid(creatureGuid) { }

    ~ListOwnerTask() override = default;

    void Execute() override;

private:
    ObjectGuid _creatureGuid;
};

class WH_GAME_API ListItemsTask : public AsyncAuctionOperation
{
public:
    explicit ListItemsTask(ObjectGuid playerGuid, std::shared_ptr<AuctionListItems> packet) :
        AsyncAuctionOperation(playerGuid), _packet(std::move(packet)) { }

    ~ListItemsTask() override = default;

    void Execute() override;

private:
    std::shared_ptr<AuctionListItems> _packet;
};

class WH_GAME_API AhBotTask : public AsyncAuctionOperation
{
public:
    AhBotTask() : AsyncAuctionOperation(ObjectGuid::Empty) { }
    ~AhBotTask() override = default;

    void Execute() override;
};

#endif
