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

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "AsyncAuctionMgr.h"
#include "AuctionHousePackets.h"
#include "AuctionHouseMgr.h"
#include "Chat.h"
#include "ChatTextBuilder.h"
#include "DatabaseEnv.h"
#include "GameConfig.h"
#include "GameTime.h"
#include "Language.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "UpdateFields.h"
#include "WorldPacket.h"
#include "WorldSession.h"

// Called when player click on auctioneer npc
void WorldSession::HandleAuctionHelloOpcode(WorldPackets::AuctionHouse::HelloFromClient& packet)
{
    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(packet.CreatureGuid, UNIT_NPC_FLAG_AUCTIONEER);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionHelloOpcode - Unit ({}) not found or you can't interact with him.", packet.CreatureGuid.ToString());
        return;
    }

    // Remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    SendAuctionHello(packet.CreatureGuid, unit);
}

// This void causes that auction window is opened
void WorldSession::SendAuctionHello(ObjectGuid guid, Creature* unit)
{
    if (GetPlayer()->getLevel() < CONF_GET_INT("LevelReq.Auction"))
    {
        Warhead::Text::SendNotification(this, LANG_AUCTION_REQ, CONF_GET_INT("LevelReq.Auction"));
        return;
    }

    if (!sScriptMgr->CanSendAuctionHello(this, guid, unit))
        return;

    AuctionHouseEntry const* auctionHouseEntry = AuctionHouseMgr::GetAuctionHouseEntry(unit->GetFaction());
    if (!auctionHouseEntry)
    {
        LOG_DEBUG("network.packet", "{}: - Not found AuctionHouseEntry for creature {}", __FUNCTION__, guid);
        return;
    }

    WorldPackets::AuctionHouse::HelloToClient packetSend;
    packetSend.CreatureGuid = guid;
    packetSend.HouseID = auctionHouseEntry->houseId;

    SendPacket(packetSend.Write());
}

// Call this method when player bids, creates, or deletes auction
void WorldSession::SendAuctionCommandResult(AuctionEntry const* auction, uint32 action, uint32 errorCode, uint32 inventoryError /*= 0*/)
{
    WorldPackets::AuctionHouse::CommandResult packet{ auction };
    packet.Action = action;
    packet.ErrorCode = errorCode;
    packet.InventoryError = inventoryError;

    SendPacket(packet.Write());
}

// Sends notification, if Bidder is online
void WorldSession::SendAuctionBidderNotification(uint32 location, uint32 auctionId, ObjectGuid Bidder, uint32 bidSum, uint32 diff, uint32 ItemID)
{
    WorldPackets::AuctionHouse::BidderNotification packet;
    packet.Location = location;
    packet.AuctionID = auctionId;
    packet.Bidder = Bidder;
    packet.BidSum = bidSum;
    packet.Diff = diff;
    packet.ItemEntry = ItemID;

    SendPacket(packet.Write());
}

// This void causes on client to display: "Your auction sold"
void WorldSession::SendAuctionOwnerNotification(AuctionEntry* auction)
{
    WorldPackets::AuctionHouse::OwnerNotification packet;
    packet.ID = auction->Id;
    packet.Bid = auction->Bid;
    packet.ItemEntry = auction->ItemID;

    SendPacket(packet.Write());
}

