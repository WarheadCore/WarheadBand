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

#include "AIOMgr.h"
#include "AIOMessage.h"
#include "Player.h"
#include "StringFormat.h"
#include <fstream>
#include <boost/crc.hpp>

void AIOPlayer::SendSimpleAIOMessage(std::string_view message)
{
    if (message.empty())
        return;

    std::string aioPrefix{ sAIOMgr->GetAIOPrefix() };
    size_t shortMsgLen = message.size() + aioPrefix.size() + 4; //+4 for S \t and 2 byte for message id

    //If its a short message
    if (shortMsgLen <= 2600)
    {
        std::string msg{ Warhead::StringFormat("S{}\t\x1\x1{}", aioPrefix, message) };

        WorldPacket data(SMSG_MESSAGECHAT, msg.size() + 30);
        data << uint8(CHAT_MSG_WHISPER);
        data << int32(LANG_ADDON);
        data << _player->GetGUID();
        data << uint32(0);
        data << _player->GetGUID();
        data << uint32(msg.size() + 1);
        data << msg;
        data << uint8(0);
        _player->SendDirectMessage(&data);
        return;
    }

    //If its a long message
    uint16 parts = std::ceilf(float(shortMsgLen + 4) / 2600);

    //parts to string
    uint16 high = std::floorf((float)parts / 254);
    std::string partsStr(1, high + 1);
    partsStr += parts - high * 254 + 1;

    //messageid to string
    high = std::floorf((float)_messageIdIndex / 254);
    std::string messageIdStr(1, high + 1);
    messageIdStr += _messageIdIndex - high * 254 + 1;

    //Increase or renew messageIdIndex
    if (_messageIdIndex >= 64769) //2^16 - 767
    {
        _messageIdIndex = 1;
    }
    else
    {
        ++_messageIdIndex;
    }

    //Send in parts
    size_t cursor = 0;
    for (uint16 partId = 1; partId <= parts; ++partId)
    {
        //partid to string
        high = std::floorf((float)partId / 254);
        std::string partIdStr(1, high + 1);
        partIdStr += partId - high * 254 + 1;

        //send
        std::string fullmsg = "S" + aioPrefix + "\t" + messageIdStr + partsStr + partIdStr;
        fullmsg += message.substr(cursor, 2600);
        WorldPacket data(SMSG_MESSAGECHAT, fullmsg.size() + 30);
        data << uint8(CHAT_MSG_WHISPER);
        data << int32(LANG_ADDON);
        data << _player->GetGUID();
        data << uint32(0);
        data << _player->GetGUID();
        data << uint32(fullmsg.size() + 1);
        data << fullmsg;
        data << uint8(0);
        _player->SendDirectMessage(&data);

        cursor += 2600;
    }
}

