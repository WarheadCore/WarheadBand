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

#include "AuctionHouseBotBuyer.h"
#include "GameConfig.h"
#include "GameTime.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "Random.h"
#include "Timer.h"

AuctionBotBuyer::AuctionBotBuyer()
{
    // Define faction for our main data class.
    for (int i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
        _houseConfig[i].Initialize(AuctionHouseType(i));
}

bool AuctionBotBuyer::Initialize()
{
    LoadConfig();

    bool activeHouse = false;
    for (auto const& i : _houseConfig)
    {
        if (i.BuyerEnabled)
        {
            activeHouse = true;
            break;
        }
    }

    if (!activeHouse)
        return false;

    // load Check interval
    _checkInterval = Minutes{ sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_RECHECK_INTERVAL) };
    _rateMoney = CONF_GET_FLOAT("Rate.Drop.Money");
    LOG_DEBUG("ahbot", "AHBotBuyer: Check interval is {}", Warhead::Time::ToTimeString(_checkInterval));
    return true;
}

void AuctionBotBuyer::LoadConfig()
{
    for (int i = 0; i < MAX_AUCTION_HOUSE_TYPE; ++i)
    {
        _houseConfig[i].BuyerEnabled = sAuctionBotConfig->GetConfigBuyerEnabled(AuctionHouseType(i));
        if (_houseConfig[i].BuyerEnabled)
            LoadBuyerValues(_houseConfig[i]);
    }
}

void AuctionBotBuyer::LoadBuyerValues(BuyerConfiguration& /* config */)
{

}

// Makes an AHbot buyer cycle for AH type if necessary
bool AuctionBotBuyer::Update(AuctionHouseType houseType)
{
    if (!sAuctionBotConfig->GetConfigBuyerEnabled(houseType))
        return false;

    LOG_DEBUG("ahbot", "AHBotBuyer: {} buying...", AuctionBotConfig::GetHouseTypeName(houseType));

    BuyerConfiguration& config = _houseConfig[houseType];
    uint32 eligibleItems = GetItemInformation(config);
    if (eligibleItems)
    {
        // Prepare list of items to bid or buy - remove old items
        PrepareListOfEntry(config);

        // Process buying and bidding items
        BuyAndBidItems(config);
    }

    return true;
}

// Collects information about item counts and minimum prices to SameItemInfo and updates EligibleItems - a list with new items eligible for bot to buy and bid
// Returns count of items in AH that were eligible for being bought or bid on by ahbot buyer (EligibleItems size)
uint32 AuctionBotBuyer::GetItemInformation(BuyerConfiguration& config)
{
    config.SameItemInfo.clear();
    auto timeNow{ GameTime::GetGameTime() };
    uint32 count = 0;

    auto auctionObject{ sAuctionMgr->GetAuctionsMap(config.GetHouseType()) };

    auctionObject->ForEachAuctions([this, timeNow, &config, &count](AuctionEntry* auction)
    {
        if (!auction->PlayerOwner || sAuctionBotConfig->IsBotChar(auction->PlayerOwner.GetCounter()))
            return; // Skip auctions owned by AHBot

        Item* item = sAuctionMgr->GetAuctionItem(auction->ItemGuid);
        if (!item)
            return;

        BuyerItemInfo& itemInfo = config.SameItemInfo[item->GetEntry()];

        // Update item entry's count and total bid prices
        // This can be used later to determine the prices and chances to bid
        uint32 itemBidPrice = auction->StartBid / item->GetCount();
        itemInfo.TotalBidPrice = itemInfo.TotalBidPrice + itemBidPrice;
        itemInfo.BidItemCount++;

        // Set minimum bid price
        if (!itemInfo.MinBidPrice)
            itemInfo.MinBidPrice = itemBidPrice;
        else
            itemBidPrice = std::min(itemInfo.MinBidPrice, itemBidPrice);

        // Set minimum buyout price if item has buyout
        if (auction->BuyOut)
        {
            // Update item entry's count and total buyout prices
            // This can be used later to determine the prices and chances to buyout
            uint32 itemBuyPrice = auction->BuyOut / item->GetCount();
            itemInfo.TotalBuyPrice = itemInfo.TotalBuyPrice + itemBuyPrice;
            itemInfo.BuyItemCount++;

            if (!itemInfo.MinBuyPrice)
                itemInfo.MinBuyPrice = itemBuyPrice;
            else
                itemInfo.MinBuyPrice = std::min(itemInfo.MinBuyPrice, itemBuyPrice);
        }

        // Add/update EligibleItems if:
        // * no bid
        // * bid from player
        if (!auction->Bid || auction->Bidder)
        {
            config.EligibleItems[auction->Id].LastExist = timeNow;
            config.EligibleItems[auction->Id].AuctionId = auction->Id;
            ++count;
        }
    });

    LOG_TRACE("ahbot", "AHBotBuyer: {} items added to buyable/biddable vector for ah type: {}", count, config.GetHouseType());
    return count;
}

