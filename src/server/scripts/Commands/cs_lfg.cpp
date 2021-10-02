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

#include "Chat.h"
#include "Group.h"
#include "LFGMgr.h"
#include "Language.h"
#include "Player.h"
#include "ScriptMgr.h"

#if WARHEAD_COMPILER == WARHEAD_COMPILER_GNU
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

void GetPlayerInfo(ChatHandler*  handler, Player* player)
{
    if (!player)
        return;

    ObjectGuid guid = player->GetGUID();
    lfg::LfgDungeonSet dungeons = sLFGMgr->GetSelectedDungeons(guid);

    std::string const& state = lfg::GetStateString(sLFGMgr->GetState(guid));
    handler->PSendSysMessage(LANG_LFG_PLAYER_INFO, player->GetName(),
                             state.c_str(), uint8(dungeons.size()), lfg::ConcatenateDungeons(dungeons).c_str(),
                             lfg::GetRolesString(sLFGMgr->GetRoles(guid)).c_str(), sLFGMgr->GetComment(guid).c_str());
}

using namespace Warhead::ChatCommands;

class lfg_commandscript : public CommandScript
{
public:
    lfg_commandscript() : CommandScript("lfg_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable lfgCommandTable =
        {
            { "player",    SEC_MODERATOR,     false, &HandleLfgPlayerInfoCommand, "" },
            { "group",     SEC_MODERATOR,     false,  &HandleLfgGroupInfoCommand, "" },
            { "queue",     SEC_MODERATOR,     false,  &HandleLfgQueueInfoCommand, "" },
            { "clean",     SEC_ADMINISTRATOR, false,      &HandleLfgCleanCommand, "" },
            { "options",   SEC_GAMEMASTER,    false,    &HandleLfgOptionsCommand, "" },
        };

        static ChatCommandTable commandTable =
        {
            {  "lfg",   SEC_GAMEMASTER, false,                           nullptr, "", lfgCommandTable },
        };
        return commandTable;
    }

    static bool HandleLfgPlayerInfoCommand(ChatHandler* handler, char const* args)
    {
        Player* target = nullptr;
        std::string playerName;
        if (!handler->extractPlayerTarget((char*)args, &target, nullptr, &playerName))
            return false;

        GetPlayerInfo(handler, target);
        return true;
    }

    static bool HandleLfgGroupInfoCommand(ChatHandler* handler, char const* args)
    {
        Player* target = nullptr;
        std::string playerName;
        if (!handler->extractPlayerTarget((char*)args, &target, nullptr, &playerName))
            return false;

        Group* grp = target->GetGroup();
        if (!grp)
        {
            handler->PSendSysMessage(LANG_LFG_NOT_IN_GROUP, playerName);
            return true;
        }

        ObjectGuid guid = grp->GetGUID();
        std::string const& state = lfg::GetStateString(sLFGMgr->GetState(guid));
        handler->PSendSysMessage(LANG_LFG_GROUP_INFO, grp->isLFGGroup(),
                                 state.c_str(), sLFGMgr->GetDungeon(guid));

        for (GroupReference* itr = grp->GetFirstMember(); itr != nullptr; itr = itr->next())
            GetPlayerInfo(handler, itr->GetSource());

        return true;
    }

    static bool HandleLfgOptionsCommand(ChatHandler* handler, char const* args)
    {
        int32 options = -1;
        if (char* str = strtok((char*)args, " "))
        {
            int32 tmp = atoi(str);
            if (tmp > -1)
                options = tmp;
        }

        if (options != -1)
        {
            sLFGMgr->SetOptions(options);
            handler->PSendSysMessage(LANG_LFG_OPTIONS_CHANGED);
        }
        handler->PSendSysMessage(LANG_LFG_OPTIONS, sLFGMgr->GetOptions());
        return true;
    }

    static bool HandleLfgQueueInfoCommand(ChatHandler*  /*handler*/, char const*  /*args*/)
    {
        return true;
    }

    static bool HandleLfgCleanCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->PSendSysMessage(LANG_LFG_CLEAN);
        sLFGMgr->Clean();
        return true;
    }
};

void AddSC_lfg_commandscript()
{
    new lfg_commandscript();
}
