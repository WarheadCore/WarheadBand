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

#include "AsyncAuctionListing.h"
#include "AuctionHouseMgr.h"
#include "Chat.h"
#include "GameConfig.h"
#include "GameTime.h"
#include "Language.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "UpdateMask.h"
#include "Util.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

//void called when player click on auctioneer npc
void WorldSession::HandleAuctionHelloOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;                                            //NPC guid
    recvData >> guid;

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_AUCTIONEER);
    if (!unit)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionHelloOpcode - Unit ({}) not found or you can't interact with him.", guid.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    SendAuctionHello(guid, unit);
}

//this void causes that auction window is opened
void WorldSession::SendAuctionHello(ObjectGuid guid, Creature* unit)
{
    if (GetPlayer()->getLevel() < CONF_GET_INT("LevelReq.Auction"))
    {
        SendNotification(GetWarheadString(LANG_AUCTION_REQ), CONF_GET_INT("LevelReq.Auction"));
        return;
    }

    if (!sScriptMgr->CanSendAuctionHello(this, guid, unit))
        return;

    AuctionHouseEntry const* ahEntry = AuctionHouseMgr::GetAuctionHouseEntry(unit->GetFaction());
    if (!ahEntry)
        return;

    WorldPacket data(MSG_AUCTION_HELLO, 12);
    data << guid;
    data << uint32(ahEntry->houseId);
    data << uint8(1);                                       // 3.3.3: 1 - AH enabled, 0 - AH disabled
    SendPacket(&data);
}

//call this method when player bids, creates, or deletes auction
void WorldSession::SendAuctionCommandResult(uint32 auctionId, uint32 Action, uint32 ErrorCode, uint32 bidError)
{
    WorldPacket data(SMSG_AUCTION_COMMAND_RESULT, 16);
    data << auctionId;
    data << Action;
    data << ErrorCode;
    if (!ErrorCode && Action)
        data << bidError;                                   //when bid, then send 0, once...
    SendPacket(&data);
}

//this function sends notification, if bidder is online
void WorldSession::SendAuctionBidderNotification(uint32 location, uint32 auctionId, ObjectGuid bidder, uint32 bidSum, uint32 diff, uint32 item_template)
{
    WorldPacket data(SMSG_AUCTION_BIDDER_NOTIFICATION, (8 * 4));
    data << uint32(location);
    data << uint32(auctionId);
    data << bidder;
    data << uint32(bidSum);
    data << uint32(diff);
    data << uint32(item_template);
    data << uint32(0);
    SendPacket(&data);
}

//this void causes on client to display: "Your auction sold"
void WorldSession::SendAuctionOwnerNotification(AuctionEntry* auction)
{
    WorldPacket data(SMSG_AUCTION_OWNER_NOTIFICATION, (8 * 4));
    data << uint32(auction->Id);
    data << uint32(auction->bid);
    data << uint32(0);                                      //unk
    data << uint64(0);                                      //unk (bidder guid?)
    data << uint32(auction->item_template);
    data << uint32(0);                                      //unk
    data << float(0);                                       //unk (time?)
    SendPacket(&data);
}

