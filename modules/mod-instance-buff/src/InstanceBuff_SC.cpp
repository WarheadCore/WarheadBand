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

#include "Log.h"
#include "ScriptMgr.h"
#include "ModulesConfig.h"
#include "Chat.h"
#include "Player.h"
#include "Group.h"
#include "Tokenize.h"
#include "StringConvert.h"
#include "Pet.h"
#include "Optional.h"

enum class InstanceDiff : uint8
{
    DungeonDefault,
    DungeonHeroic,
    Raid10ManDefault,
    Raid25ManDefault
};

class InstanceBuff
{
public:
    static InstanceBuff* instance()
    {
        static InstanceBuff instance;
        return &instance;
    }

    // Config load
    void LoadConfig()
    {
        _IsEnable = MOD_CONF_GET_BOOL("IB.Enable");

        if (!_IsEnable)
            return;

        _buffStore.clear();

        auto ConfigureBuffList = [this](std::string const& configOption, InstanceDiff diff)
        {
            auto const buffList = MOD_CONF_GET_STR(configOption);
            auto const& buffTokens = Warhead::Tokenize(buffList, ',', false);

            uint32 playerCount = 0;

            std::vector<std::pair<uint32 /*playerCount*/, uint32 /*spellID*/>> buffs;

            for (auto const& spellStr : buffTokens)
            {
                auto spellID = Warhead::StringTo<uint32>(spellStr);
                if (!spellID)
                {
                    LOG_ERROR("module", "> InstanceBuff: Incorrect spell id '{}' in config option '{}'. Module disabled.", spellStr, configOption);
                    _IsEnable = false;
                    break;
                }

                buffs.emplace_back(std::make_pair(++playerCount, *spellID));
            }

            _buffStore.emplace(diff, buffs);
        };

        ConfigureBuffList("IB.Dungeon.Default.BuffList", InstanceDiff::DungeonDefault);
        ConfigureBuffList("IB.Dungeon.Heroic.BuffList", InstanceDiff::DungeonHeroic);
        ConfigureBuffList("IB.Raid.10.BuffList", InstanceDiff::Raid10ManDefault);
        ConfigureBuffList("IB.Raid.25.BuffList", InstanceDiff::Raid25ManDefault);
    }

    // Manage system
    void ApplyBuffs(Player* player)
    {
        ClearBuffs(player, true);

        uint8 diff = player->GetMap()->GetDifficulty();
        bool isRaid = player->GetMap()->IsRaid();

        // Check if enable 5 default
        if (!isRaid && !diff && !MOD_CONF_GET_BOOL("IB.Dungeon.Default.Enable"))
            return;

        // Check if enable 5 heroic
        if (!isRaid && diff && !MOD_CONF_GET_BOOL("IB.Dungeon.Heroic.Enable"))
            return;

        // Check if enable 10 default
        if (isRaid && !diff && !MOD_CONF_GET_BOOL("IB.Raid.10.Enable"))
            return;

        // Check if enable 25 default
        if (isRaid && diff && !MOD_CONF_GET_BOOL("IB.Raid.25.Enable"))
            return;

        if (Group* group = player->GetGroup())
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* groupMember = itr->GetSource();
                if (!groupMember || !groupMember->IsInMap(player))
                    continue;

                ApplyBuffForPlayer(groupMember, diff, isRaid);
            }
        }
        else
            ApplyBuffForPlayer(player, diff, isRaid);
    }

    void ClearBuffs(Player* player, bool checkGroup)
    {
        Group* group = player->GetGroup();

        if (group && checkGroup)
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* groupMember = itr->GetSource();
                if (!groupMember)
                    continue;

                if (!IsPlayerBuffed(groupMember))
                    continue;

                ClearBuffForPlayer(groupMember);
            }
        }

        if (!IsPlayerBuffed(player))
            return;

        ClearBuffForPlayer(player);
    }