// Creates new auction and adds auction to some auctionhouse
void WorldSession::HandleAuctionSellItem(WorldPackets::AuctionHouse::SellItem& packet)
{
    if (packet.ItemsCount > MAX_AUCTION_ITEMS)
    {
        SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
        return;
    }

    for (uint32 i = 0; i < packet.ItemsCount; ++i)
    {
        auto const& [itemGuid, itemCount] = packet.Items[i];
        if (!itemGuid || !itemCount || itemCount > 1000)
        {
            SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_ITEM_NOT_FOUND);
            return;
        }
    }

    if (!packet.Bid || !packet.ExpireTime)
    {
        SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
        return;
    }

    if (packet.Bid > MAX_MONEY_AMOUNT || packet.Buyout > MAX_MONEY_AMOUNT)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionSellItem - Player {} ({}) attempted to sell item with higher price than max gold amount.",
            _player->GetName(), _player->GetGUID().ToString());
        SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
        return;
    }

    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(packet.Auctioneer, UNIT_NPC_FLAG_AUCTIONEER);
    if (!creature)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionSellItem - Unit ({}) not found or you can't interact with him.", packet.Auctioneer.ToString());
        SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
        return;
    }

    AuctionHouseEntry const* auctionHouseEntry = AuctionHouseMgr::GetAuctionHouseEntry(creature->GetFaction());
    if (!auctionHouseEntry)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionSellItem - Unit ({}) has wrong faction.", packet.Auctioneer.ToString());
        SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
        return;
    }

    Seconds depositTime{ Minutes{ packet.ExpireTime } };
    if (depositTime != 12h && depositTime != 1_days && depositTime != 2_days)
    {
        LOG_ERROR("network", "HandleAuctionSellItem: Incorrect deposit time: {}. Player info: {}", Warhead::Time::ToTimeString(depositTime), GetPlayerInfo());
        SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
        return;
    }

    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    std::array<Item*, MAX_AUCTION_ITEMS> items{};