bool AIOPlayer::SendAddonMessage(Player* receiver, std::string_view message)
{
    //AIO
    size_t delimPos;
    if (!receiver)
        return false;

    std::string_view prefix;
    delimPos = message.find('\t');

    if (message.find('\t') == std::string_view::npos)
        return false;

    prefix = message.substr(0, delimPos);
    std::string aioPrefix{ "C" };
    aioPrefix.append(sAIOMgr->GetAIOPrefix());

    if (prefix != aioPrefix)
        return false;

    if (receiver != _player)
        return false;

    //Must have meta data
    uint16 messageId;
    if ((message.size() - delimPos - 1) >= 2)
    {
        messageId = (message[delimPos + 1] - 1) * 254 + message[delimPos + 2] - 1;

        //If its a short message
        if (messageId == 0) //messageId = 0
        {
            sScriptMgr->OnAddonMessage(_player, message.substr(delimPos + 3));
            return true;
        }
    }

    //Its a long message
    if ((message.size() - delimPos - 1) >= 6)
    {
        uint32 parts = (message[delimPos + 3] - 1) * 254 + message[delimPos + 4] - 1;
        if (parts < 2)
        {
            LOG_ERROR("aio", "HandleAddonMessagechatOpcode: Received AIO addon message with number of parts: {} (< 2). Message Id: {}, Sender: {}", parts, messageId, _player->GetGUID().ToString());
            return true;
        }

        uint32 maxparts = sWorld->getIntConfig(CONFIG_AIO_MAXPARTS);
        if (parts > maxparts)
        {
            LOG_ERROR("aio", "HandleAddonMessagechatOpcode: Received AIO addon message with too many parts: {} (> {}). Message Id: {}, Sender: {}", parts, maxparts, messageId, _player->GetGUID().ToString());
            return true;
        }

        uint32 partId = (message[delimPos + 5] - 1) * 254 + message[delimPos + 6] - 1;

        //Check if message exists
        auto itr = _addonMessageBuffer.find(messageId);
        if (itr == _addonMessageBuffer.end())
        {
            itr = _addonMessageBuffer.insert(std::make_pair(messageId, LongMessageBufferInfo())).first;
        }
        else
        {
            //If message already exist and has different number of parts remove it
            if (parts != itr->second.Parts)
            {
                itr->second = LongMessageBufferInfo();
            }
        }

        itr->second.Parts = parts;
        itr->second.Map[partId] = message.substr(delimPos + 7);

        //If there are enough parts
        if (itr->second.Map.size() >= itr->second.Parts)
        {
            //Assemble the parts
            std::string actualAIOMessage;
            for (auto& itr2 : itr->second.Map)
                actualAIOMessage += itr2.second;

            sScriptMgr->OnAddonMessage(_player, actualAIOMessage);
            _addonMessageBuffer.erase(itr);
            return true;
        }
        else //Or else wait for other packets to arrive
            return true;
    }

    LOG_ERROR("aio", "HandleAddonMessagechatOpcode: Received AIO long addon message without meta data, Sender: {}", _player->GetGUID().ToString());
    return true;
}

AIOMgr* AIOMgr::Instance()
{
    static AIOMgr instance;
    return &instance;
}

void AIOMgr::AddAIOMessage(Player* player, AIOMessage* msg)
{
    SendAIOMessage(player, msg->dumps());
}

void AIOMgr::SendAIOMessage(Player* player, std::string_view message)
{
    auto aioPlayer = GetAIOPlayer(player->GetGUID());
    if (!aioPlayer)
        return;

    aioPlayer->SendSimpleAIOMessage(message);
}

bool AIOMgr::IsAIOInitOnCooldown(Player* player)
{
    auto aioPlayer = GetAIOPlayer(player->GetGUID());
    if (!aioPlayer)
        return false;

    return aioPlayer->IsAIOInitOnCooldown();
}

void AIOMgr::SetAIOIntOnCooldown(Player* player, bool cd)
{
    auto aioPlayer = GetAIOPlayer(player->GetGUID());
    if (!aioPlayer)
        return;

    aioPlayer->SetAIOIntOnCooldown(cd);
}

void AIOMgr::SetAIOInt(Player* player, bool isInit)
{
    auto aioPlayer = GetAIOPlayer(player->GetGUID());
    if (!aioPlayer)
        return;

    aioPlayer->SetAIOInt(isInit);
}

AIOPlayer* AIOMgr::GetAIOPlayer(ObjectGuid playerGuid)
{
    auto const& itr = _players.find(playerGuid);
    if (itr == _players.end())
        return nullptr;

    return itr->second.get();
}

bool AIOMgr::SendAddonMessage(Player* player, Player* receiver, std::string_view message)
{
    auto aioPlayer = GetAIOPlayer(player->GetGUID());
    if (!aioPlayer)
        return false;

    return aioPlayer->SendAddonMessage(receiver, message);
}

