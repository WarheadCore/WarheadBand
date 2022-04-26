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

#include "MasterBot.h"
#include "Player.h"
#include "PlayerBot.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "PetitionMgr.h"
#include "Guild.h"

MasterBot::~MasterBot()
{
    LogoutAllBots(true);
}

void MasterBot::HandleMasterIncomingPacket(WorldPacket const& packet)
{
    switch (packet.GetOpcode())
    {
        case CMSG_OFFER_PETITION:
        {
            WorldPacket data(packet);
            data.rpos(0); // reset reader
            ObjectGuid petitionGuid;
            ObjectGuid playerGuid;
            uint32 junk;

            data >> junk;                                      // this is not petition type!
            data >> petitionGuid;                              // petition guid
            data >> playerGuid;                                // player guid

            Player* player = ObjectAccessor::FindPlayer(playerGuid);
            if (!player)
                return;

            Petition const* petition = sPetitionMgr->GetPetition(petitionGuid);
            if (petition)
            {
                ChatHandler(_master->GetSession()).PSendSysMessage("{} has already signed the petition", player->GetName());
                return;
            }

            Signatures const* signatures = sPetitionMgr->GetSignature(petitionGuid);
            if (!signatures)
                return;

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PETITION_SIGNATURE);
            stmt->SetData(0, petition->ownerGuid.GetCounter());
            stmt->SetData(1, petitionGuid.GetCounter());
            stmt->SetData(2, playerGuid.GetCounter());
            stmt->SetData(3, GetMaster()->GetSession()->GetAccountId());
            CharacterDatabase.Execute(stmt);

            // xinef: fill petition store
            sPetitionMgr->AddSignature(petitionGuid, GetMaster()->GetSession()->GetAccountId(), playerGuid);

            data.Initialize(SMSG_PETITION_SIGN_RESULTS, (8 + 8 + 4));
            data << ObjectGuid(petitionGuid);
            data << ObjectGuid(playerGuid);
            data << uint32(PETITION_SIGN_OK);

            // close at signer side
            GetMaster()->GetSession()->SendPacket(&data);
            break;
        }
        case CMSG_ACTIVATETAXI:
        {
            WorldPacket p(packet);
            p.rpos(0); // reset reader

            ObjectGuid guid;
            std::vector<uint32> nodes;
            nodes.resize(2);
            uint8 delay = 9;
            p >> guid >> nodes[0] >> nodes[1];

            // DEBUG_LOG ("[PlayerbotMgr]: HandleMasterIncomingPacket - Received CMSG_ACTIVATETAXI from %d to %d", nodes[0], nodes[1]);

            /*for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            {
                delay = delay + 3;
                Player* const bot = it->second;
                if (!bot)
                    return;

                Group* group = bot->GetGroup();
                if (!group)
                    continue;

                Unit* target = ObjectAccessor::GetUnit(*bot, guid);

                bot->GetPlayerbotAI()->SetIgnoreUpdateTime(delay);

                bot->GetMotionMaster()->Clear(true);
                bot->GetMotionMaster()->MoveFollow(target, INTERACTION_DISTANCE, bot->GetOrientation());
                bot->GetPlayerbotAI()->GetTaxi(guid, nodes);
            }*/

            break;
        }
        case CMSG_ACTIVATETAXIEXPRESS:
        {
            WorldPacket p(packet);
            p.rpos(0); // reset reader

            ObjectGuid guid;
            uint32 node_count;
            uint8 delay = 9;
            p >> guid;
            p >> node_count;

            std::vector<uint32> nodes;
            for (uint32 i = 0; i < node_count; ++i)
            {
                uint32 node;
                p >> node;
                nodes.push_back(node);
            }

            if (nodes.empty())
                return;

            // DEBUG_LOG ("[PlayerbotMgr]: HandleMasterIncomingPacket - Received CMSG_ACTIVATETAXIEXPRESS from %d to %d", nodes.front(), nodes.back());

            /*for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            {

                delay = delay + 3;
                Player* const bot = it->second;
                if (!bot)
                    return;
                Group* group = bot->GetGroup();
                if (!group)
                    continue;
                Unit* target = ObjectAccessor::GetUnit(*bot, guid);

                bot->GetPlayerbotAI()->SetIgnoreUpdateTime(delay);

                bot->GetMotionMaster()->Clear(true);
                bot->GetMotionMaster()->MoveFollow(target, INTERACTION_DISTANCE, bot->GetOrientation());
                bot->GetPlayerbotAI()->GetTaxi(guid, nodes);
            }*/
            break;
        }
        // if master is logging out, log out all bots
        case CMSG_LOGOUT_REQUEST:
        {
            LogoutAllBots();
            break;
        }

        // If master inspects one of his bots, give the master useful info in chat window
        // such as inventory that can be equipped
        case CMSG_INSPECT:
        {
            //WorldPacket p(packet);
            //p.rpos(0); // reset reader
            //ObjectGuid guid;
            //p >> guid;
            //auto bot = GetPlayerBot(guid);
            //if (bot)
            //    bot->GetPlayerbotAI()->SendNotEquipList(*bot);
            break;
        }
        case CMSG_GAMEOBJ_USE: // not sure if we still need this one
        case CMSG_GAMEOBJ_REPORT_USE:
        {
            WorldPacket p(packet);
            p.rpos(0);     // reset reader
            ObjectGuid objGUID;
            p >> objGUID;
            //for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            //{
            //    Player* const bot = it->second;

            //    // If player and bot are on different maps: then player was teleported by GameObject
            //    // let's return and let playerbot summon do its job by teleporting bots
            //    Map* masterMap = m_master->IsInWorld() ? m_master->GetMap() : nullptr;
            //    if (!masterMap || bot->GetMap() != masterMap || m_master->IsBeingTeleported())
            //        return;

            //    bot->GetPlayerbotAI()->FollowAutoReset();
            //    GameObject* obj = masterMap->GetGameObject(objGUID);
            //    if (!obj)
            //        return;

            //    // add other go types here, i.e.:
            //    // GAMEOBJECT_TYPE_CHEST - loot quest items of chest
            //    if (obj->GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER)
            //    {
            //        bot->GetPlayerbotAI()->TurnInQuests(obj);

            //        // auto accept every available quest this NPC has
            //        bot->PrepareQuestMenu(objGUID);
            //        QuestMenu& questMenu = bot->GetPlayerMenu()->GetQuestMenu();
            //        for (uint32 iI = 0; iI < questMenu.MenuItemCount(); ++iI)
            //        {
            //            QuestMenuItem const& qItem = questMenu.GetItem(iI);
            //            uint32 questID = qItem.m_qId;
            //            if (!bot->GetPlayerbotAI()->AddQuest(questID, obj))
            //                DEBUG_LOG("Couldn't take quest");
            //        }
            //    }
            //}
            break;
        }
        case CMSG_QUESTGIVER_HELLO:
        {
            //WorldPacket p(packet);
            //p.rpos(0);    // reset reader
            //ObjectGuid npcGUID;
            //p >> npcGUID;
            //WorldObject* pNpc = m_master->GetMap()->GetWorldObject(npcGUID);
            //if (!pNpc)
            //    return;

            //// for all master's bots
            //for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            //{
            //    Player* const bot = it->second;
            //    bot->GetPlayerbotAI()->FollowAutoReset();
            //    bot->GetPlayerbotAI()->TurnInQuests(pNpc);
            //}

            break;
        }
        // if master accepts a quest, bots should also try to accept quest
        case CMSG_QUESTGIVER_ACCEPT_QUEST:
        {
            WorldPacket p(packet);
            p.rpos(0);    // reset reader
            ObjectGuid guid;
            uint32 quest;
            uint32 unk1;
            p >> guid >> quest >> unk1;

            // DEBUG_LOG ("[PlayerbotMgr]: HandleMasterIncomingPacket - Received CMSG_QUESTGIVER_ACCEPT_QUEST npc = %s, quest = %u, unk1 = %u", guid.GetString().c_str(), quest, unk1);

            //Quest const* qInfo = sObjectMgr->GetQuestTemplate(quest);
            //if (qInfo)
            //{
            //    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            //    {
            //        Player* const bot = it->second;
            //        bot->GetPlayerbotAI()->FollowAutoReset();
            //        if (bot->GetQuestStatus(quest) == QUEST_STATUS_COMPLETE)
            //            bot->GetPlayerbotAI()->TellMaster("I already completed that quest.");
            //        else if (!bot->CanTakeQuest(qInfo, false))
            //        {
            //            if (!bot->SatisfyQuestStatus(qInfo, false))
            //                bot->GetPlayerbotAI()->TellMaster("I already have that quest.");
            //            else
            //                bot->GetPlayerbotAI()->TellMaster("I can't take that quest.");
            //        }
            //        else if (!bot->SatisfyQuestLog(false))
            //            bot->GetPlayerbotAI()->TellMaster("My quest log is full.");
            //        else if (!bot->CanAddQuest(qInfo, false))
            //            bot->GetPlayerbotAI()->TellMaster("I can't take that quest because it requires that I take items, but my bags are full!");

            //        else
            //        {
            //            p.rpos(0);         // reset reader
            //            bot->GetSession()->HandleQuestgiverAcceptQuestOpcode(p);
            //            bot->GetPlayerbotAI()->TellMaster("Got the quest.");

            //            // build needed items if quest contains any
            //            for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; i++)
            //                if (qInfo->ReqItemCount[i] > 0)
            //                {
            //                    bot->GetPlayerbotAI()->SetQuestNeedItems();
            //                    break;
            //                }

            //            // build needed creatures if quest contains any
            //            for (int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
            //                if (qInfo->ReqCreatureOrGOCount[i] > 0)
            //                {
            //                    bot->GetPlayerbotAI()->SetQuestNeedCreatures();
            //                    break;
            //                }
            //        }
            //    }
            //}
            break;
        }
        case CMSG_AREATRIGGER:
        {
            WorldPacket p(packet);

            //for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            //{
            //    Player* const bot = it->second;
            //    if (!bot)
            //        continue;

            //    if (bot->IsWithinDistInMap(GetMaster(), 50))
            //    {
            //        p.rpos(0);         // reset reader
            //        bot->GetSession()->HandleAreaTriggerOpcode(p);
            //    }
            //}
            break;
        }
        case CMSG_QUESTGIVER_COMPLETE_QUEST:
        {
            WorldPacket p(packet);
            p.rpos(0);    // reset reader
            uint32 quest;
            ObjectGuid npcGUID;
            p >> npcGUID >> quest;

            // DEBUG_LOG ("[PlayerbotMgr]: HandleMasterIncomingPacket - Received CMSG_QUESTGIVER_COMPLETE_QUEST npc = %s, quest = %u", npcGUID.GetString().c_str(), quest);

            //WorldObject* pNpc = m_master->GetMap()->GetWorldObject(npcGUID);
            //if (!pNpc)
            //    return;

            //// for all master's bots
            //for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            //{
            //    Player* const bot = it->second;
            //    bot->GetPlayerbotAI()->FollowAutoReset();
            //    bot->GetPlayerbotAI()->TurnInQuests(pNpc);
            //}
            break;
        }
        case CMSG_LOOT_ROLL:
        {
            WorldPacket p(packet);  //WorldPacket packet for CMSG_LOOT_ROLL, (8+4+1)
            ObjectGuid Guid;
            uint32 itemSlot;
            uint8 rollType;

            p.rpos(0);              //reset packet pointer
            p >> Guid;              //guid of the lootable target
            p >> itemSlot;          //loot index
            p >> rollType;          //need,greed or pass on roll

            //for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            //{
            //    uint32 choice = 0;

            //    Player* const bot = it->second;
            //    if (!bot)
            //        return;

            //    Group* group = bot->GetGroup();
            //    if (!group)
            //        return;

            //    // check that the bot did not already vote
            //    if (rollType >= ROLL_NOT_EMITED_YET)
            //        return;

            //    Loot* loot = sLootMgr.GetLoot(bot, Guid);

            //    if (!loot)
            //    {
            //        sLog.outError("LootMgr::PlayerVote> Error cannot get loot object info!");
            //        return;
            //    }

            //    LootItem* lootItem = loot->GetLootItemInSlot(itemSlot);

            //    ItemPrototype const* pProto = lootItem->itemProto;
            //    if (!pProto)
            //        return;

            //    if (bot->GetPlayerbotAI()->CanStore())
            //    {
            //        if (bot->CanUseItem(pProto) == EQUIP_ERR_OK && bot->GetPlayerbotAI()->IsItemUseful(lootItem->itemId))
            //            choice = 1;     // Need
            //        else if (bot->HasSkill(SKILL_ENCHANTING))
            //            choice = 3;     // Disenchant
            //        else
            //            choice = 2;     // Greed
            //    }
            //    else
            //        choice = 0;         // Pass

            //    sLootMgr.PlayerVote(bot, Guid, itemSlot, RollVote(choice));

            //    switch (choice)
            //    {
            //    case ROLL_NEED:
            //        bot->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED, 1);
            //        break;
            //    case ROLL_GREED:
            //    case ROLL_DISENCHANT:
            //        bot->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED, 1);
            //        break;
            //    }
            //}
            break;
        }
        // Handle GOSSIP activate actions, prior to GOSSIP select menu actions
        case CMSG_GOSSIP_HELLO:
        {
            // DEBUG_LOG ("[PlayerbotMgr]: HandleMasterIncomingPacket - Received CMSG_GOSSIP_HELLO");

            WorldPacket p(packet);    //WorldPacket packet for CMSG_GOSSIP_HELLO, (8)
            ObjectGuid guid;
            p.rpos(0);                //reset packet pointer
            p >> guid;
            //for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            //{
            //    Player* const bot = it->second;
            //    if (!bot)
            //        continue;
            //    bot->GetPlayerbotAI()->FollowAutoReset();
            //    Creature* pCreature = bot->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
            //    if (!pCreature)
            //    {
            //        DEBUG_LOG("[PlayerbotMgr]: HandleMasterIncomingPacket - Received  CMSG_GOSSIP_HELLO %s not found or you can't interact with him.", guid.GetString().c_str());
            //        continue;
            //    }

            //    GossipMenuItemsMapBounds pMenuItemBounds = sObjectMgr->GetGossipMenuItemsMapBounds(pCreature->GetCreatureInfo()->GossipMenuId);
            //    for (GossipMenuItemsMap::const_iterator itr = pMenuItemBounds.first; itr != pMenuItemBounds.second; ++itr)
            //    {
            //        uint32 npcflags = pCreature->GetUInt32Value(UNIT_NPC_FLAGS);

            //        if (!(itr->second.npc_option_npcflag & npcflags))
            //            continue;

            //        switch (itr->second.option_id)
            //        {
            //        case GOSSIP_OPTION_TAXIVENDOR:
            //        {
            //            // bot->GetPlayerbotAI()->TellMaster("PlayerbotMgr:GOSSIP_OPTION_TAXIVENDOR");
            //            bot->GetSession()->SendLearnNewTaxiNode(pCreature);
            //            break;
            //        }
            //        case GOSSIP_OPTION_QUESTGIVER:
            //        {
            //            // bot->GetPlayerbotAI()->TellMaster("PlayerbotMgr:GOSSIP_OPTION_QUESTGIVER");
            //            bot->GetPlayerbotAI()->TurnInQuests(pCreature);
            //            break;
            //        }
            //        case GOSSIP_OPTION_VENDOR:
            //        {
            //            // bot->GetPlayerbotAI()->TellMaster("PlayerbotMgr:GOSSIP_OPTION_VENDOR");
            //            if (!botConfig.GetBoolDefault("PlayerbotAI.SellGarbage", true))
            //                continue;

            //            // changed the SellGarbage() function to support ch.SendSysMessaage()
            //            bot->GetPlayerbotAI()->SellGarbage(*bot);
            //            break;
            //        }
            //        case GOSSIP_OPTION_STABLEPET:
            //        {
            //            // bot->GetPlayerbotAI()->TellMaster("PlayerbotMgr:GOSSIP_OPTION_STABLEPET");
            //            break;
            //        }
            //        case GOSSIP_OPTION_AUCTIONEER:
            //        {
            //            // bot->GetPlayerbotAI()->TellMaster("PlayerbotMgr:GOSSIP_OPTION_AUCTIONEER");
            //            break;
            //        }
            //        case GOSSIP_OPTION_BANKER:
            //        {
            //            // bot->GetPlayerbotAI()->TellMaster("PlayerbotMgr:GOSSIP_OPTION_BANKER");
            //            break;
            //        }
            //        case GOSSIP_OPTION_INNKEEPER:
            //        {
            //            // bot->GetPlayerbotAI()->TellMaster("PlayerbotMgr:GOSSIP_OPTION_INNKEEPER");
            //            break;
            //        }
            //        }
            //    }
            //}
            break;
        }
        case CMSG_SPIRIT_HEALER_ACTIVATE:
        {
            // DEBUG_LOG ("[PlayerbotMgr]: HandleMasterIncomingPacket - Received CMSG_SPIRIT_HEALER_ACTIVATE SpiritHealer is resurrecting the Player %s",m_master->GetName());
            /*for (PlayerBotMap::iterator itr = m_playerBots.begin(); itr != m_playerBots.end(); ++itr)
            {
                Player* const bot = itr->second;
                Group* grp = bot->GetGroup();
                if (grp)
                    grp->RemoveMember(bot->GetObjectGuid(), 1);
            }*/
            break;
        }
        case CMSG_LIST_INVENTORY:
        {
            //if (!botConfig.GetBoolDefault("PlayerbotAI.SellGarbage", true))
            //    return;

            //WorldPacket p(packet);
            //p.rpos(0);  // reset reader
            //ObjectGuid npcGUID;
            //p >> npcGUID;
            //Object* const pNpc = (WorldObject*)m_master->GetObjectByTypeMask(npcGUID, TYPEMASK_CREATURE_OR_GAMEOBJECT);
            //if (!pNpc)
            //    return;

            //// for all master's bots
            //for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
            //{
            //    Player* const bot = it->second;
            //    if (!bot->IsInMap(static_cast<WorldObject*>(pNpc)))
            //    {
            //        bot->GetPlayerbotAI()->TellMaster("I'm too far away to sell items!");
            //        continue;
            //    }
            //    else
            //    {
            //        // changed the SellGarbage() function to support ch.SendSysMessaage()
            //        bot->GetPlayerbotAI()->FollowAutoReset();
            //        bot->GetPlayerbotAI()->SellGarbage(*bot);
            //    }
            //}
            break;
        }
        default:
            break;
    }
}