//this void creates new auction and adds auction to some auctionhouse
void WorldSession::HandleAuctionSellItem(WorldPacket& recvData)
{
    ObjectGuid auctioneer;
    uint32 itemsCount, etime, bid, buyout;
    recvData >> auctioneer;
    recvData >> itemsCount;

    ObjectGuid itemGUIDs[MAX_AUCTION_ITEMS]; // 160 slot = 4x 36 slot bag + backpack 16 slot
    uint32 count[MAX_AUCTION_ITEMS];
    memset(count, 0, sizeof(count));

    if (itemsCount > MAX_AUCTION_ITEMS)
    {
        SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
        recvData.rfinish();
        return;
    }

    for (uint32 i = 0; i < itemsCount; ++i)
    {
        recvData >> itemGUIDs[i];
        recvData >> count[i];

        if (!itemGUIDs[i] || !count[i] || count[i] > 1000)
        {
            recvData.rfinish();
            return;
        }
    }

    recvData >> bid;
    recvData >> buyout;
    recvData >> etime;

    if (!bid || !etime)
        return;

    if (bid > MAX_MONEY_AMOUNT || buyout > MAX_MONEY_AMOUNT)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionSellItem - Player {} ({}) attempted to sell item with higher price than max gold amount.",
            _player->GetName(), _player->GetGUID());
        SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
        return;
    }

    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(auctioneer, UNIT_NPC_FLAG_AUCTIONEER);
    if (!creature)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionSellItem - Unit ({}) not found or you can't interact with him.", auctioneer.ToString());
        return;
    }

    AuctionHouseEntry const* auctionHouseEntry = AuctionHouseMgr::GetAuctionHouseEntry(creature->GetFaction());
    if (!auctionHouseEntry)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionSellItem - Unit ({}) has wrong faction.", auctioneer.ToString());
        return;
    }

    etime *= MINUTE;

    switch (etime)
    {
        case 1*MIN_AUCTION_TIME:
        case 2*MIN_AUCTION_TIME:
        case 4*MIN_AUCTION_TIME:
            break;
        default:
            return;
    }

    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    Item* items[MAX_AUCTION_ITEMS];

    uint32 finalCount = 0;
    uint32 itemEntry = 0;

    for (uint32 i = 0; i < itemsCount; ++i)
    {
        Item* item = _player->GetItemByGuid(itemGUIDs[i]);

        if (!item)
        {
            SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_ITEM_NOT_FOUND);
            return;
        }

        if (itemEntry == 0)
            itemEntry = item->GetTemplate()->ItemId;

        if (sAuctionMgr->GetAItem(item->GetGUID()) || !item->CanBeTraded() || item->IsNotEmptyBag() ||
                item->GetTemplate()->Flags & ITEM_FLAG_CONJURED || item->GetUInt32Value(ITEM_FIELD_DURATION) ||
                item->GetCount() < count[i] || itemEntry != item->GetTemplate()->ItemId)
        {
            SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
            return;
        }

        items[i] = item;
        finalCount += count[i];
    }

    if (!finalCount)
    {
        SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
        return;
    }

    // check if there are 2 identical guids, in this case user is most likely cheating
    for (uint32 i = 0; i < itemsCount - 1; ++i)
    {
        for (uint32 j = i + 1; j < itemsCount; ++j)
        {
            if (itemGUIDs[i] == itemGUIDs[j])
            {
                SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
                return;
            }
        }
    }

    for (uint32 i = 0; i < itemsCount; ++i)
    {
        Item* item = items[i];

        if (item->GetMaxStackCount() < finalCount)
        {
            SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
            return;
        }
    }

    for (uint32 i = 0; i < itemsCount; ++i)
    {
        Item* item = items[i];

        uint32 auctionTime = uint32(etime * CONF_GET_FLOAT("Rate.Auction.Time"));
        AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(creature->GetFaction());

        uint32 deposit = sAuctionMgr->GetAuctionDeposit(auctionHouseEntry, etime, item, finalCount);
        if (!_player->HasEnoughMoney(deposit))
        {
            SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_NOT_ENOUGHT_MONEY);
            return;
        }

        _player->ModifyMoney(-int32(deposit));

        AuctionEntry* AH = new AuctionEntry;
        AH->Id = sObjectMgr->GenerateAuctionID();

        if (CONF_GET_BOOL("AllowTwoSide.Interaction.Auction"))
            AH->houseId = AUCTIONHOUSE_NEUTRAL;
        else
        {
            CreatureData const* auctioneerData = sObjectMgr->GetCreatureData(creature->GetSpawnId());
            if (!auctioneerData)
            {
                LOG_ERROR("network.opcode", "Data for auctioneer not found ({})", auctioneer.ToString());
                return;
            }

            CreatureTemplate const* auctioneerInfo = sObjectMgr->GetCreatureTemplate(auctioneerData->id);
            if (!auctioneerInfo)
            {
                LOG_ERROR("network.opcode", "Non existing auctioneer ({})", auctioneer.ToString());
                return;
            }

            const AuctionHouseEntry* AHEntry = sAuctionMgr->GetAuctionHouseEntry(auctioneerInfo->faction);
            AH->houseId = AHEntry->houseId;
        }

        // Required stack size of auction matches to current item stack size, just move item to auctionhouse
        if (itemsCount == 1 && item->GetCount() == count[i])
        {
            AH->item_guid = item->GetGUID();
            AH->item_template = item->GetEntry();
            AH->itemCount = item->GetCount();
            AH->owner = _player->GetGUID();
            AH->startbid = bid;
            AH->bidder = ObjectGuid::Empty;
            AH->bid = 0;
            AH->buyout = buyout;
            AH->expire_time = GameTime::GetGameTime().count() + auctionTime;
            AH->deposit = deposit;
            AH->auctionHouseEntry = auctionHouseEntry;

            LOG_DEBUG("network.opcode", "CMSG_AUCTION_SELL_ITEM: Player {} ({}) is selling item {} entry {} ({}) with count {} with initial bid {} with buyout {} and with time {} (in sec) in auctionhouse {}",
                _player->GetName(), _player->GetGUID(), item->GetTemplate()->Name1, item->GetEntry(), item->GetGUID(), item->GetCount(), bid, buyout, auctionTime, AH->GetHouseId());
            sAuctionMgr->AddAItem(item);
            auctionHouse->AddAuction(AH);

            _player->MoveItemFromInventory(item->GetBagSlot(), item->GetSlot(), true);

            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            item->DeleteFromInventoryDB(trans);
            item->SaveToDB(trans);
            AH->SaveToDB(trans);
            _player->SaveInventoryAndGoldToDB(trans);
            CharacterDatabase.CommitTransaction(trans);

            SendAuctionCommandResult(AH->Id, AUCTION_SELL_ITEM, ERR_AUCTION_OK);

            GetPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION, 1);
            return;
        }
        else // Required stack size of auction does not match to current item stack size, clone item and set correct stack size
        {
            Item* newItem = item->CloneItem(finalCount, _player);
            if (!newItem)
            {
                LOG_ERROR("network.opcode", "CMSG_AUCTION_SELL_ITEM: Could not create clone of item {}", item->GetEntry());
                SendAuctionCommandResult(0, AUCTION_SELL_ITEM, ERR_AUCTION_DATABASE_ERROR);
                return;
            }

            AH->item_guid = newItem->GetGUID();
            AH->item_template = newItem->GetEntry();
            AH->itemCount = newItem->GetCount();
            AH->owner = _player->GetGUID();
            AH->startbid = bid;
            AH->bidder = ObjectGuid::Empty;
            AH->bid = 0;
            AH->buyout = buyout;
            AH->expire_time = GameTime::GetGameTime().count() + auctionTime;
            AH->deposit = deposit;
            AH->auctionHouseEntry = auctionHouseEntry;

            LOG_DEBUG("network.opcode", "CMSG_AUCTION_SELL_ITEM: Player {} ({}) is selling item {} entry {} ({}) with count {} with initial bid {} with buyout {} and with time {} (in sec) in auctionhouse {}",
                _player->GetName(), _player->GetGUID(), newItem->GetTemplate()->Name1, newItem->GetEntry(), newItem->GetGUID(), newItem->GetCount(), bid, buyout, auctionTime, AH->GetHouseId());
            sAuctionMgr->AddAItem(newItem);
            auctionHouse->AddAuction(AH);

            for (uint32 j = 0; j < itemsCount; ++j)
            {
                Item* item2 = items[j];

                // Item stack count equals required count, ready to delete item - cloned item will be used for auction
                if (item2->GetCount() == count[j])
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
                    item2->SetCount(item2->GetCount() - count[j]);
                    item2->SetState(ITEM_CHANGED, _player);
                    _player->ItemRemovedQuestCheck(item2->GetEntry(), count[j]);
                    item2->SendUpdateToPlayer(_player);

                    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
                    item2->SaveToDB(trans);
                    CharacterDatabase.CommitTransaction(trans);
                }
            }

            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            newItem->SaveToDB(trans);
            AH->SaveToDB(trans);
            _player->SaveInventoryAndGoldToDB(trans);
            CharacterDatabase.CommitTransaction(trans);

            SendAuctionCommandResult(AH->Id, AUCTION_SELL_ITEM, ERR_AUCTION_OK);

            GetPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION, 1);
            return;
        }
    }
}