bool AIOMgr::AddAddon(AIOAddon const& addon)
{
    if (addon.file.empty())
        return false;

    //Check if addon already exist
    for (auto const& _addon : _addonList)
    {
        if (_addon.name == addon.name)
            return false;
    }

    AIOAddon copy(addon);
    copy.code = "";

    //Format path
    std::string path{ _aioClientpath };
    if (path.back() != '/' && path.back() != '\\')
        path += '/';

    path += copy.file;

    //Get file
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (in)
    {
        in.seekg(0, std::ios::end);
        copy.code.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&copy.code[0], copy.code.size());
        in.close();

        if (copy.code.empty())
            return false;
    }
    else
    {
        LOG_ERROR("aio", "AIO AddAddon: Couldn't open file {} of addon {}", path, copy.name);
        return false;
    }

    //Set crc on original file content
    boost::crc_32_type crc_result;
    crc_result.process_bytes(copy.code.data(), copy.code.length());
    copy.crc = crc_result.checksum();

    //Process code
    char compressPrefix = 'U';

    //Set final code and go
    copy.code = std::string(1, compressPrefix) + copy.code;
    _addonList.emplace_back(copy);

    LOG_INFO("aio", "AIO: Loaded addon {} from file {}", copy.name, copy.file);
    return true;
}

bool AIOMgr::RemoveAddon(std::string_view addonName)
{
    auto eraseCount = std::erase_if(_addonList, [addonName](AIOAddon const& addon)
    {
        return addon.name == addonName;
    });

    return eraseCount > 0;
}

void AIOMgr::ReloadAddons()
{
    LOG_INFO("aio", "AIO: Reload addons");

    std::vector<AIOAddon> prevAddonList;
    prevAddonList.swap(_addonList);

    for (auto const& addon : prevAddonList)
        AddAddon(addon);
}

size_t AIOMgr::PrepareClientAddons(LuaVal& clientData, LuaVal& addonsTable, LuaVal& cacheTable, Player* forPlayer) const
{
    uint32 count{ 0 };

    for (auto const& addon : _addonList)
    {
        LuaVal& CRCVal = clientData[addon.name];
        if (CRCVal == addon.crc)
        {
            cacheTable[++count] = addon.name;
        }
        else
        {
            LuaVal addonData(TTABLE);
            addonData["name"] = addon.name;
            addonData["crc"] = addon.crc;
            addonData["code"] = addon.code;
            addonsTable[++count] = addonData;
        }
    }

    return count;
}

void AIOMgr::ForceReloadPlayerAddons()
{
    for (auto const& [guid, aioPlayer] : _players)
        aioPlayer->ForceReloadAddons();
}

void AIOMgr::ForceResetPlayerAddons()
{
    for (auto const& [guid, aioPlayer] : _players)
        aioPlayer->ForceResetAddons();
}

void AIOMgr::AIOMessageAll(AIOMessage& msg)
{
    std::string messageStr = msg.dumps();
    SendAllSimpleAIOMessage(messageStr);
}

void AIOMgr::SendAllSimpleAIOMessage(std::string_view message)
{
    for (auto const& [guid, aioPlayer] : _players)
        aioPlayer->SendSimpleAIOMessage(message);
}

///
constexpr auto AIO_VERSION = 1.72;

AIOScript::AIOScript(LuaVal const& scriptKey)
    : _key(scriptKey)
{
    sAIOScriptMgr->AddAIOScript(scriptKey, this);
}

void AIOScript::AddInitArgs(const LuaVal& scriptKey, const LuaVal& handlerKey, std::vector<ArgFunc> const& args)
{
    auto handler = sAIOScriptMgr->GetHandler();
    if (!handler)
        return;

    //Look for hook
    std::vector<ArgFunc>* list{ nullptr };

    for (auto& hookInfo : *handler->GetHandlersList())
    {
        if (hookInfo.ScriptKey == scriptKey && hookInfo.HandlerKey == handlerKey)
        {
            list = &hookInfo.ArgsList;
            break;
        }
    }

    //Add hook
    if (!list)
    {
        handler->GetHandlersList()->emplace_back(scriptKey, handlerKey);
        list = &handler->GetHandlersList()->back().ArgsList;
    }

    if (args.empty())
        return;

    //Add args
    for (auto& arg : args)
        list->emplace_back(arg);
}

bool AIOScript::AddAddon(AIOAddon const& addon)
{
    return sAIOMgr->AddAddon(addon);
}

