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

#ifndef _INSTANCESAVEMGR_H
#define _INSTANCESAVEMGR_H

#include "DatabaseEnv.h"
#include "DBCEnums.h"
#include "Define.h"
#include "ObjectDefines.h"
#include <ace/Null_Mutex.h>
#include <ace/Thread_Mutex.h>
#include <list>
#include <map>
#include <unordered_map>

struct InstanceTemplate;
struct MapEntry;
class Player;
class Group;
class InstanceSaveManager;
class InstanceSave;

struct InstancePlayerBind
{
    InstanceSave* save{nullptr};
    bool perm : 1;
    bool extended : 1;
    InstancePlayerBind() :  perm(false), extended(false) {}
};

typedef std::unordered_map< uint32 /*mapId*/, InstancePlayerBind > BoundInstancesMap;

struct BoundInstancesMapWrapper
{
    BoundInstancesMap m[MAX_DIFFICULTY];
};

typedef std::unordered_map< uint32 /*guidLow*/, BoundInstancesMapWrapper* > PlayerBindStorage;

class WH_GAME_API InstanceSave
{
    friend class InstanceSaveManager;
public:
    InstanceSave(uint16 MapId, uint32 InstanceId, Difficulty difficulty, time_t resetTime, time_t extendedResetTime);
    ~InstanceSave();
    [[nodiscard]] uint32 GetInstanceId() const { return m_instanceid; }
    [[nodiscard]] uint32 GetMapId() const { return m_mapid; }
    [[nodiscard]] Difficulty GetDifficulty() const { return m_difficulty; }

    /* Saved when the instance is generated for the first time */
    void InsertToDB();
    // pussywizard: deleting is done internally when there are no binds left

    [[nodiscard]] std::string GetInstanceData() const { return m_instanceData; }
    void SetInstanceData(std::string str) { m_instanceData = str; }
    [[nodiscard]] uint32 GetCompletedEncounterMask() const { return m_completedEncounterMask; }
    void SetCompletedEncounterMask(uint32 mask) { m_completedEncounterMask = mask; }

    // pussywizard: for normal instances this corresponds to 0, for raid/heroic instances this caches the global reset time for the map
    [[nodiscard]] time_t GetResetTime() const { return m_resetTime; }
    [[nodiscard]] time_t GetExtendedResetTime() const { return m_extendedResetTime; }
    time_t GetResetTimeForDB();
    void SetResetTime(time_t resetTime) { m_resetTime = resetTime; }
    void SetExtendedResetTime(time_t extendedResetTime) { m_extendedResetTime = extendedResetTime; }

    [[nodiscard]] bool CanReset() const { return m_canReset; }
    void SetCanReset(bool canReset) { m_canReset = canReset; }

    InstanceTemplate const* GetTemplate();
    MapEntry const* GetMapEntry();

    void AddPlayer(uint32 guidLow);
    bool RemovePlayer(uint32 guidLow, InstanceSaveManager* ism);

    typedef std::list<uint32> PlayerListType;
private:
    PlayerListType m_playerList;
    time_t m_resetTime;
    time_t m_extendedResetTime;
    uint32 m_instanceid;
    uint32 m_mapid;
    Difficulty m_difficulty;
    bool m_canReset;
    std::string m_instanceData;
    uint32 m_completedEncounterMask;

    ACE_Thread_Mutex _lock;
};

typedef std::unordered_map<uint32 /*PAIR32(map, difficulty)*/, time_t /*resetTime*/> ResetTimeByMapDifficultyMap;

class WH_GAME_API InstanceSaveManager
{
    friend class InstanceSave;

private:
    InstanceSaveManager()  {};
    ~InstanceSaveManager();

public:
    static InstanceSaveManager* instance();

    typedef std::unordered_map<uint32 /*InstanceId*/, InstanceSave*> InstanceSaveHashMap;

    struct InstResetEvent
    {
        uint8 type{0}; // 0 - unused, 1-4 warnings about pending reset, 5 - reset
        Difficulty difficulty: 8;
        uint16 mapid{0};