//this function is called when client bids or buys out auction
void WorldSession::HandleAuctionPlaceBid(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: Received CMSG_AUCTION_PLACE_BID");

    ObjectGuid auctioneer;
    uint32 auctionId;
    uint32 price;
    recvData >> auctioneer;
    recvData >> auctionId >> price;

    if (!auctionId || !price)
        return;                                             //check for cheaters

    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(auctioneer, UNIT_NPC_FLAG_AUCTIONEER);
    if (!creature)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionPlaceBid - Unit ({}) not found or you can't interact with him.", auctioneer.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(creature->GetFaction());

    AuctionEntry* auction = auctionHouse->GetAuction(auctionId);
    Player* player = GetPlayer();

    if (!auction || auction->owner == player->GetGUID())
    {
        //you cannot bid your own auction:
        SendAuctionCommandResult(0, AUCTION_PLACE_BID, ERR_AUCTION_BID_OWN);
        return;
    }

    // impossible have online own another character (use this for speedup check in case online owner)
    Player* auction_owner = ObjectAccessor::FindConnectedPlayer(auction->owner);
    if (!auction_owner && sObjectMgr->GetPlayerAccountIdByGUID(auction->owner.GetCounter()) == GetAccountId())
    {
        //you cannot bid your another character auction:
        SendAuctionCommandResult(0, AUCTION_PLACE_BID, ERR_AUCTION_BID_OWN);
        return;
    }

    // cheating
    if (price <= auction->bid || price < auction->startbid)
        return;

    // price too low for next bid if not buyout
    if ((price < auction->buyout || auction->buyout == 0) &&
            price < auction->bid + auction->GetAuctionOutBid())
    {
        //auction has already higher bid, client tests it!
        return;
    }

    if (!player->HasEnoughMoney(price))
    {
        //you don't have enought money!, client tests!
        //SendAuctionCommandResult(auction->auctionId, AUCTION_PLACE_BID, ???);
        return;
    }

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    if (price < auction->buyout || auction->buyout == 0)
    {
        if (auction->bidder)
        {
            if (auction->bidder == player->GetGUID())
                player->ModifyMoney(-int32(price - auction->bid));
            else
            {
                // mail to last bidder and return money
                sAuctionMgr->SendAuctionOutbiddedMail(auction, price, GetPlayer(), trans);
                player->ModifyMoney(-int32(price));
            }
        }
        else
            player->ModifyMoney(-int32(price));

        auction->bidder = player->GetGUID();
        auction->bid = price;
        GetPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID, price);

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_AUCTION_BID);
        stmt->setUInt32(0, auction->bidder.GetCounter());
        stmt->setUInt32(1, auction->bid);
        stmt->setUInt32(2, auction->Id);
        trans->Append(stmt);

        SendAuctionCommandResult(auction->Id, AUCTION_PLACE_BID, ERR_AUCTION_OK, 0);
    }
    else
    {
        //buyout:
        if (player->GetGUID() == auction->bidder)
            player->ModifyMoney(-int32(auction->buyout - auction->bid));
        else
        {
            player->ModifyMoney(-int32(auction->buyout));
            if (auction->bidder)                          //buyout for bidded auction ..
                sAuctionMgr->SendAuctionOutbiddedMail(auction, auction->buyout, GetPlayer(), trans);
        }
        auction->bidder = player->GetGUID();
        auction->bid = auction->buyout;
        GetPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID, auction->buyout);

        //- Mails must be under transaction control too to prevent data loss
        sAuctionMgr->SendAuctionSalePendingMail(auction, trans);
        sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);
        sAuctionMgr->SendAuctionWonMail(auction, trans);

        SendAuctionCommandResult(auction->Id, AUCTION_PLACE_BID, ERR_AUCTION_OK);

        auction->DeleteFromDB(trans);

        sAuctionMgr->RemoveAItem(auction->item_guid);
        auctionHouse->RemoveAuction(auction);
    }
    player->SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);
}