void MasterBot::HandleMasterOutgoingPacket(WorldPacket const& packet)
{
    switch (packet.GetOpcode())
    {
        // If a change in speed was detected for the master
        // make sure we have the same mount status
        case SMSG_FORCE_RUN_SPEED_CHANGE:
        {
            WorldPacket data(packet);
            ObjectGuid guid;

            // Only adjust speed and mount if this is master that did so
            data >> guid.ReadAsPacked();

            if (guid != GetMaster()->GetGUID())
                return;

            for (auto& [botGuid, playerBot] : _playerBotsStore)
            {
                if (GetMaster()->IsMounted() && !playerBot->IsMounted())
                {
                    // Player Part
                    //if (!GetMaster()->GetAuraEffectsByType(SPELL_AURA_MOUNTED).empty())
                    //{
                    //    // Step 2: no spell found or cast failed -> search for an item in inventory (mount)
                    //    // We start with the fastest mounts as bot will not be able to outrun its master since it is following him/her
                    //    uint32 skillLevels[] = { 375, 300, 225, 150, 75 };
                    //    for (uint32 level : skillLevels)
                    //    {
                    //        Item* mount = bot->GetPlayerbotAI()->FindMount(level);
                    //        if (mount)
                    //        {
                    //            bot->GetPlayerbotAI()->UseItem(mount);
                    //            return;
                    //        }
                    //    }
                    //}
                }
                // If master dismounted, do so
                else if (!GetMaster()->IsMounted() && playerBot->IsMounted())    // only execute code if master is the one who dismounted
                {
                    WorldPacket emptyPacket;
                    playerBot->GetSession()->HandleCancelMountAuraOpcode(emptyPacket);  //updated code
                }
            }
        }
        // maybe our bots should only start looting after the master loots?
        //case SMSG_LOOT_RELEASE_RESPONSE: {}
        case SMSG_NAME_QUERY_RESPONSE:
        case SMSG_MONSTER_MOVE:
        case SMSG_COMPRESSED_UPDATE_OBJECT:
        case SMSG_DESTROY_OBJECT:
        case SMSG_UPDATE_OBJECT:
        case SMSG_STANDSTATE_UPDATE:
        case MSG_MOVE_HEARTBEAT:
        case SMSG_QUERY_TIME_RESPONSE:
        case SMSG_AURA_UPDATE_ALL:
        case SMSG_CREATURE_QUERY_RESPONSE:
        case SMSG_GAMEOBJECT_QUERY_RESPONSE:
            return;
        default:
            break;
    }
}