//    items.fill(nullptr);

    uint32 finalCount = 0;
    uint32 itemEntry = 0;

    for (uint32 i = 0; i < packet.ItemsCount; ++i)
    {
        auto const& [itemGuid, itemCount] = packet.Items[i];

        Item* item = _player->GetItemByGuid(itemGuid);
        if (!item)
        {
            SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_ITEM_NOT_FOUND);
            return;
        }

        if (!itemEntry)
            itemEntry = item->GetTemplate()->ItemId;

        if (sAuctionMgr->GetAuctionItem(item->GetGUID()))
        {
            SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_ITEM_NOT_FOUND);
            return;
        }

        if (!item->CanBeTraded())
        {
            SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_INVENTORY, EQUIP_ERR_CANNOT_TRADE_THAT);
            return;
        }

        if (item->IsNotEmptyBag())
        {
            SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_INVENTORY, EQUIP_ERR_CAN_ONLY_DO_WITH_EMPTY_BAGS);
            return;
        }

        if (item->GetTemplate()->Flags & ITEM_FLAG_CONJURED || item->GetUInt32Value(ITEM_FIELD_DURATION))
        {
            SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_INVENTORY, EQUIP_ERR_CANNOT_TRADE_THAT);
            return;
        }

        if (item->GetCount() < itemCount || itemEntry != item->GetTemplate()->ItemId)
        {
            SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_ITEM_NOT_FOUND);
            return;
        }

        items[i] = item;
        finalCount += itemCount;
    }

    if (!finalCount)
    {
        SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_ITEM_NOT_FOUND);
        return;
    }

    // Check if there are 2 identical guids, in this case user is most likely cheating
    for (uint32 i = 0; i < packet.ItemsCount - 1; ++i)
    {
        for (uint32 j = i + 1; j < packet.ItemsCount; ++j)
        {
            if (packet.Items[i].first == packet.Items[j].first)
            {
                SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_ITEM_NOT_FOUND);
                return;
            }
        }
    }

    for (uint32 i = 0; i < packet.ItemsCount; ++i)
    {
        Item* item = items[i];
        if (!item || item->GetMaxStackCount() < finalCount)
        {
            SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_ITEM_NOT_FOUND);
            return;
        }
    }

    for (uint32 i = 0; i < packet.ItemsCount; ++i)
    {
        Item* item = items[i];

        auto auctionTime = Seconds{ std::size_t{ float(depositTime.count()) * CONF_GET_FLOAT("Rate.Auction.Time") }};
        AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(creature->GetFaction());

        uint32 Deposit = sAuctionMgr->GetAuctionDeposit(auctionHouseEntry, Minutes{ packet.ExpireTime }, item, finalCount);
        if (!_player->HasEnoughMoney(Deposit))
        {
            SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_NOT_ENOUGHT_MONEY);
            return;
        }

        _player->ModifyMoney(-int32(Deposit));

        auto auction = std::make_unique<AuctionEntry>();
        auction->Id = sObjectMgr->GenerateAuctionID();

        if (CONF_GET_BOOL("AllowTwoSide.Interaction.Auction"))
            auction->HouseId = AUCTIONHOUSE_NEUTRAL;
        else
        {
            const AuctionHouseEntry* AHEntry = sAuctionMgr->GetAuctionHouseEntry(creature->GetCreatureTemplate()->faction);
            auction->HouseId = AHEntry->houseId;
        }

        auction->PlayerOwner = _player->GetGUID();
        auction->StartBid = packet.Bid;
        auction->BuyOut = packet.Buyout;
        auction->BuyOut = packet.Buyout;
        auction->ExpireTime = GameTime::GetGameTime() + auctionTime;
        auction->Deposit = Deposit;
        auction->auctionHouseEntry = auctionHouseEntry;

        // Required stack size of auction matches to current item stack size, just move item to auctionhouse
        if (packet.ItemsCount == 1 && item->GetCount() == packet.Items[i].second)
        {
            auction->ItemGuid = item->GetGUID();
            auction->ItemID = item->GetEntry();
            auction->ItemCount = item->GetCount();

            LOG_DEBUG("network.opcode", "CMSG_AUCTION_SELL_ITEM: Player {} ({}) is selling item {} entry {} ({}) with count {} with initial Bid {} with BuyOut {} and with time {} (in sec) in auctionhouse {}",
                _player->GetName(), _player->GetGUID().ToString(), item->GetTemplate()->Name1, item->GetEntry(), item->GetGUID().ToString(), item->GetCount(), packet.Bid, packet.Buyout, auctionTime.count(), auction->GetHouseId());

            _player->MoveItemFromInventory(item->GetBagSlot(), item->GetSlot(), true);

            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            item->DeleteFromInventoryDB(trans);
            item->SaveToDB(trans);

            auction->SaveToDB(trans);
            _player->SaveInventoryAndGoldToDB(trans);

            CharacterDatabase.CommitTransaction(trans);

            SendAuctionCommandResult(auction.get(), AUCTION_SELL_ITEM, ERR_AUCTION_OK);

            sAuctionMgr->AddAuctionItem(item);
            auctionHouse->AddAuction(std::move(auction));

            _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION, 1);
            return;
        }
        else // Required stack size of auction does not match to current item stack size, clone item and set correct stack size
        {
            Item* newItem = item->CloneItem(finalCount, _player);
            if (!newItem)
            {
                LOG_ERROR("network.opcode", "CMSG_AUCTION_SELL_ITEM: Could not create clone of item {}", item->GetEntry());
                SendAuctionCommandResult(nullptr, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
                return;
            }

            auction->ItemGuid = newItem->GetGUID();
            auction->ItemID = newItem->GetEntry();
            auction->ItemCount = newItem->GetCount();

            LOG_DEBUG("network.opcode", "CMSG_AUCTION_SELL_ITEM: Player {} ({}) is selling item {} entry {} ({}) with count {} with initial Bid {} with BuyOut {} and with time {} (in sec) in auctionhouse {}",
                _player->GetName(), _player->GetGUID().ToString(), newItem->GetTemplate()->Name1, newItem->GetEntry(), newItem->GetGUID().ToString(), newItem->GetCount(), packet.Bid, packet.Buyout, auctionTime.count(), auction->GetHouseId());

            for (uint32 j = 0; j < packet.ItemsCount; ++j)
            {
                Item* item2 = items[j];

                // Item stack count equals required count, ready to delete item - cloned item will be used for auction
                if (item2->GetCount() == packet.Items[j].second)
                {
                    _player->MoveItemFromInventory(item2->GetBagSlot(), item2->GetSlot(), true);

                    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                    item2->DeleteFromInventoryDB(trans);
                    item2->DeleteFromDB(trans);
                    CharacterDatabase.CommitTransaction(trans);
                    delete item2;
                }
                else // Item stack count is bigger than required count, update item stack count and save to database - cloned item will be used for auction
                {
                    item2->SetCount(item2->GetCount() - packet.Items[j].second);
                    item2->SetState(ITEM_CHANGED, _player);
                    _player->ItemRemovedQuestCheck(item2->GetEntry(), packet.Items[j].second);
                    item2->SendUpdateToPlayer(_player);

                    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                    item2->SaveToDB(trans);
                    CharacterDatabase.CommitTransaction(trans);
                }
            }

            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            newItem->SaveToDB(trans);
            auction->SaveToDB(trans);
            _player->SaveInventoryAndGoldToDB(trans);
            CharacterDatabase.CommitTransaction(trans);

            SendAuctionCommandResult(auction.get(), AUCTION_SELL_ITEM, ERR_AUCTION_OK);

            sAuctionMgr->AddAuctionItem(newItem);
            auctionHouse->AddAuction(std::move(auction));

            _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION, 1);
            return;
        }
    }
}

