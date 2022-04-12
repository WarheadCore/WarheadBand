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

#include "AccountMgr.h"
#include "Chat.h"
#include "GameTime.h"
#include "Log.h"
#include "ModuleLocale.h"
#include "ModulesConfig.h"
#include "MuteMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include <Poco/Exception.h>
#include <Poco/RegularExpression.h>
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

    void Init()
    {
        if (!MOD_CONF_GET_BOOL("AntiAD.Enable"))
            return;

        LoadDataFromDB();
    }

    void LoadDataFromDB()
    {
        uint32 oldMSTime = getMSTime();

        LOG_INFO("module.antiad", "Loading anti advertisment...");

        QueryResult result = WorldDatabase.Query("SELECT Pattern FROM `anti_ad_patterns`");
        if (!result)
        {
            LOG_INFO("module.antiad", ">> Loading 0 word. DB table `anti_ad_patterns` is empty.");
            LOG_INFO("module.antiad", "");
            return;
        }

        _pattern = "";

        do
        {
            _pattern += "(" + result->Fetch()->Get<std::string>() + ")|";
        } while (result->NextRow());

        // Delete last (|)
        if (!_pattern.empty())
            _pattern.erase(_pattern.end() - 1, _pattern.end());

        try
        {
            Poco::RegularExpression re(_pattern);
            LOG_INFO("module.antiad", ">> Regular expression successfully loaded in {} ms", GetMSTimeDiffToNow(oldMSTime));
        }
        catch (const Poco::Exception& e)
        {
            LOG_FATAL("module.antiad", ">> {}", e.displayText());
            LOG_FATAL("module.antiad", ">> Regular expression failed loaded ({})", _pattern);

            // Set disable module
            sModulesConfig->SetOption<bool>("AntiAD.Enable", false);
        }

        LOG_INFO("module.antiad", "");
    }

    bool IsNeedCheckChannel(uint8 channelType)
    {
        return channelType & MOD_CONF_GET_INT("AntiAD.Check.Channels");
    }

    bool IsValidMessage(std::string& msg)
    {
        if (_pattern.empty())
            return false;

        try
        {
            Poco::RegularExpression re(_pattern, Poco::RegularExpression::RE_CASELESS);

            if (re.subst(msg, "***", Poco::RegularExpression::RE_GLOBAL))
                return true;
        }
        catch (const Poco::Exception& e)
        {
            LOG_FATAL("module.antiad", "{}", e.displayText());
        }

        return false;
    }

private:
    std::string _pattern;
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

        CheckMessage(player, msg);
    }

    void OnChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg, Player* /*receiver*/) override
    {
        if (!sAD->IsNeedCheckChannel(ANTIAD_CHANNEL_WHISPER))
            return;

        CheckMessage(player, msg);
    }

    void OnChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg, Group* /*group*/) override
    {
        if (!sAD->IsNeedCheckChannel(ANTIAD_CHANNEL_GROUP))
            return;

        CheckMessage(player, msg);
    }

    void OnChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg, Guild* /*guild*/) override
    {
        if (!sAD->IsNeedCheckChannel(ANTIAD_CHANNEL_GUILD))
            return;

        CheckMessage(player, msg);
    }

    void OnChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg, Channel* /*channel*/) override
    {
        if (!sAD->IsNeedCheckChannel(ANTIAD_CHANNEL_CHANNEL))
            return;

        CheckMessage(player, msg);
    }

private:
    void Mute(Player* player)
    {
        if (!MOD_CONF_GET_BOOL("AntiAD.Mute.Player.Enable"))
            return;

        if (AccountMgr::IsGMAccount(player->GetSession()->GetSecurity()) && !MOD_CONF_GET_BOOL("AntiAD.Mute.GM.Enable"))
            return;

        uint32 muteTime = MOD_CONF_GET_INT("AntiAD.Mute.Count");

        sMute->MutePlayer(player->GetName(), Seconds(muteTime), "Console", "Advertisment");

        if (MOD_CONF_GET_BOOL("AntiAD.Send.SelfMessage.Enable"))
            sModuleLocale->SendGlobalMessage(true, "ANTIAD_LOCALE_SEND_SELF", muteTime);
    }

    void SendGMTexts(Player* player, std::string const& message)
    {
        if (!MOD_CONF_GET_BOOL("AntiAD.Send.GMMessage.Enable"))
            return;

        sModuleLocale->SendGlobalMessage(true, "ANTIAD_LOCALE_SEND_GM_TEXT", ChatHandler(player->GetSession()).GetNameLink(player).c_str(), message.c_str());
    }

    void CheckMessage(Player* player, std::string& msg)
    {
        if (!MOD_CONF_GET_BOOL("AntiAD.Enable"))
            return;

        std::string const message = msg;

        if (sAD->IsValidMessage(msg))
        {
            SendGMTexts(player, message);
            Mute(player);
        }
    };
};

class AntiAD_World : public WorldScript
{
public:
    AntiAD_World() : WorldScript("AntiAD_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sModulesConfig->AddOption({ "AntiAD.Enable",
            "AntiAD.Send.GMMessage.Enable",
            "AntiAD.Send.SelfMessage.Enable",
            "AntiAD.Mute.Player.Enable",
            "AntiAD.Mute.GM.Enable",
            "AntiAD.Mute.Count",
            "AntiAD.Check.Channels" });
    }

    void OnStartup() override
    {
        if (!MOD_CONF_GET_BOOL("AntiAD.Enable"))
            return;

        sAD->Init();
    }
};

// Group all custom scripts
void AddSC_AntiAD()
{
    new AntiAD_Player();
    new AntiAD_World();
}
