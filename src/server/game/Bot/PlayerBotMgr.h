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

#ifndef _PLAYER_BOT_MGR_H_
#define _PLAYER_BOT_MGR_H_

#include "Define.h"
#include "Duration.h"
#include <array>
#include <unordered_map>

class Player;
class PlayerBot;

struct BotsData
{
    BotsData() = default;
    ~BotsData();

    Player* Master{ nullptr };
    std::unordered_map<ObjectGuid, PlayerBot*> PlayerBotsStore;
};

class WH_GAME_API PlayerBotMgr
{
public:
    PlayerBotMgr() = default;
    ~PlayerBotMgr() = default;

    static PlayerBotMgr* instance();

    // remove marked bot
    // should be called from worldsession::Update only to avoid possible problem with invalid session or player pointer
    void RemoveBots();

    // This is called from Unit.cpp and is called every second (I think)
    void Update(Milliseconds diff);

    // This is called whenever the master sends a packet to the server.
    // These packets can be viewed, but not edited.
    // It allows bot creators to craft AI in response to a master's actions.
    // For a list of opcodes that can be caught see Opcodes.cpp (CMSG_* opcodes only)
    // Notice: this is static - it is called once for all bots of the master.
    void HandleMasterIncomingPacket(Player* master, WorldPacket const& packet);
    void HandleMasterOutgoingPacket(Player* master, WorldPacket const& packet);

    void LoginPlayerBot(Player* master, ObjectGuid guid);
    void LogoutPlayerBot(Player* master, ObjectGuid guid); // mark bot to be removed on next update
    Player* GetPlayerBot(Player* master, ObjectGuid guid);

    void LogoutAllBots(Player* master, bool fullRemove = false); // mark all bots to be removed on next update
    void RemoveAllBotsFromGroup(Player* master);
    void OnBotLogin(Player* master, PlayerBot* bot);
    void Stay();

private:
    // Config options
    uint32 _restrictBotLevel{ 0 };
    uint32 _disableBotsInRealm{ 0 };
    uint32 _maxNumBots{ 0 };
    bool _disableBots{ false };
    bool _debugWhisper{ false };
    std::array<float, 2> _followDistance{};
    uint32 _sellLevelDiff{ 0 };
    bool _collectCombat{ false };
    bool _collectQuest{ false };
    bool _collectProfession{ false };
    bool _collectLoot{ false };
    bool _collectSkin{ false };
    bool _collectObjects{ false };
    uint32 _collectDistance{ 25 };
    uint32 _collectDistanceMax{ 50 };

    std::unordered_map<Player*, BotsData> _masterStore;

    PlayerBotMgr(PlayerBotMgr const&) = delete;
    PlayerBotMgr(PlayerBotMgr&&) = delete;
    PlayerBotMgr& operator=(PlayerBotMgr const&) = delete;
    PlayerBotMgr& operator=(PlayerBotMgr&&) = delete;
};

#endif