private:
    std::unordered_map<ObjectGuid, uint32 /*spellID*/> _playerBuffedStore;
    std::unordered_map<ObjectGuid/*owner guid*/, std::pair<ObjectGuid/*pet guid*/, uint32 /*spellID*/>> _petsBuffedStore;
    std::unordered_map<InstanceDiff, std::vector<std::pair<uint32 /*playerCount*/, uint32 /*spellID*/>>> _buffStore;

    bool IsPlayerBuffed(Player* player)
    {
        return _playerBuffedStore.find(player->GetGUID()) != _playerBuffedStore.end();
    }

    bool IsPlayerPetBuffed(Player* player)
    {
        return _petsBuffedStore.find(player->GetGUID()) != _petsBuffedStore.end();
    }

    uint32 GetPlayerCountInInstance(Player* player)
    {
        uint32 count = 0;

        if (Group* group = player->GetGroup())
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* groupMember = itr->GetSource();
                if (!groupMember)
                    continue;

                if (groupMember->IsInMap(player))
                    count++;
            }
        }

        return !count ? 1 : count;
    }

    uint32 GetSpellForDungeon(uint32 diff, bool isRaid, uint32 playerCount)
    {
        auto instanceDiff = GetInstanceDiff(diff, isRaid);
        if (!instanceDiff)
        {
            LOG_ERROR("module", "> InstanceBuff: Incorrect difficuly {}/{}", diff, isRaid);
            return 0;
        }

        auto const& itr = _buffStore.find(*instanceDiff);
        if (itr != _buffStore.end())
        {
            for (auto const& [_playerCount, spellID] : itr->second)
            {
                if (playerCount == _playerCount)
                    return spellID;
            }
        }

        return 0;
    }

    void ApplyBuffForPlayer(Player* player, uint8 diff, bool isRaid)
    {
        ClearBuffForPlayer(player);

        uint32 spellID = GetSpellForDungeon(diff, isRaid, GetPlayerCountInInstance(player));
        if (!spellID)
            return;

        player->CastSpell(player, spellID, true);
        _playerBuffedStore.emplace(player->GetGUID(), spellID);

        if (Pet* pet = player->GetPet())
        {
            pet->CastSpell(pet, spellID, true);
            _petsBuffedStore.emplace(player->GetGUID(), std::make_pair(pet->GetGUID(), spellID));
        }
    }

    void ClearBuffForPlayer(Player* player)
    {
        auto const& itrPlayer = _playerBuffedStore.find(player->GetGUID());
        if (itrPlayer != _playerBuffedStore.end())
        {
            player->RemoveAurasDueToSpell(itrPlayer->second);
            _playerBuffedStore.erase(player->GetGUID());
        }

        if (IsPlayerPetBuffed(player))
        {
            auto const& itrPet = _petsBuffedStore.find(player->GetGUID());
            if (itrPet != _petsBuffedStore.end())
            {
                auto const& [petGuid, spellID] = itrPet->second;
                Pet* pet = player->GetPet();

                if (pet && petGuid == pet->GetGUID())
                {
                    pet->RemoveAurasDueToSpell(itrPlayer->second);
                    _petsBuffedStore.erase(player->GetGUID());
                }
                else
                {
                    auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PET_AURAS);
                    stmt->SetArguments(petGuid.GetCounter());
                    CharacterDatabase.Execute(stmt);
                }
            }
        }
    }

    Optional<InstanceDiff> GetInstanceDiff(uint32 diff, bool isRaid)
    {
        if (!isRaid && !diff)
            return InstanceDiff::DungeonDefault;
        else if (!isRaid && diff)
            return InstanceDiff::DungeonHeroic;
        else if (isRaid && !diff)
            return InstanceDiff::Raid10ManDefault;
        else if (isRaid && diff)
            return InstanceDiff::Raid25ManDefault;

        return std::nullopt;
    }

    // Variables
    bool _IsEnable{ false };
};

#define sIB InstanceBuff::instance()

class InstanceBuff_Player : public PlayerScript
{
public:
    InstanceBuff_Player() : PlayerScript("InstanceBuff_Player") { }

    void OnMapChanged(Player* player) override
    {
        if (!MOD_CONF_GET_BOOL("IB.Enable"))
            return;

        if (!player)
            return;

        sIB->ClearBuffs(player, false);

        Map* map = player->GetMap();

        if (map->IsRaid() && map->IsHeroic())
            return;

        if (map->IsDungeon() || map->IsRaid())
            sIB->ApplyBuffs(player);
    }

    void OnAfterResurrect(Player* player, float /*restore_percent*/, bool /*applySickness*/) override
    {
        if (!MOD_CONF_GET_BOOL("IB.Enable"))
            return;

        if (!player)
            return;

        sIB->ClearBuffs(player, false);

        Map* map = player->GetMap();

        if (map->IsRaid() && map->IsHeroic())
            return;

        if (map->IsDungeon() || map->IsRaid())
            sIB->ApplyBuffs(player);
    }
};

class InstanceBuff_Pet : public PetScript
{
public:
    InstanceBuff_Pet() : PetScript("InstanceBuff_Pet") { }

    void OnPetAddToWorld(Pet* pet) override
    {
        if (!MOD_CONF_GET_BOOL("IB.Enable"))
            return;

        if (!pet)
            return;

        Player* player = pet->GetOwner();
        if (!player)
            return;

        sIB->ClearBuffs(player, false);

        Map* map = player->GetMap();

        if (map->IsRaid() && map->IsHeroic())
            return;

        if (map->IsDungeon() || map->IsRaid())
            sIB->ApplyBuffs(player);
    }
};

class InstanceBuff_World : public WorldScript
{
public:
    InstanceBuff_World() : WorldScript("InstanceBuff_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sIB->LoadConfig();
    }
};

// Group all custom scripts
void AddSC_InstanceBuff()
{
    new InstanceBuff_World();
    new InstanceBuff_Player();
    new InstanceBuff_Pet();
}