void MasterBot::RemoveBots()
{
    for (auto& guid : _botsToRemove)
    {
        Player* bot = GetPlayerBot(guid);
        if (bot)
        {
            WorldSession* botWorldSessionPtr = bot->GetSession();
            _playerBotsStore.erase(guid);                           // deletes bot player ptr inside this WorldSession PlayerBotMap
            botWorldSessionPtr->LogoutPlayer(true);                 // this will delete the bot Player object and PlayerbotAI object
            delete botWorldSessionPtr;                              // finally delete the bot's WorldSession
        }
    }

    _botsToRemove.clear();
}

void MasterBot::LogoutAllBots(bool fullRemove /*= false*/)
{
    DoForAllBots([this](ObjectGuid const& botGuid, PlayerBot* playerBot)
    {
        _botsToRemove.emplace(botGuid);
    });

    if (fullRemove)
        RemoveBots();
}

void MasterBot::Stay()
{
    DoForAllBots([this](ObjectGuid const& botGuid, PlayerBot* playerBot)
    {
        playerBot->GetMotionMaster()->Clear();
    });
}

// Playerbot mod: logs out a Playerbot.
void MasterBot::LogoutPlayerBot(ObjectGuid guid)
{
    _botsToRemove.emplace(guid);
}

// Playerbot mod: Gets a player bot Player object for this WorldSession master
PlayerBot* MasterBot::GetPlayerBot(ObjectGuid botGuid) const
{
    return Warhead::Containers::MapGetValuePtr(_playerBotsStore, botGuid);
}