//this void is called when auction_owner cancels his auction
void WorldSession::HandleAuctionRemoveItem(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: Received CMSG_AUCTION_REMOVE_ITEM");

    ObjectGuid auctioneer;
    uint32 auctionId;
    recvData >> auctioneer;
    recvData >> auctionId;

    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(auctioneer, UNIT_NPC_FLAG_AUCTIONEER);
    if (!creature)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionRemoveItem - Unit ({}) not found or you can't interact with him.", auctioneer.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(creature->GetFaction());

    AuctionEntry* auction = auctionHouse->GetAuction(auctionId);
    Player* player = GetPlayer();

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    if (auction && auction->owner == player->GetGUID())
    {
        Item* pItem = sAuctionMgr->GetAItem(auction->item_guid);
        if (pItem)
        {
            if (auction->bidder)                        // If we have a bidder, we have to send him the money he paid
            {
                uint32 auctionCut = auction->GetAuctionCut();
                if (!player->HasEnoughMoney(auctionCut))          //player doesn't have enough money, maybe message needed
                    return;
                //some auctionBidderNotification would be needed, but don't know that parts..
                sAuctionMgr->SendAuctionCancelledToBidderMail(auction, trans);
                player->ModifyMoney(-int32(auctionCut));
            }

            // item will deleted or added to received mail list
            MailDraft(auction->BuildAuctionMailSubject(AUCTION_CANCELED), AuctionEntry::BuildAuctionMailBody(ObjectGuid::Empty, 0, auction->buyout, auction->deposit))
            .AddItem(pItem)
            .SendMailTo(trans, player, auction, MAIL_CHECK_MASK_COPIED);
        }
        else
        {
            LOG_ERROR("network.opcode", "Auction id: {} has non-existed item (item: {})!!!", auction->Id, auction->item_guid.ToString());
            SendAuctionCommandResult(0, AUCTION_CANCEL, ERR_AUCTION_DATABASE_ERROR);
            return;
        }
    }
    else
    {
        SendAuctionCommandResult(0, AUCTION_CANCEL, ERR_AUCTION_DATABASE_ERROR);
        //this code isn't possible ... maybe there should be assert
        LOG_ERROR("network.opcode", "CHEATER : {}, he tried to cancel auction (id: {}) of another player, or auction is nullptr", player->GetGUID().ToString(), auctionId);
        return;
    }

    //inform player, that auction is removed
    SendAuctionCommandResult(auction->Id, AUCTION_CANCEL, ERR_AUCTION_OK);

    // Now remove the auction

    player->SaveInventoryAndGoldToDB(trans);
    auction->DeleteFromDB(trans);
    CharacterDatabase.CommitTransaction(trans);

    sAuctionMgr->RemoveAItem(auction->item_guid);
    auctionHouse->RemoveAuction(auction);
}

