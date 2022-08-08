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

#ifndef _AIO_MGR_H_
#define _AIO_MGR_H_

#include "AIOMessage.h"
#include "Duration.h"
#include "ObjectGuid.h"
#include <string_view>
#include <unordered_map>
#include <map>
#include <memory>

class Player;

class WH_GAME_API AIOPlayer
{
public:
    AIOPlayer(Player* player) : _player(player) { }

    void Update(Milliseconds diff)
    {
        //AIO Init cooldown
        if (_aioInitCd)
        {
            _aioInitTimer += diff;
            if (_aioInitTimer >= 5s)
            {
                _aioInitCd = false;
                _aioInitTimer = 0ms;
            }
        }
    }

    void UpdateSession(Milliseconds diff)
    {
        for (auto& [messageID, bufferInfo] : _addonMessageBuffer)
        {
            bufferInfo.Timer += diff;
            if (bufferInfo.Timer >= 30s)
                _addonMessageBuffer.erase(messageID);
        }
    }

    inline void SetAIOInt(bool isInit) { _aioInitialized = isInit; }

    void SendSimpleAIOMessage(std::string_view message);

    inline bool IsAIOInitOnCooldown() const { return _aioInitCd; }
    inline void SetAIOIntOnCooldown(bool cd) { _aioInitCd = cd; _aioInitTimer = 0ms; }

    bool SendAddonMessage(Player* receiver, std::string_view message);

    template<typename... Ts>
    inline std::enable_if_t<std::conjunction_v<std::is_same<LuaVal, std::decay_t<Ts>>...>>
        AIOHandle(LuaVal const& scriptKey, LuaVal const& handlerKey, Ts&&... pack)
    {
        AIOMessage msg{ scriptKey, handlerKey, std::forward<Ts>(pack)... };
        SendSimpleAIOMessage(msg.dumps());
    }

    // Forces reload on the player AIO addons
    // Syncs player AIO addons with server
    inline void ForceReloadAddons() { AIOHandle("AIO", "ForceReload"); }

    // Force reset on the player AIO addons
    // Player AIO addons and addon data is deleted and downloaded again
    inline void ForceResetAddons() { AIOHandle("AIO", "ForceReset"); }

private:
    struct LongMessageBufferInfo
    {
        LongMessageBufferInfo() = default;

        uint32 Parts{ 0 };
        Milliseconds Timer{ 0ms };
        std::map<uint32, std::string> Map;
    };

    Player* _player;
    bool _aioInitialized{ false };
    bool _aioInitCd{ false };
    Milliseconds _aioInitTimer{ 0ms };
    uint16 _messageIdIndex{ 1 };
    std::map<uint16, LongMessageBufferInfo> _addonMessageBuffer;
};

struct WH_GAME_API AIOAddon
{
    // AIOAddon container constructor
    AIOAddon(std::string_view addonName, std::string_view addonFile)
        : name(addonName), file(addonFile) { }

    std::string name;
    std::string code;
    std::string file;
    uint32 crc{ 0 };
};

class WH_GAME_API AIOMgr
{
    AIOMgr() = default;
    ~AIOMgr() = default;

    AIOMgr(AIOMgr const&) = delete;
    AIOMgr(AIOMgr&&) = delete;
    AIOMgr& operator=(AIOMgr const&) = delete;
    AIOMgr& operator=(AIOMgr&&) = delete;

public:
    static AIOMgr* Instance();

    void AddAIOMessage(Player* player, AIOMessage* msg);

    void SendAIOMessage(Player* player, std::string_view message);

    bool IsAIOInitOnCooldown(Player* player);
    void SetAIOIntOnCooldown(Player* player, bool cd);
    void SetAIOInt(Player* player, bool isInit);

    bool SendAddonMessage(Player* player, Player* receiver, std::string_view message);

    // AIO prefix configured in worldserver.conf
    std::string_view GetAIOPrefix() const { return _aioPrefix; }

    // AIO client LUA files path configured in worldserver.conf
    std::string_view GetAIOClientScriptPath() const { return _aioClientpath; }

    // Forces reload on all player AIO addons
    // Syncs player AIO addons with server
    void ForceReloadPlayerAddons();

    // Forces reset on all player AIO addons
    // Player AIO addons and addon data is deleted and downloaded again
    void ForceResetPlayerAddons();

    // Sends an AIO message to all players
    // See: class AIOMsg
    void AIOMessageAll(AIOMessage& msg);

    // Sends a simple string message to all players

