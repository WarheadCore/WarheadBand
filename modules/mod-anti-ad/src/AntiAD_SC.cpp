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

#include "AccountMgr.h"
#include "Chat.h"
#include "GameTime.h"
#include "Log.h"
#include "ModuleLocale.h"
#include "ModulesConfig.h"
#include "MuteMgr.h"
#include "Player.h"
#include "ScriptObject.h"
#include "StopWatch.h"
#include <Warhead/RegularExpression.h>
#include <Warhead/RegularExpressionException.h>
#include <vector>

enum AntiADChannelsType : uint8
{
    ANTIAD_CHANNEL_SAY      = 1,
    ANTIAD_CHANNEL_WHISPER  = 2,
    ANTIAD_CHANNEL_GROUP    = 4,
    ANTIAD_CHANNEL_GUILD    = 8,
    ANTIAD_CHANNEL_CHANNEL  = 16,
};

class AntiAD
{
public:
    static AntiAD* instance()
    {
        static AntiAD instance;
        return &instance;
    }

    inline void LoadConfig()
    {
        _isEnable = MOD_CONF_GET_BOOL("AntiAD.Enable");
        if (!_isEnable)
            return;

        _isEnableGMMessage = MOD_CONF_GET_BOOL("AntiAD.MessageSend.GM.Enable");
        _isEnableSelfMessage = MOD_CONF_GET_BOOL("AntiAD.MessageSend.Self.Enable");
        _isEnableMutePlayer = MOD_CONF_GET_BOOL("AntiAD.Mute.Player.Enable");
        _isEnableMuteGM = MOD_CONF_GET_BOOL("AntiAD.Mute.GM.Enable");
        _muteDuration = Minutes(MOD_CONF_GET_INT("AntiAD.Mute.Duration"));
        _checkChannelsFlag = MOD_CONF_GET_INT("AntiAD.Check.Channels");
    }

    inline void Init()
    {
        if (!_isEnable)
            return;

        LoadDataFromDB();
    }

    inline void LoadDataFromDB()
    {
        StopWatch sw;

        LOG_INFO("module.antiad", "Loading anti advertisment...");

        // Init default re
        _reDefault = std::make_unique<Warhead::RegularExpression>("[wW]\s*[oO]\s*[wW]", Warhead::RegularExpression::RE_MULTILINE);

        QueryResult result = WorldDatabase.Query("SELECT Pattern FROM `anti_ad_patterns`");
        if (!result)
        {
            LOG_INFO("module.antiad", ">> Loading 0 patterns. DB table `anti_ad_patterns` is empty.");
            LOG_INFO("module.antiad", "");
            return;
        }

        _reStore.clear();

        do
        {
            auto const& [pattern] = result->FetchTuple<std::string>();
            if (pattern.empty())
            {
                LOG_ERROR("module.antiad", "Found empty pattern, skip");
                continue;
            }

            try
            {
                auto re = std::make_unique<Warhead::RegularExpression>(pattern, Warhead::RegularExpression::RE_MULTILINE);
                _reStore.emplace_back(std::move(re));
            }
            catch (Warhead::RegularExpressionException const& e)
            {
                LOG_ERROR("module.antiad", ">> Warhead::RegularExpressionException: {}", e.GetErrorMessage());
            }
        } while (result->NextRow());

        LOG_INFO("module.antiad", ">> Loading {} pattern in {}", _reStore.size(), sw);
        LOG_INFO("module.antiad", "");
    }

    inline bool IsNeedCheckChannel(uint8 channelType)
    {
        if (!_isEnable)
            return false;

        return channelType & MOD_CONF_GET_INT("AntiAD.Check.Channels");
    }

    inline bool IsValidMessage(std::string& msg)
    {
        try
        {
            if (_reDefault->subst(msg, "***", Warhead::RegularExpression::RE_GLOBAL))
            {
                msg = "$%^&";
                return true;
            }
        }
        catch (Warhead::RegularExpressionException const& e)
        {
            LOG_FATAL("module.antiad", "Warhead::RegularExpressionException: {}", e.GetErrorMessage());
        }

        if (_reStore.empty())
            return false;

        try
        {
            for (auto const& re : _reStore)
                if (re->subst(msg, "***", Warhead::RegularExpression::RE_GLOBAL))
                    return true;
        }
        catch (Warhead::RegularExpressionException const& e)
        {
            LOG_FATAL("module.antiad", "Warhead::RegularExpressionException: {}", e.GetErrorMessage());
        }

        return false;
    }

    inline void MutePlayer(Player* player)
    {
        if (!_isEnableMutePlayer)
            return;

        if (AccountMgr::IsGMAccount(player->GetSession()->GetSecurity()) && !_isEnableMuteGM)
            return;

        sMute->MutePlayer(player->GetName(), _muteDuration, "Console", "Anti ad system");

        if (_isEnableSelfMessage)
            sModuleLocale->SendPlayerMessage(player, "ANTIAD_LOCALE_SEND_SELF", _muteDuration.count());
    }

    inline void SendGMTexts(Player* player, std::string_view message)
    {
        if (!_isEnableGMMessage)
            return;

        sModuleLocale->SendGlobalMessage(true, "ANTIAD_LOCALE_SEND_GM_TEXT", ChatHandler(player->GetSession()).GetNameLink(player), message);
    }

    inline void CheckMessage(Player* player, std::string& msg)
    {
        if (!_isEnable)
            return;

        std::string const message = msg;

        if (IsValidMessage(msg))
        {
            SendGMTexts(player, message);
            MutePlayer(player);
        }
    };

private:
    bool _isEnable{ false };
    bool _isEnableGMMessage{ false };
    bool _isEnableSelfMessage{ false };
    bool _isEnableMutePlayer{ false };
    bool _isEnableMuteGM{ false };
    Minutes _muteDuration{ 0min };
    int32 _checkChannelsFlag{ 0 };
    std::vector<std::unique_ptr<Warhead::RegularExpression>> _reStore;
    std::unique_ptr<Warhead::RegularExpression> _reDefault;
};

#define sAD AntiAD::instance()

class AntiAD_Player : public PlayerScript
{
public:
    AntiAD_Player() : PlayerScript("AntiAD_Player") { }

    void OnChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg) override
    {
        if (!sAD->IsNeedCheckChannel(ANTIAD_CHANNEL_SAY))
            return;

        sAD->CheckMessage(player, msg);
    }

    void OnChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg, Player* /*receiver*/) override
    {
        if (!sAD->IsNeedCheckChannel(ANTIAD_CHANNEL_WHISPER))
            return;

        sAD->CheckMessage(player, msg);
    }

    void OnChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg, Group* /*group*/) override
    {
        if (!sAD->IsNeedCheckChannel(ANTIAD_CHANNEL_GROUP))
            return;

        sAD->CheckMessage(player, msg);
    }

    void OnChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg, Guild* /*guild*/) override
    {
        if (!sAD->IsNeedCheckChannel(ANTIAD_CHANNEL_GUILD))
            return;

        sAD->CheckMessage(player, msg);
    }

    void OnChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg, Channel* /*channel*/) override
    {
        if (!sAD->IsNeedCheckChannel(ANTIAD_CHANNEL_CHANNEL))
            return;

        sAD->CheckMessage(player, msg);
    }
};

class AntiAD_World : public WorldScript
{
public:
    AntiAD_World() : WorldScript("AntiAD_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sAD->LoadConfig();
    }

    void OnStartup() override
    {
        sAD->Init();
    }
};

// Group all custom scripts
void AddSC_AntiAD()
{
    new AntiAD_Player();
    new AntiAD_World();
}
