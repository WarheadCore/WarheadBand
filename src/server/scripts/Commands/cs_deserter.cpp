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

/**
* @file cs_deserter.cpp
* @brief .deserter related commands
*
* This file contains the CommandScripts for all deserter sub-commands
*/

#include "Chat.h"
#include "Language.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"

using namespace Warhead::ChatCommands;

enum Spells
{
    LFG_SPELL_DUNGEON_DESERTER = 71041,
    BG_SPELL_DESERTER = 26013
};

#if WARHEAD_COMPILER == WARHEAD_COMPILER_GNU
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

using namespace Warhead::ChatCommands;

class deserter_commandscript : public CommandScript
{
public:
    deserter_commandscript() : CommandScript("deserter_commandscript") { }

    /**
    * @brief Returns the command structure for the system.
    */

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable deserterInstanceCommandTable =
        {
            { "add",        HandleDeserterInstanceAdd,       SEC_ADMINISTRATOR, Console::Yes },
            { "remove all", HandleDeserterInstanceRemoveAll, SEC_ADMINISTRATOR, Console::Yes },
            { "remove",     HandleDeserterInstanceRemove,    SEC_ADMINISTRATOR, Console::Yes }
        };
        static ChatCommandTable deserterBGCommandTable =
        {
            { "add",        HandleDeserterBGAdd,       SEC_ADMINISTRATOR, Console::Yes },
            { "remove all", HandleDeserterBGRemoveAll, SEC_ADMINISTRATOR, Console::Yes },
            { "remove",     HandleDeserterBGRemove,    SEC_ADMINISTRATOR, Console::Yes }
        };