void MasterBot::OnBotLogin(PlayerBot* bot)
{
    auto const& botGuid = bot->GetGUID();

    // simulate client taking control
    auto data1 = std::make_unique<WorldPacket>(CMSG_SET_ACTIVE_MOVER, 8);
    *data1 << botGuid;
    bot->GetSession()->QueuePacket(data1.get());

    auto data2 = std::make_unique<WorldPacket>(MSG_MOVE_FALL_LAND, 64);
    bot->GetSession()->WriteMovementInfo(data2.get(), &bot->m_mover->m_movementInfo);
    bot->GetSession()->QueuePacket(data2.get());

    // give the bot some AI, object is owned by the player class
    /*PlayerbotAI* ai = new PlayerbotAI(*this, bot, m_confDebugWhisper);
    bot->SetPlayerbotAI(ai);*/

    // tell the world session that they now manage this new bot
    _playerBotsStore.emplace(botGuid, bot);

    // if bot is in a group and master is not in group then
    // have bot leave their group
    if (bot->GetGroup() && (!_master->GetGroup() || !_master->GetGroup()->IsMember(bot->GetGUID())))
        bot->RemoveFromGroup();

    // sometimes master can lose leadership, pass leadership to master check
    const ObjectGuid masterGuid = _master->GetGUID();
    if (_master->GetGroup() && !_master->GetGroup()->IsLeader(masterGuid))
    {
        // But only do so if one of the master's bots is leader
        DoForAllBots([this, masterGuid](ObjectGuid const& botGuid, PlayerBot* playerBot)
        {
            if (!_master->GetGroup()->IsLeader(playerBot->GetGUID()))
                return;

            _master->GetGroup()->ChangeLeader(masterGuid);
        });
    }
}

void MasterBot::RemoveAllBotsFromGroup()
{
    DoForAllBots([this](ObjectGuid const& botGuid, PlayerBot* playerBot)
    {
        if (playerBot->IsInSameGroupWith(_master))
            _master->GetGroup()->RemoveMember(botGuid);
    });
}

void MasterBot::DoForAllBots(std::function<void(ObjectGuid const&, PlayerBot*)> const& execute)
{
    for (auto& [botGuid, playerBot] : _playerBotsStore)
        execute(botGuid, playerBot);
}