// ahInfo can be NULL
bool AuctionBotBuyer::RollBuyChance(BuyerItemInfo const* ahInfo, Item const* item, AuctionEntry const* auction, uint32 /*bidPrice*/)
{
    if (!auction->BuyOut)
        return false;

    auto itemBuyPrice = float(auction->BuyOut / item->GetCount());
    auto itemPrice = float(item->GetTemplate()->SellPrice ? item->GetTemplate()->SellPrice : GetVendorPrice(item->GetTemplate()->Quality));

    // The AH cut needs to be added to the price, but we don't want a 100% chance to buy if the price is exactly AH default
    itemPrice *= 1.4f;

    // Include server rate money
    itemPrice *= _rateMoney;

    auto baseChance{ itemPrice / itemBuyPrice * 100.f / sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_CHANCE_FACTOR)  };
    if (baseChance < 0.0f)
        baseChance = 0.0f;

    // This value is between 0 and 100 and is used directly as the chance to buy or bid
    // Value equal or above 100 means 100% chance and value below 0 means 0% chance
    float chance = std::min(100.f, baseChance);

    // If a player has bid on item, have fifth of normal chance
    if (auction->Bidder)
        chance = chance / 5.f;

    if (ahInfo)
    {
        float avgBuyPrice = ahInfo->TotalBuyPrice / float(ahInfo->BuyItemCount);

        LOG_TRACE("ahbot", "AHBotBuyer: Buyout average: {:.1f} items with buyout: {}", avgBuyPrice, ahInfo->BuyItemCount);

        // If there are more than 5 items on AH of this entry, try weigh in the average buyout price
        if (ahInfo->BuyItemCount > 5)
            chance *= 1.f / std::sqrt(itemBuyPrice / avgBuyPrice);
    }

    // Add config weigh in for quality
    chance *= GetChanceMultiplier(item->GetTemplate()->Quality) / 100.0f;
    if (chance <= frand(0.f, 100.f))
        return false;

    auto itemID{ item->GetTemplate()->ItemId };
    auto itemName{ item->GetTemplate()->Name1 };

    LOG_DEBUG("ahbot", "AHBotBuyer: Win roll buy. Chance: {:.2f}. Price: {}, BuyPrice: {}. Item: {} ({})", chance, uint32(itemPrice), uint32(itemBuyPrice), itemID, itemName);
    return true;
}

// ahInfo can be NULL
bool AuctionBotBuyer::RollBidChance(BuyerItemInfo const* ahInfo, Item const* item, AuctionEntry const* auction, uint32 bidPrice)
{
    auto itemBidPrice = float(bidPrice / item->GetCount());
    auto itemPrice = float(item->GetTemplate()->SellPrice ? item->GetTemplate()->SellPrice : GetVendorPrice(item->GetTemplate()->Quality));

    // The AH cut needs to be added to the price, but we don't want a 100% chance to buy if the price is exactly AH default
    itemPrice *= 1.4f;

    // Include server rate money
    itemPrice *= _rateMoney;

    auto baseChance{ itemPrice / itemBidPrice * 100.f / sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_CHANCE_FACTOR)  };
    if (baseChance < 0.0f)
        baseChance = 0.0f;

    // This value is between 0 and 100 and is used directly as the chance to buy or bid
    // Value equal or above 100 means 100% chance and value below 0 means 0% chance
    float chance = std::min(100.f, baseChance);

    if (ahInfo)
    {
        float avgBidPrice = ahInfo->TotalBidPrice / float(ahInfo->BidItemCount);

        LOG_TRACE("ahbot", "AHBotBuyer: Bid average: {:.1f} biddable item count: {}", avgBidPrice, ahInfo->BidItemCount);

        // If there are more than 5 items on AH of this entry, try weigh in the average bid price
        if (ahInfo->BidItemCount >= 5)
            chance *= 1.f / std::sqrt(itemBidPrice / avgBidPrice);
    }

    // If a player has bid on item, have fifth of normal chance
    if (auction->Bidder && !sAuctionBotConfig->IsBotChar(auction->Bidder.GetCounter()))
        chance = chance / 5.f;

    // Add config weigh in for quality
    chance *= GetChanceMultiplier(item->GetTemplate()->Quality) / 100.0f;
    if (chance <= frand(0.f, 100.f))
        return false;

    auto itemID{ item->GetTemplate()->ItemId };
    auto itemName{ item->GetTemplate()->Name1 };

    LOG_DEBUG("ahbot", "AHBotBuyer: Win roll bid. Chance: {:.2f}. Price: {}, BidPrice: {}. Item: {} ({})", chance, uint32(itemPrice), uint32(itemBidPrice), itemID, itemName);
    return true;
}

