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

#include "AsyncAuctionOperation.h"
#include "AuctionHousePackets.h"
#include "Player.h"
#include "Creature.h"
#include "AuctionHouseMgr.h"

void ListOwnerTask::Execute()
{
    if (Player* player = ObjectAccessor::FindPlayer(_playerGuid))
        player->GetSession()->HandleAuctionListOwnerItemsEvent(_creatureGuid);
}

void ListItemsTask::Execute()
{
    Player* player = ObjectAccessor::FindPlayer(_listItems->PlayerGuid);
    if (!player || !player->IsInWorld() || player->IsDuringRemoveFromWorld() || player->IsBeingTeleported())
        return;

    Creature* creature = player->GetNPCIfCanInteractWith(_listItems->CreatureGuid, UNIT_NPC_FLAG_AUCTIONEER);
    if (!creature)
        return;

    WorldPackets::AuctionHouse::ListResult packetSend;
    if (!packetSend.IsCorrectSearchedName(_listItems->SearchedName))
    {
        LOG_ERROR("server", "if (!packetSend.IsCorrectSearchedName(_listItems->SearchedName))");
        return;
    }

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(creature->GetFaction());
    bool result = auctionHouse->BuildListAuctionItems(packetSend, player, _listItems);
    if (!result)
        return;

    player->SendDirectMessage(packetSend.Write());
}