        static ChatCommandTable deserterCommandTable =
        {
            { "instance", deserterInstanceCommandTable },
            { "bg",       deserterBGCommandTable }
        };
        static ChatCommandTable commandTable =
        {
            { "deserter", deserterCommandTable }
        };
        return commandTable;
    }

    /**
    * @brief Applies the Deserter Debuff to a player
    *
    * This function applies a Deserter Debuff of the given type (Instance or BG) to the
    * selected player, with the provided duration in seconds.
    *
    * @param handler The ChatHandler, passed by the system.
    * @param playerName Player by name. Optional, defaults to selected or self.
    * @param time The provided duration as TimeString. Optional, defaults to bg/instance default time.
    * @param isInstance provided by the relaying functions, so we don't have
    * to write that much code :)
    *
    * @return true if everything was correct, false if an error occured.
    *
    * Example Usage:
    * @code
    * .deserter instance add 1h30m (using player target or self)
    * -or-
    * .deserter bg add 1h30m (using player target or self)
    * -or-
    * .deserter bg add Tester 1h30m (using player of name 'Tester')
    * @endcode
    */
    static bool HandleDeserterAdd(ChatHandler* handler, Optional<std::string> playerName, Optional<std::string_view> time, bool isInstance)
    {
        Player* target = handler->getSelectedPlayerOrSelf();
        ObjectGuid guid;

        if (playerName)
        {
            if (!normalizePlayerName(*playerName))
            {
                handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
                handler->SetSentErrorMessage(true);
                return false;
            }

            guid = sCharacterCache->GetCharacterGuidByName(*playerName);
            if (guid)
            {
                target = ObjectAccessor::FindPlayerByName(*playerName);
            }
            else
            {
                if (time)
                {
                    handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
                    handler->SetSentErrorMessage(true);
                    return false;
                }

                time = playerName;
                playerName = "";
            }
        }

        if (!playerName || playerName->empty())
        {
            if (!handler->GetSession())
            {
                return false;
            }

            playerName = target->GetName();
            guid = target->GetGUID();
        }

        if (!time)
        {
            time = isInstance ? "30m" : "15m";
        }

        Seconds duration = Warhead::Time::TimeStringTo(*time);

        if (duration == 0s)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 deserterSpell = isInstance ? LFG_SPELL_DUNGEON_DESERTER : BG_SPELL_DESERTER;

        if (target)
        {
            Aura* aura = target->GetAura(deserterSpell);
            if (aura && aura->GetDuration() >= duration * IN_MILLISECONDS)
            {
                handler->PSendSysMessage("Player {} already has a longer {} Deserter active.", handler->playerLink(*playerName), isInstance ? "Instance" : "Battleground");
                return true;
            }

            aura = target->AddAura(deserterSpell, target);
            if (!aura)
            {
                handler->SendSysMessage(LANG_BAD_VALUE);
                handler->SetSentErrorMessage(true);
                return false;
            }
            aura->SetDuration(duration.count() * IN_MILLISECONDS);
            return true;
        }
        else
        {
            int32 remainTime = 0;
            if (QueryResult result = CharacterDatabase.Query("SELECT remainTime FROM character_aura WHERE guid = {} AND spell = {}", guid.GetCounter(), deserterSpell))
            {
                Field* fields = result->Fetch();
                remainTime = fields[0].Get<int32>();

            if (remainTime < 0 || remainTime >= duration.count() * IN_MILLISECONDS)
            {
                handler->PSendSysMessage("Player {} already has a longer {} Deserter active.", handler->playerLink(*playerName), isInstance ? "Instance" : "Battleground");
                return true;
            }

            uint8 index = 0;
            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_AURA);
            stmt->SetData(index++, guid.GetCounter());
            stmt->SetData(index++, guid.GetCounter());
            stmt->SetData(index++, 0);
            stmt->SetData(index++, deserterSpell);
            stmt->SetData(index++, 1);
            stmt->SetData(index++, 1);
            stmt->SetData(index++, 1);
            stmt->SetData(index++, 0);
            stmt->SetData(index++, 0);
            stmt->SetData(index++, 0);
            stmt->SetData(index++, 0);
            stmt->SetData(index++, 0);
            stmt->SetData(index++, 0);
            stmt->SetData(index++, isInstance ? 1800000 : 900000);
            stmt->SetData(index++, duration * IN_MILLISECONDS);
            stmt->SetData(index, 0);
            CharacterDatabase.Execute(stmt);
        }

        handler->PSendSysMessage("%s of %s Deserter has been added to player %s.", secsToTimeString(duration), isInstance ? "Instance" : "Battleground", handler->playerLink(*playerName));
        return true;
    }

    /**
    * @brief Removes the Deserter Debuff from a player
    *
    * This function removes a Deserter Debuff of the given type (Instance or BG) from the
    * selected player.
    *
    * @param handler The ChatHandler, passed by the system.
    * @param player The target player, either by name, the target or self
    * @param isInstance provided by the relaying functions, so we don't have
    * to write that much code :)
    *
    * @return true if everything was correct, false if an error occured.
    *
    * Example Usage:
    * @code
    * .deserter instance remove (using player target or self)
    * -or-
    * .deserter bg remove (using player target or self)
    * -or-
    * .deserter bg remove Tester (using player of name 'Tester')
    * @endcode
    */
    static bool HandleDeserterRemove(ChatHandler* handler, Optional<PlayerIdentifier> player, bool isInstance)
    {
        if (!player)
        {
            player = PlayerIdentifier::FromTargetOrSelf(handler);
        }

        if (!player)
        {
            handler->SendSysMessage(LANG_NO_CHAR_SELECTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Player* target = player->GetConnectedPlayer();
        uint32 deserterSpell = isInstance ? LFG_SPELL_DUNGEON_DESERTER : BG_SPELL_DESERTER;
        int32 duration = 0;

        if (target)
        {
            if (Aura* aura = target->GetAura(deserterSpell))
            {
                duration = aura->GetDuration();
                target->RemoveAura(deserterSpell);
            }
        }
        else
        {
            if (QueryResult result = CharacterDatabase.Query("SELECT remainTime FROM character_aura WHERE guid = {} AND spell = {}", player->GetGUID().GetCounter(), deserterSpell))
            {
                Field* fields = result->Fetch();
                duration = fields[0].Get<int32>();
                CharacterDatabase.Execute("DELETE FROM character_aura WHERE guid = {} AND spell = {}", player->GetGUID().GetCounter(), deserterSpell);
            }
        }

        if (duration == 0)
        {
            handler->PSendSysMessage("Player %s does not have %s Deserter.", handler->playerLink(player->GetName()), isInstance ? "Instance" : "Battleground");
            handler->SetSentErrorMessage(true);
            return true;
        }

        if (duration < 0)
        {
            handler->PSendSysMessage("Permanent %s Deserter has been removed from player %s (GUID %u).", isInstance ? "Instance" : "Battleground", handler->playerLink(player->GetName()), player->GetGUID().GetCounter());
            handler->SetSentErrorMessage(true);
            return true;
        }

        handler->PSendSysMessage("%s of %s Deserter has been removed from player %s (GUID %u).", secsToTimeString(duration / IN_MILLISECONDS), isInstance ? "Instance" : "Battleground", handler->playerLink(player->GetName()), player->GetGUID().GetCounter());
        return true;
    }

    /**
    * @brief Removes the Deserter Debuff from all players
    *
    * This function removes a Deserter Debuff of the given type (Instance or BG) from
    * all players, online or offline.
    *
    * @param handler The ChatHandler, passed by the system.
    * @param isInstance provided by the relaying functions, so we don't have
    * to write that much code :)
    * @param maxTime Optional: The maximum remaining time of the Debuff on players to be removed.
    * Any Player with a Deserter Debuff of this time or less will get their Debuff removed. Use -1 for any.
    * Default: 15m for BG, 30m for Instance.
    *
    * @return true if everything was correct, false if an error occured.
    *
    * Example Usage:
    * @code
    * .deserter bg remove all
    * -or-
    * .deserter bg remove all 30m
    * -or-
    * .deserter bg remove all -1
    * @endcode
    */
    static bool HandleDeserterRemoveAll(ChatHandler* handler, bool isInstance, Optional<std::string_view> maxTime)
    {
        Seconds remainTime = isInstance ? 1800s : 900s;

        if (maxTime)
            remainTime = Warhead::Time::TimeStringTo(*maxTime);

        if (remainTime == 0s)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (remainTime < 0s)
            CharacterDatabase.Execute("DELETE FROM character_aura WHERE spell = {}", isInstance ? LFG_SPELL_DUNGEON_DESERTER : BG_SPELL_DESERTER);
        else
            CharacterDatabase.Execute("DELETE FROM character_aura WHERE spell = {} AND remainTime <= {}", isInstance ? LFG_SPELL_DUNGEON_DESERTER : BG_SPELL_DESERTER, remainTime.count());

        if (result)
        {
            deserterCount = (*result)[0].Get<uint64>();
        }

        // Optimization. Only execute these if there even is a result.
        if (deserterCount > 0)
        {
            countOnline = false;
            if (remainTime > 0)
            {
                CharacterDatabase.Execute("DELETE FROM character_aura WHERE spell = {} AND remainTime <= {}", deserterSpell, remainTime * IN_MILLISECONDS);
            }
            else
            {
                CharacterDatabase.Execute("DELETE FROM character_aura WHERE spell = {}", deserterSpell);
            }
        }

        std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());

        for (auto const& [guid, player] : ObjectAccessor::GetPlayers())
        {
            Aura* aura = player->GetAura(isInstance ? LFG_SPELL_DUNGEON_DESERTER : BG_SPELL_DESERTER);
            if (aura && (remainTime < 0s || Seconds(aura->GetDuration()) <= remainTime))
                player->RemoveAura(isInstance ? LFG_SPELL_DUNGEON_DESERTER : BG_SPELL_DESERTER);
        }

        handler->PSendSysMessage("{} Deserter has been removed from all players", isInstance ? "Instance" : "Battleground");
        return true;
    }

    /// @sa HandleDeserterAdd()
    static bool HandleDeserterInstanceAdd(ChatHandler* handler, Optional<std::string> playerName, Optional<std::string> time)
    {
        return HandleDeserterAdd(handler, playerName, time, true);
    }

    /// @sa HandleDeserterAdd()
    static bool HandleDeserterBGAdd(ChatHandler* handler, Optional<std::string> playerName, Optional<std::string> time)
    {
        return HandleDeserterAdd(handler, playerName, time, false);
    }

    /// @sa HandleDeserterRemove()
    static bool HandleDeserterInstanceRemove(ChatHandler* handler, Optional<PlayerIdentifier> player)
    {
        return HandleDeserterRemove(handler, player, true);
    }

    /// @sa HandleDeserterRemove()
    static bool HandleDeserterBGRemove(ChatHandler* handler, Optional<PlayerIdentifier> player)
    {
        return HandleDeserterRemove(handler, player, false);
    }

    static bool HandleDeserterInstanceRemoveAll(ChatHandler* handler, Optional<std::string> maxTime)
    {
        return HandleDeserterRemoveAll(handler, true, maxTime);
    }

    static bool HandleDeserterBGRemoveAll(ChatHandler* handler, Optional<std::string> maxTime)
    {
        return HandleDeserterRemoveAll(handler, false, maxTime);
    }
};

void AddSC_deserter_commandscript()
{
    new deserter_commandscript();
}