// Removes items from EligibleItems that we shouldn't buy or bid on
// The last existed time on them should be older than now
void AuctionBotBuyer::PrepareListOfEntry(BuyerConfiguration& config)
{
    // now - 5 seconds to leave out all old entries but keep the ones just updated a moment ago
    auto timeNow{ GameTime::GetGameTime() - 5s };

    for (auto itr = config.EligibleItems.begin(); itr != config.EligibleItems.end();)
    {
        if (itr->second.LastExist < timeNow)
            config.EligibleItems.erase(itr++);
        else
            ++itr;
    }

    LOG_TRACE("ahbot", "AHBotBuyer: EligibleItems size: {}", config.EligibleItems.size());
}

// Tries to bid and buy items based on their prices and chances set in configs
void AuctionBotBuyer::BuyAndBidItems(BuyerConfiguration& config)
{
    auto timeNow{ GameTime::GetGameTime() };
    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMap(config.GetHouseType());
    auto& items = config.EligibleItems;
    bool canBid{ sAuctionBotConfig->GetRandChar() != 0 };

    // Max amount of items to buy or bid
    uint32 cycles = sAuctionBotConfig->GetItemPerCycleNormal();
    if (items.size() > sAuctionBotConfig->GetItemPerCycleBoost())
    {
        // set more cycles if there is a huge influx of items
        cycles = sAuctionBotConfig->GetItemPerCycleBoost();
        LOG_DEBUG("ahbot", "AHBotBuyer: Boost value used for Buyer! (if this happens often adjust both ItemsPerCycle in worldserver.conf)");
    }

    // Process items eligible to be bid or bought
    auto itr = items.begin();
    while (cycles && itr != items.end())
    {
        AuctionEntry* auction = auctionHouse->GetAuction(itr->second.AuctionId);
        if (!auction)
        {
            LOG_DEBUG("ahbot", "AHBotBuyer: Entry {} doesn't exists, perhaps bought already?", itr->second.AuctionId);
            items.erase(itr++);
            continue;
        }

        // Check if the item has been checked once before
        // If it has been checked, and it was recently, skip it
        if (itr->second.LastChecked > 0s && (timeNow - itr->second.LastChecked) <= _checkInterval)
        {
            LOG_TRACE("ahbot", "AHBotBuyer: In time interval wait for entry {}!", auction->Id);
            ++itr;
            continue;
        }

        Item* item = sAuctionMgr->GetAuctionItem(auction->ItemGuid);
        if (!item)
        {
            // auction item not accessible, possible auction in payment pending mode
            items.erase(itr++);
            continue;
        }

        // price to bid if bidding
        uint32 bidPrice;
        if (auction->Bid >= auction->StartBid)
        {
            // get bid price to outbid previous Bidder
            bidPrice = auction->Bid + auction->GetAuctionOutBid();
        }
        else
        {
            // no previous bidders - use starting bid
            bidPrice = auction->StartBid;
        }

        BuyerItemInfo const* ahInfo = nullptr;
        auto sameItemItr = config.SameItemInfo.find(item->GetEntry());
        if (sameItemItr != config.SameItemInfo.end())
            ahInfo = &sameItemItr->second;

        LOG_TRACE("ahbot", "AHBotBuyer: Rolling for AH entry: {}", auction->Id);

        // Roll buy and bid chances
        bool successBuy = RollBuyChance(ahInfo, item, auction, bidPrice);
        bool successBid = RollBidChance(ahInfo, item, auction, bidPrice);

        // If roll bidding succesfully and bid price is above buyout -> buyout
        // If roll for buying was successful but not for bid, buyout directly
        // If roll bidding was also successful, buy the entry with 20% chance
        // - Better bid than buy since the item is bought by bot if no player bids after
        // Otherwise bid if roll for bid was successful
        if ((auction->BuyOut && successBid && bidPrice >= auction->BuyOut) ||
            (successBuy && (!successBid || urand(1, 5) == 1)))
            BuyEntry(auction, auctionHouse); // buyout
        else if (successBid && canBid)
            PlaceBidToEntry(auction, bidPrice); // bid

        itr->second.LastChecked = timeNow;
        --cycles;
        ++itr;
    }

    // Clear not needed entries
    config.SameItemInfo.clear();
}