//called when player lists his bids
void WorldSession::HandleAuctionListBidderItems(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: Received CMSG_AUCTION_LIST_BIDDER_ITEMS");

    ObjectGuid guid;                                            //NPC guid
    uint32 listfrom;                                        //page of auctions
    uint32 outbiddedCount;                                  //count of outbidded auctions

    recvData >> guid;
    recvData >> listfrom;                                  // not used in fact (this list not have page control in client)
    recvData >> outbiddedCount;
    if (recvData.size() != (16 + outbiddedCount * 4))
    {
        LOG_ERROR("network.opcode", "Client sent bad opcode!!! with count: {} and size : {} (must be: {})", outbiddedCount, (unsigned long)recvData.size(), (16 + outbiddedCount * 4));
        outbiddedCount = 0;
    }

    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_AUCTIONEER);
    if (!creature)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionListBidderItems - Unit ({}) not found or you can't interact with him.", guid.ToString());
        recvData.rfinish();
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(creature->GetFaction());

    WorldPacket data(SMSG_AUCTION_BIDDER_LIST_RESULT, (4 + 4 + 4) + 30000); // pussywizard: ensure there is enough memory
    Player* player = GetPlayer();
    data << (uint32) 0;                                     //add 0 as count
    uint32 count = 0;
    uint32 totalcount = 0;
    while (outbiddedCount > 0)                             //add all data, which client requires
    {
        --outbiddedCount;
        uint32 outbiddedAuctionId;
        recvData >> outbiddedAuctionId;
        AuctionEntry* auction = auctionHouse->GetAuction(outbiddedAuctionId);
        if (auction && auction->BuildAuctionInfo(data))
        {
            ++totalcount;
            ++count;
        }
    }

    auctionHouse->BuildListBidderItems(data, player, count, totalcount);
    data.put<uint32>(0, count);                           // add count to placeholder
    data << totalcount;
    data << (uint32)300;                                    //unk 2.3.0
    SendPacket(&data);
}