// Called when client bids or buys out auction
void WorldSession::HandleAuctionPlaceBid(WorldPackets::AuctionHouse::PlaceBid& packet)
{
    LOG_DEBUG("network", "WORLD: Received CMSG_AUCTION_PLACE_BID");

    if (!packet.AuctionID || !packet.Price)
        return; // Check for cheaters

    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(packet.Auctioneer, UNIT_NPC_FLAG_AUCTIONEER);
    if (!creature)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionPlaceBid - Unit ({}) not found or you can't interact with him.", packet.Auctioneer.ToString());
        return;
    }

    // Remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(creature->GetFaction());
    AuctionEntry* auction = auctionHouse->GetAuction(packet.AuctionID);
    Player* player = GetPlayer();

    if (!auction || auction->PlayerOwner == player->GetGUID())
    {
        //you cannot Bid your own auction:
        SendAuctionCommandResult(nullptr, AUCTION_PLACE_BID, ERR_AUCTION_BID_OWN);
        return;
    }

    // impossible have online own another character (use this for speedup check in case online owner)
    Player* auction_owner = ObjectAccessor::FindConnectedPlayer(auction->PlayerOwner);
    if (!auction_owner && sCharacterCache->GetCharacterAccountIdByGuid(auction->PlayerOwner) == GetAccountId())
    {
        //you cannot Bid your another character auction:
        SendAuctionCommandResult(nullptr, AUCTION_PLACE_BID, ERR_AUCTION_BID_OWN);
        return;
    }

    // cheating
    if (packet.Price <= auction->Bid || packet.Price < auction->StartBid)
    {
        // client test but possible in result lags
        SendAuctionCommandResult(auction, AUCTION_PLACE_BID, ERR_AUCTION_HIGHER_BID);
    }

    // price too low for next Bid if not BuyOut
    if ((packet.Price < auction->BuyOut || auction->BuyOut == 0) &&
        packet.Price < auction->Bid + auction->GetAuctionOutBid())
    {
        // Auction has already higher Bid, client tests it!
        // client test but possible in result lags
        SendAuctionCommandResult(auction, AUCTION_PLACE_BID, ERR_AUCTION_BID_INCREMENT);
        return;
    }

    if (!player->HasEnoughMoney(packet.Price))
        return;

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    if (packet.Price < auction->BuyOut || auction->BuyOut == 0)
    {
        if (auction->Bidder)
        {
            if (auction->Bidder == player->GetGUID())
                player->ModifyMoney(-int32(packet.Price - auction->Bid));
            else
            {
                // mail to last Bidder and return money
                sAuctionMgr->SendAuctionOutbiddedMail(auction, packet.Price, GetPlayer(), trans);
                player->ModifyMoney(-int32(packet.Price));
            }
        }
        else
            player->ModifyMoney(-int32(packet.Price));

        auction->Bidder = player->GetGUID();
        auction->Bid = packet.Price;
        _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID, packet.Price);

        CharacterDatabasePreparedStatement stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_AUCTION_BID);
        stmt->SetData(0, auction->Bidder.GetCounter());
        stmt->SetData(1, auction->Bid);
        stmt->SetData(2, auction->Id);
        trans->Append(stmt);

        SendAuctionCommandResult(auction, AUCTION_PLACE_BID, ERR_AUCTION_OK);
    }
    else
    {
        // Buyout:
        if (player->GetGUID() == auction->Bidder)
            player->ModifyMoney(-int32(auction->BuyOut - auction->Bid));
        else
        {
            player->ModifyMoney(-int32(auction->BuyOut));

            if (auction->Bidder) // Buyout for Bid auction
                sAuctionMgr->SendAuctionOutbiddedMail(auction, auction->BuyOut, GetPlayer(), trans);
        }

        auction->Bidder = player->GetGUID();
        auction->Bid = auction->BuyOut;

        _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID, auction->BuyOut);

        //- Mails must be under transaction control too to prevent data loss
        sAuctionMgr->SendAuctionSalePendingMail(auction, trans);
        sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);
        sAuctionMgr->SendAuctionWonMail(auction, trans);
        sScriptMgr->OnAuctionSuccessful(auctionHouse, auction);

        SendAuctionCommandResult(auction, AUCTION_PLACE_BID, ERR_AUCTION_OK);

        auction->DeleteFromDB(trans);

        sAuctionMgr->RemoveAItem(auction->ItemGuid);
        auctionHouse->RemoveAuction(auction);
    }

    player->SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);
}