        InstResetEvent() :  difficulty(DUNGEON_DIFFICULTY_NORMAL) {}
        InstResetEvent(uint8 t, uint32 _mapid, Difficulty d)
            : type(t), difficulty(d), mapid(_mapid) {}
    };
    typedef std::multimap<time_t /*resetTime*/, InstResetEvent> ResetTimeQueue;

    void LoadInstances();
    void LoadResetTimes();
    void LoadInstanceSaves();
    void LoadCharacterBinds();

    [[nodiscard]] time_t GetResetTimeFor(uint32 mapid, Difficulty d) const
    {
        ResetTimeByMapDifficultyMap::const_iterator itr  = m_resetTimeByMapDifficulty.find(MAKE_PAIR32(mapid, d));
        return itr != m_resetTimeByMapDifficulty.end() ? itr->second : 0;
    }

    [[nodiscard]] time_t GetExtendedResetTimeFor(uint32 mapid, Difficulty d) const
    {
        ResetTimeByMapDifficultyMap::const_iterator itr  = m_resetExtendedTimeByMapDifficulty.find(MAKE_PAIR32(mapid, d));
        return itr != m_resetExtendedTimeByMapDifficulty.end() ? itr->second : 0;
    }

    void SetResetTimeFor(uint32 mapid, Difficulty d, time_t t)
    {
        m_resetTimeByMapDifficulty[MAKE_PAIR32(mapid, d)] = t;
    }

    void SetExtendedResetTimeFor(uint32 mapid, Difficulty d, time_t t)
    {
        m_resetExtendedTimeByMapDifficulty[MAKE_PAIR32(mapid, d)] = t;
    }

    [[nodiscard]] ResetTimeByMapDifficultyMap const& GetResetTimeMap() const
    {
        return m_resetTimeByMapDifficulty;
    }

    void ScheduleReset(time_t time, InstResetEvent event);

    void Update();

    InstanceSave* AddInstanceSave(uint32 mapId, uint32 instanceId, Difficulty difficulty, bool startup = false);
    bool DeleteInstanceSaveIfNeeded(uint32 InstanceId, bool skipMapCheck);
    bool DeleteInstanceSaveIfNeeded(InstanceSave* save, bool skipMapCheck);

    InstanceSave* GetInstanceSave(uint32 InstanceId);

    InstancePlayerBind* PlayerBindToInstance(uint32 guidLow, InstanceSave* save, bool permanent, Player* player = nullptr);
    void PlayerUnbindInstance(uint32 guidLow, uint32 mapid, Difficulty difficulty, bool deleteFromDB, Player* player = nullptr);
    void PlayerUnbindInstanceNotExtended(uint32 guidLow, uint32 mapid, Difficulty difficulty, Player* player = nullptr);
    InstancePlayerBind* PlayerGetBoundInstance(uint32 guidLow, uint32 mapid, Difficulty difficulty);
    bool PlayerIsPermBoundToInstance(uint32 guidLow, uint32 mapid, Difficulty difficulty);
    BoundInstancesMap const& PlayerGetBoundInstances(uint32 guidLow, Difficulty difficulty);
    void PlayerCreateBoundInstancesMaps(uint32 guidLow);
    InstanceSave* PlayerGetInstanceSave(uint32 guidLow, uint32 mapid, Difficulty difficulty);
    uint32 PlayerGetDestinationInstanceId(Player* player, uint32 mapid, Difficulty difficulty);
    void CopyBinds(uint32 from, uint32 to, Player* toPlr = nullptr);
    void UnbindAllFor(InstanceSave* save);

protected:
    static uint16 ResetTimeDelay[];
    static PlayerBindStorage playerBindStorage;
    static BoundInstancesMap emptyBoundInstancesMap;

private:
    void _ResetOrWarnAll(uint32 mapid, Difficulty difficulty, bool warn, time_t resetTime);
    void _ResetSave(InstanceSaveHashMap::iterator& itr);
    bool lock_instLists{false};
    InstanceSaveHashMap m_instanceSaveById;
    ResetTimeByMapDifficultyMap m_resetTimeByMapDifficulty;
    ResetTimeByMapDifficultyMap m_resetExtendedTimeByMapDifficulty;
    ResetTimeQueue m_resetTimeQueue;
};

#define sInstanceSaveMgr InstanceSaveManager::instance()

#endif
