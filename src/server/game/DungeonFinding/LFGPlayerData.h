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

#ifndef _LFGPLAYERDATA_H
#define _LFGPLAYERDATA_H

#include "LFG.h"

namespace lfg
{
    /**
        Stores all lfg data needed about the player.
    */

    class WH_GAME_API LfgPlayerData
    {
    public:
        LfgPlayerData();
        ~LfgPlayerData();

        // General
        void SetState(LfgState state);
        void RestoreState();
        void SetLockedDungeons(LfgLockMap const& lock);
        void SetTeam(TeamId teamId);
        void SetGroup(ObjectGuid group);
        void SetRandomPlayersCount(uint8 count);

        // Queue
        void SetRoles(uint8 roles);
        void SetComment(std::string const& comment);
        void SetSelectedDungeons(const LfgDungeonSet& dungeons);

        // General
        LfgState GetState() const;
        LfgState GetOldState() const;
        LfgLockMap const& GetLockedDungeons() const;
        TeamId GetTeam() const;
        ObjectGuid GetGroup() const;
        uint8 GetRandomPlayersCount() const;
        void SetCanOverrideRBState(bool val) { m_canOverrideRBState = val; }
        bool CanOverrideRBState() const { return m_canOverrideRBState; }

        // Queue
        uint8 GetRoles() const;
        std::string const& GetComment() const;
        LfgDungeonSet const& GetSelectedDungeons() const;

    private:
        // General
        LfgState m_State;                                  ///< State if group in LFG
        LfgState m_OldState;                               ///< Old State - Used to restore state after failed Rolecheck/Proposal
        bool m_canOverrideRBState;                         ///< pussywizard
        // Player
        LfgLockMap m_LockedDungeons;                       ///< Dungeons player can't do and reason
        TeamId m_TeamId;                                   ///< Player team - determines the queue to join
        ObjectGuid m_Group;                                ///< Original group of player when joined LFG
        uint8 m_randomPlayers;                             ///< Xinef: Amount of random players you raid with

        // Queue
        uint8 m_Roles;                                     ///< Roles the player selected when joined LFG
        std::string m_Comment;                             ///< Player comment used when joined LFG
        LfgDungeonSet m_SelectedDungeons;                  ///< Selected Dungeons when joined LFG
    };

} // namespace lfg

#endif