// Called when auction_owner cancels his auction
void WorldSession::HandleAuctionRemoveItem(WorldPackets::AuctionHouse::RemoveItem& packet)
{
    LOG_DEBUG("network", "WORLD: Received CMSG_AUCTION_REMOVE_ITEM");

    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(packet.Auctioneer, UNIT_NPC_FLAG_AUCTIONEER);
    if (!creature)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionRemoveItem - Unit ({}) not found or you can't interact with him.", packet.Auctioneer.ToString());
        return;
    }

    // Remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(creature->GetFaction());
    AuctionEntry* auction = auctionHouse->GetAuction(packet.AuctionID);
    Player* player = GetPlayer();

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    if (auction && auction->PlayerOwner == player->GetGUID())
    {
        Item* pItem = sAuctionMgr->GetAuctionItem(auction->ItemGuid);
        if (pItem)
        {
            if (auction->Bidder) // If we have a Bidder, we have to send him the money he paid
            {
                uint32 auctionCut = auction->GetAuctionCut();
                if (!player->HasEnoughMoney(auctionCut))          //player doesn't have enough money, maybe message needed
                    return;
                //some auctionBidderNotification would be needed, but don't know that parts..
                sAuctionMgr->SendAuctionCancelledToBidderMail(auction, trans);
                player->ModifyMoney(-int32(auctionCut));
            }

            // item will deleted or added to received mail list
            MailDraft(auction->BuildAuctionMailSubject(AUCTION_CANCELED), AuctionEntry::BuildAuctionMailBody(ObjectGuid::Empty, 0, auction->BuyOut, auction->Deposit))
            .AddItem(pItem)
            .SendMailTo(trans, player, auction, MAIL_CHECK_MASK_COPIED);
        }
        else
        {
            LOG_ERROR("network.opcode", "Auction id: {} has non-existed item (item: {})!!!", auction->Id, auction->ItemGuid.ToString());
            SendAuctionCommandResult(nullptr, AUCTION_CANCEL, ERR_AUCTION_ITEM_NOT_FOUND);
            return;
        }
    }
    else
    {
        SendAuctionCommandResult(nullptr, AUCTION_CANCEL, ERR_AUCTION_DATABASE_ERROR);

        // This code isn't possible ... maybe there should be assert
        LOG_ERROR("network.opcode", "CHEATER : {}, he tried to cancel auction (id: {}) of another player, or auction is nullptr", player->GetGUID().ToString(), packet.AuctionID);
        return;
    }

    //inform player, that auction is removed
    SendAuctionCommandResult(auction, AUCTION_CANCEL, ERR_AUCTION_OK);

    // Now remove the auction

    player->SaveInventoryAndGoldToDB(trans);
    auction->DeleteFromDB(trans);
    CharacterDatabase.CommitTransaction(trans);

    sAuctionMgr->RemoveAItem(auction->ItemGuid);
    auctionHouse->RemoveAuction(auction);
}

