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

struct AuctionListItems;

class WH_GAME_API AsyncAuctionOperation
{
public:
    AsyncAuctionOperation() = default;
    virtual ~AsyncAuctionOperation() = default;

    virtual void Execute() = 0;

private:
    AsyncAuctionOperation(AsyncAuctionOperation const& right) = delete;
    AsyncAuctionOperation& operator=(AsyncAuctionOperation const& right) = delete;
};

class WH_GAME_API ListOwnerTask : public AsyncAuctionOperation
{
public:
    ListOwnerTask(ObjectGuid playerGuid, ObjectGuid creatureGuid) :
        _playerGuid(playerGuid), _creatureGuid(creatureGuid) { }

    ~ListOwnerTask() override = default;

    void Execute() override;

private:
    ObjectGuid _playerGuid;
    ObjectGuid _creatureGuid;
};

class WH_GAME_API ListItemsTask : public AsyncAuctionOperation
{
public:
    ListItemsTask(std::shared_ptr<AuctionListItems> listItems) :
        _listItems(std::move(listItems)) { }

    ~ListItemsTask() override = default;

    void Execute() override;

private:
    std::shared_ptr<AuctionListItems> _listItems;
};

#endif