//this void sends player info about his auctions
void WorldSession::HandleAuctionListOwnerItems(WorldPacket& recvData)
{
    // prevent crash caused by malformed packet
    ObjectGuid guid;
    uint32 listfrom;

    recvData >> guid;
    recvData >> listfrom;       // not used in fact (this list does not have page control in client)

    // pussywizard:
    const uint32 delay = 4500;
    const uint32 now = getMSTime();
    if (_lastAuctionListOwnerItemsMSTime > now) // list is pending
        return;
    uint32 diff = getMSTimeDiff(_lastAuctionListOwnerItemsMSTime, now);
    if (diff > delay)
        diff = delay;

    _lastAuctionListOwnerItemsMSTime = now + delay; // set longest possible here, actual exectuing will change this to getMSTime of that moment
    _player->m_Events.AddEvent(new AuctionListOwnerItemsDelayEvent(guid, _player->GetGUID(), true), _player->m_Events.CalculateTime(delay - diff));
}

void WorldSession::HandleAuctionListOwnerItemsEvent(ObjectGuid creatureGuid)
{
    LOG_DEBUG("network", "WORLD: Received CMSG_AUCTION_LIST_OWNER_ITEMS");

    _lastAuctionListOwnerItemsMSTime = getMSTime(); // pussywizard

    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(creatureGuid, UNIT_NPC_FLAG_AUCTIONEER);
    if (!creature)
    {
        LOG_DEBUG("network", "WORLD: HandleAuctionListOwnerItems - Unit ({}) not found or you can't interact with him.", creatureGuid.ToString());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(creature->GetFaction());

    WorldPacket data(SMSG_AUCTION_OWNER_LIST_RESULT, (4 + 4 + 4) + 60000); // pussywizard: ensure there is enough memory
    data << (uint32) 0;                                     // amount place holder

    uint32 count = 0;
    uint32 totalcount = 0;

    auctionHouse->BuildListOwnerItems(data, _player, count, totalcount);
    data.put<uint32>(0, count);
    data << (uint32) totalcount;
    data << (uint32) 0;
    SendPacket(&data);
}

//this void is called when player clicks on search button
void WorldSession::HandleAuctionListItems(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: Received CMSG_AUCTION_LIST_ITEMS");

    std::string searchedname;
    uint8 levelmin, levelmax, usable;
    uint32 listfrom, auctionSlotID, auctionMainCategory, auctionSubCategory, quality;
    ObjectGuid guid;

    recvData >> guid;
    recvData >> listfrom;                                  // start, used for page control listing by 50 elements
    recvData >> searchedname;

    recvData >> levelmin >> levelmax;
    recvData >> auctionSlotID >> auctionMainCategory >> auctionSubCategory;
    recvData >> quality >> usable;

    //recvData.read_skip<uint8>(); // pussywizard: this is the getAll option
    uint8 getAll;
    recvData >> getAll;

    // this block looks like it uses some lame byte packing or similar...
    uint8 unkCnt;
    recvData >> unkCnt;
    for (uint8 i = 0; i < unkCnt; i++)
    {
        recvData.read_skip<uint8>();
        recvData.read_skip<uint8>();
    }

    // remove fake death
    if (_player->HasUnitState(UNIT_STATE_DIED))
        _player->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    // pussywizard:
    const uint32 delay = 2000;
    const uint32 now = getMSTime();
    uint32 diff = getMSTimeDiff(_lastAuctionListItemsMSTime, now);
    if (diff > delay)
        diff = delay;
    _lastAuctionListItemsMSTime = now + delay - diff;
    std::lock_guard<std::mutex> guard(AsyncAuctionListingMgr::GetTempLock());
    AsyncAuctionListingMgr::GetTempList().push_back( AuctionListItemsDelayEvent(delay - diff, _player->GetGUID(), guid, searchedname, listfrom, levelmin, levelmax, usable, auctionSlotID, auctionMainCategory, auctionSubCategory, quality, getAll) );
}

void WorldSession::HandleAuctionListPendingSales(WorldPacket& recvData)
{
    LOG_DEBUG("network", "WORLD: Received CMSG_AUCTION_LIST_PENDING_SALES");

    recvData.read_skip<uint64>();

    uint32 count = 0;

    WorldPacket data(SMSG_AUCTION_LIST_PENDING_SALES, 4);
    data << uint32(count);                                  // count
    /*for (uint32 i = 0; i < count; ++i)
    {
        data << "";                                         // string
        data << "";                                         // string
        data << uint32(0);
        data << uint32(0);
        data << float(0);
    }*/
    SendPacket(&data);
}