void AIOScript::OnHandle(Player* sender, const LuaVal& handlerKey, const LuaVal& args)
{
    auto const& itr = _handlerMap.find(handlerKey);
    if (itr != _handlerMap.end())
        itr->second(sender, args); //Call the handler function
}

AIOHandler::AIOHandler()
    : AIOScript("AIO")
{
    AddHandler("Init", std::bind(&AIOHandler::HandleInit, this, std::placeholders::_1, std::placeholders::_2));
    AddHandler("Error", std::bind(&AIOHandler::HandleError, this, std::placeholders::_1, std::placeholders::_2));
}

void AIOHandler::HandleInit(Player* sender, LuaVal const& args)
{
    //Init hasn't cooled down
    if (sAIOMgr->IsAIOInitOnCooldown(sender))
        return;

    sAIOMgr->SetAIOIntOnCooldown(sender, true);

    LuaVal versionVal{ args[4] };
    LuaVal clientDataVal{ args[5] };

    if (!versionVal.isnumber() || !clientDataVal.istable())
    {
        LOG_ERROR("aio", "AIOHandlers::HandleInit: Invalid version value or clientData value. Sender: {}, Args: {}", sender->GetName(), args.dumps());
        return;
    }

    if (versionVal.num() != AIO_VERSION)
    {
        AIOMessage msg{ "AIO", "Init", AIO_VERSION };
        sAIOMgr->AddAIOMessage(sender, &msg);
        return;
    }

    LuaVal addonTable(TTABLE);
    LuaVal cacheTable(TTABLE);
    uint32 nAddons = sAIOMgr->PrepareClientAddons(clientDataVal, addonTable, cacheTable, sender);

    LuaVal argsToSend(TTABLE);

    uint32 blockIndex = 1;

    for (auto const& hookInfo : _hookList)
    {
        uint32 index = 3;
        LuaVal HookBlock(TTABLE);

        HookBlock[1] = (uint32)hookInfo.ArgsList.size() + 1;
        HookBlock[2] = hookInfo.ScriptKey;
        HookBlock[3] = hookInfo.HandlerKey;

        for (auto const& arg : hookInfo.ArgsList)
            HookBlock[++index] = arg(sender);

        argsToSend[++blockIndex] = HookBlock;
    }

    LuaVal AIOInitBlock(TTABLE);
    AIOInitBlock[1] = 5;
    AIOInitBlock[2] = "AIO";
    AIOInitBlock[3] = "Init";
    AIOInitBlock[4] = AIO_VERSION;
    AIOInitBlock[5] = nAddons;
    AIOInitBlock[6] = addonTable;
    AIOInitBlock[7] = cacheTable;

    argsToSend[1] = AIOInitBlock;
    sAIOMgr->SendAIOMessage(sender, argsToSend.dumps());
    sAIOMgr->SetAIOInt(sender, true);
}

void AIOHandler::HandleError(Player* sender, LuaVal const& args)
{
    LuaVal msgVal{ args[4] };
    if (!msgVal.isstring())
        return;

    LOG_ERROR("aio", "AIO: {} Received client addon error: {}", sender->GetSession()->GetPlayerInfo(), msgVal.str());
}

///

AIOScriptMgr::AIOScriptMgr()
{
    _handler = std::make_unique<AIOHandler>();
}

/*static*/ AIOScriptMgr* AIOScriptMgr::instance()
{
    static AIOScriptMgr instance;
    return &instance;
}

AIOScript* AIOScriptMgr::GetAIOScript(LuaVal const& scriptKey)
{
    auto const& itr = _scripts.find(scriptKey);
    if (itr == _scripts.end())
        return nullptr;

    return itr->second.get();
}

void AIOScriptMgr::AddAIOScript(LuaVal const& scriptKey, AIOScript* script)
{
    ASSERT(!_scripts.contains(scriptKey), "AIO scriptKey '{}' of type tag '{}' already exist. Use another key.", scriptKey.tostring(), scriptKey.type());
    _scripts.emplace(scriptKey, std::make_unique<AIOScript>(script));
}

AIOHandler* AIOScriptMgr::GetHandler()
{
    if (!_handler)
        return nullptr;

    return _handler.get();
}
