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

#ifndef _MASTER_BOT_H_
#define _MASTER_BOT_H_

#include "Define.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <functional>

class Player;
class PlayerBot;

class WH_GAME_API MasterBot
{
public:
    explicit MasterBot(Player* master) : _master(master) { }
    ~MasterBot();

    // remove marked bot
    // should be called from worldsession::Update only to avoid possible problem with invalid session or player pointer
    void RemoveBots();

    // This is called whenever the master sends a packet to the server.
    // These packets can be viewed, but not edited.
    // It allows bot creators to craft AI in response to a master's actions.
    // For a list of opcodes that can be caught see Opcodes.cpp (CMSG_* opcodes only)
    // Notice: this is static - it is called once for all bots of the master.
    void HandleMasterIncomingPacket(WorldPacket const& packet);
    void HandleMasterOutgoingPacket(WorldPacket const& packet);

    void LoginPlayerBot(ObjectGuid guid);
    void LogoutPlayerBot(ObjectGuid guid); // mark bot to be removed on next update
    PlayerBot* GetPlayerBot(ObjectGuid guid) const;
    inline Player* GetMaster() { return _master; }

    void LogoutAllBots(bool fullRemove = false); // mark all bots to be removed on next update
    void RemoveAllBotsFromGroup();
    void OnBotLogin(PlayerBot* bot);
    void Stay();

    void DoForAllBots(std::function<void(ObjectGuid const&, PlayerBot*)> const& execute);

private:
    Player* _master{ nullptr };
    std::unordered_map<ObjectGuid, PlayerBot*> _playerBotsStore;
    GuidSet _botsToRemove;
};

#endif