    // AIO can only understand smallfolk LuaVal::dumps() format
    // Handler functions are called by creating a table as below
    // {
    //     {n, ScriptName, HandlerName(optional), Arg1..N(optional) },
    //     {n, AnotherScriptName, AnotherHandlerName(optional), Arg1..N(optional) }
    // }
    // Where n is number of arguments including handler name as a argument
    void SendAllSimpleAIOMessage(std::string_view message);

    // Reloads client side AIO addon files and force reloads
    // all player AIO addons
    void ReloadAddons();

    // Adds a WoW AIO addon file to the list of addons with a unique
    // addon name to send on AIO client initialization.
    // Returns true if addon was added, false if addon name is already taken
    //
    // It is required to call World::ForceReloadPlayerAddons()
    // if addons are added after server is fully initialized
    // for online players to load the added addons.
    bool AddAddon(AIOAddon const& addon);

    // Removes an addon from addon list and force reloads affected players
    // Returns permission id if an addon was removed, 0 if addon not found
    //
    // It is required to call World::ForceReloadPlayerAddons()
    // if addons are added after server is fully initialized
    // for online players to load the added addons.
    bool RemoveAddon(std::string_view addonName);

    // For internal use only
    std::size_t PrepareClientAddons(LuaVal& clientData, LuaVal& addonsTable, LuaVal& cacheTable, Player* forPlayer) const;

private:
    AIOPlayer* GetAIOPlayer(ObjectGuid playerGuid);

    std::unordered_map<ObjectGuid, std::unique_ptr<AIOPlayer>> _players;
    std::string _aioPrefix;
    std::string _aioClientpath;
    std::vector<AIOAddon> _addonList;
};

#define sAIOMgr AIOMgr::Instance()

class WH_GAME_API AIOScript
{
public:
    using HandlerFunc = std::function<void(Player*, const LuaVal&)>;
    using ArgFunc = std::function<LuaVal(Player*)>;

    // Registers an AIO Handler script of scriptName
    AIOScript(const LuaVal& scriptKey);

    // Returns the key of this CAIO script
    LuaVal GetKey() const { return _key; }

    // Registers a handler function to call when handling
    // handleKey of this script.
    void AddHandler(const LuaVal& handlerKey, HandlerFunc function) { _handlerMap[handlerKey] = function; }

    // Adds a client side handler to call and adds arguments
    // to sends with it for AIO client initialization.
    //
    // You can add additional arguments to the handler by
    // calling this function again
    void AddInitArgs(const LuaVal& scriptKey, const LuaVal& handlerKey, std::vector<ArgFunc> const& args);

    // Adds a WoW addon file to the list of addons with a unique
    // addon key to send on AIO client initialization.
    // Returns true if addon was added, false if addon key is taken.
    //
    // It is required to call World::ForceReloadPlayerAddons()
    // if addons are added after server is fully initialized
    // for online players to load the added addons.
    bool AddAddon(AIOAddon const& addon);

    void OnHandle(Player* sender, const LuaVal& handlerKey, const LuaVal& args);

private:
    LuaVal _key;
    std::unordered_map<LuaVal, HandlerFunc, LuaVal::LuaValHasher> _handlerMap;
};

class WH_GAME_API AIOHandler : public AIOScript
{
public:
    AIOHandler();

    void HandleInit(Player* sender, LuaVal const& args);
    void HandleError(Player* sender, LuaVal const& args);

    struct InitHookInfo
    {
        InitHookInfo(LuaVal const& scriptKey, LuaVal const& handlerKey)
            : ScriptKey(scriptKey), HandlerKey(handlerKey) { }

        LuaVal ScriptKey;
        LuaVal HandlerKey;
        std::vector<AIOScript::ArgFunc> ArgsList;
    };

    inline auto GetHandlersList() { return &_hookList; }

private:
    std::vector<InitHookInfo> _hookList;
};

class WH_GAME_API AIOScriptMgr
{
    AIOScriptMgr();
    ~AIOScriptMgr() = default;

    AIOScriptMgr(AIOScriptMgr const&) = delete;
    AIOScriptMgr(AIOScriptMgr&&) = delete;
    AIOScriptMgr& operator=(AIOScriptMgr const&) = delete;
    AIOScriptMgr& operator=(AIOScriptMgr&&) = delete;

public:
    static AIOScriptMgr* instance();

    void AddAIOScript(LuaVal const& scriptKey, AIOScript* script);
    AIOScript* GetAIOScript(LuaVal const& scriptKey);
    AIOHandler* GetHandler();

private:
    std::unordered_map<LuaVal, std::unique_ptr<AIOScript>, LuaVal::LuaValHasher> _scripts;
    std::unique_ptr<AIOHandler> _handler;
};

#define sAIOScriptMgr AIOScriptMgr::instance()

#endif //