// Called when player lists his bids
void WorldSession::HandleAuctionListBidderItems(WorldPackets::AuctionHouse::ListBidderItems& packet)
{
    LOG_DEBUG("network", "WORLD: Received CMSG_AUCTION_LIST_BIDDER_ITEMS");

    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(packet.Auctioneer, UNIT_NPC_FLAG_AUCTIONEER);
    if (!creature)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionListBidderItems - Unit ({}) not found or you can't interact with him.", packet.Auctioneer.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    auto auctionHouse = sAuctionMgr->GetAuctionsMap(creature->GetFaction());
    if (!auctionHouse)
    {
        LOG_ERROR("network.packet", "{}: - Not found AuctionHouseObject for creature {}", __FUNCTION__, packet.Auctioneer.ToString());
        return;
    }

    WorldPackets::AuctionHouse::BidderListResult packetSend(auctionHouse);
    packetSend.OutbiddedAuctionIds = packet.OutbiddedAuctionIds;
    packetSend.PlayerGuid = _player->GetGUID();

    SendPacket(packetSend.Write());
}

// Sends player info about his auctions
void WorldSession::HandleAuctionListOwnerItems(WorldPackets::AuctionHouse::ListOwnerItems& packet)
{
    sAsyncAuctionMgr->ListOwnerItemsForPlayer(_player->GetGUID(), packet.Auctioneer);
}

void WorldSession::HandleAuctionListOwnerItemsEvent(ObjectGuid creatureGuid)
{
    LOG_DEBUG("network", "WORLD: Received CMSG_AUCTION_LIST_OWNER_ITEMS");

    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(creatureGuid, UNIT_NPC_FLAG_AUCTIONEER);
    if (!creature)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionListOwnerItems - Unit ({}) not found or you can't interact with him.", creatureGuid.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    WorldPackets::AuctionHouse::ListOwnerResult packetSend;
    packetSend.Auctioneer = creatureGuid;
    packetSend.PlayerGuid = _player->GetGUID();
    packetSend.auctionHouse = sAuctionMgr->GetAuctionsMap(creature->GetFaction());

    SendPacket(packetSend.Write());
}

// Called when player clicks on search button
void WorldSession::HandleAuctionListItems(WorldPackets::AuctionHouse::ListItems& packet)
{
    LOG_DEBUG("network", "WORLD: Received CMSG_AUCTION_LIST_ITEMS");

    // Remove fake death
    if (_player->HasUnitState(UNIT_STATE_DIED))
        _player->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    auto listItems{ std::make_shared<AuctionListItems>(std::move(packet.AuctionItems)) };
    listItems->PlayerGuid = _player->GetGUID();

    sAsyncAuctionMgr->ListItemsForPlayer(std::move(listItems));
}

void WorldSession::HandleAuctionListPendingSales(WorldPackets::AuctionHouse::ListPendingSales& /*packet*/)
{
    LOG_DEBUG("network", "WORLD: Received CMSG_AUCTION_LIST_PENDING_SALES");
    SendPacket(WorldPackets::AuctionHouse::ListPendingSalesServer().Write());
}