uint32 AuctionBotBuyer::GetVendorPrice(uint32 quality)
{
    switch (quality)
    {
        case ITEM_QUALITY_POOR:
            return sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_BASEPRICE_GRAY);
        case ITEM_QUALITY_NORMAL:
            return sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_BASEPRICE_WHITE);
        case ITEM_QUALITY_UNCOMMON:
            return sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_BASEPRICE_GREEN);
        case ITEM_QUALITY_RARE:
            return sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_BASEPRICE_BLUE);
        case ITEM_QUALITY_EPIC:
            return sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_BASEPRICE_PURPLE);
        case ITEM_QUALITY_LEGENDARY:
            return sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_BASEPRICE_ORANGE);
        case ITEM_QUALITY_ARTIFACT:
            return sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_BASEPRICE_YELLOW);
        default:
            return 1 * SILVER;
    }
}

uint32 AuctionBotBuyer::GetChanceMultiplier(uint32 quality)
{
    switch (quality)
    {
        case ITEM_QUALITY_POOR:
            return sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_CHANCEMULTIPLIER_GRAY);
        case ITEM_QUALITY_NORMAL:
            return sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_CHANCEMULTIPLIER_WHITE);
        case ITEM_QUALITY_UNCOMMON:
            return sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_CHANCEMULTIPLIER_GREEN);
        case ITEM_QUALITY_RARE:
            return sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_CHANCEMULTIPLIER_BLUE);
        case ITEM_QUALITY_EPIC:
            return sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_CHANCEMULTIPLIER_PURPLE);
        case ITEM_QUALITY_LEGENDARY:
            return sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_CHANCEMULTIPLIER_ORANGE);
        case ITEM_QUALITY_ARTIFACT:
            return sAuctionBotConfig->GetConfig(CONFIG_AHBOT_BUYER_CHANCEMULTIPLIER_YELLOW);
        default:
            return 100;
    }
}

// Buys the auction and does necessary actions to complete the buyout
void AuctionBotBuyer::BuyEntry(AuctionEntry* auction, AuctionHouseObject* auctionHouse)
{
    LOG_DEBUG("ahbot", "AHBotBuyer: Entry {} bought at {}g", auction->Id, float(auction->BuyOut) / GOLD);

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    // Send mail to previous Bidder if any
    if (auction->Bidder && !sAuctionBotConfig->IsBotChar(auction->Bidder.GetCounter()))
        sAuctionMgr->SendAuctionOutbiddedMail(auction, auction->BuyOut, nullptr, trans);

    // Set bot as Bidder and set new bid amount
    auction->Bidder = ObjectGuid::Create<HighGuid::Player>(sAuctionBotConfig->GetRandCharExclude(auction->PlayerOwner.GetCounter()));
    auction->Bid = auction->BuyOut;

    // Mails must be under transaction control too to prevent data loss
    sAuctionMgr->SendAuctionSalePendingMail(auction, trans);
    sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);
    sAuctionMgr->SendAuctionWonMail(auction, trans);

    // Delete auction from DB
    auction->DeleteFromDB(trans);

    // Remove auction item and auction from memory
    sAuctionMgr->RemoveAItem(auction->ItemGuid);
    auctionHouse->RemoveAuction(auction);

    // Run SQLs
    CharacterDatabase.CommitTransaction(trans);
}

// Bids on the auction and does the necessary actions for bidding
void AuctionBotBuyer::PlaceBidToEntry(AuctionEntry* auction, uint32 bidPrice)
{
    LOG_DEBUG("ahbot", "AHBotBuyer: Bid placed to entry {}, {}g", auction->Id, float(bidPrice) / GOLD);

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    // Send mail to previous Bidder if any
    if (auction->Bidder && !sAuctionBotConfig->IsBotChar(auction->Bidder.GetCounter()))
        sAuctionMgr->SendAuctionOutbiddedMail(auction, bidPrice, nullptr, trans);

    // Set bot as Bidder and set new bid amount
    auction->Bidder = ObjectGuid::Create<HighGuid::Player>(sAuctionBotConfig->GetRandCharExclude(auction->PlayerOwner.GetCounter()));
    auction->Bid = bidPrice;
//    auction->Flags = AuctionEntryFlag(auction->Flags & ~AUCTION_ENTRY_FLAG_GM_LOG_BUYER);

    // Update auction to DB
    auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_AUCTION_BID);
    stmt->SetArguments(auction->Bidder.GetCounter(), auction->Bid, auction->Id);
    trans->Append(stmt);

    // Run SQLs
    CharacterDatabase.CommitTransaction(trans);
}