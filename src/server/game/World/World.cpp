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

/** \file
    \ingroup world
*/

#include "WorldSession.h"
#include "AccountMgr.h"
#include "AchievementMgr.h"
#include "AddonMgr.h"
#include "ArenaTeamMgr.h"
#include "AsyncAuctionListing.h"
#include "AuctionHouseMgr.h"
#include "BattlefieldMgr.h"
#include "BattlegroundMgr.h"
#include "CalendarMgr.h"
#include "CellImpl.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "CharacterDatabaseCleaner.h"
#include "Chat.h"
#include "Common.h"
#include "ConditionMgr.h"
#include "Config.h"
#include "CreatureAIRegistry.h"
#include "CreatureGroups.h"
#include "CreatureTextMgr.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "DisableMgr.h"
#include "DynamicVisibility.h"
#include "GameConfig.h"
#include "GameEventMgr.h"
#include "GameGraveyard.h"
#include "GameLocale.h"
#include "GameTime.h"
#include "GitRevision.h"
#include "GridNotifiersImpl.h"
#include "GroupMgr.h"
#include "GuildMgr.h"
#include "IPLocation.h"
#include "InstanceSaveMgr.h"
#include "ItemEnchantmentMgr.h"
#include "LFGMgr.h"
#include "Language.h"
#include "LocaleCommon.h"
#include "Log.h"
#include "LootItemStorage.h"
#include "LootMgr.h"
#include "MMapFactory.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "OutdoorPvPMgr.h"
#include "PetitionMgr.h"
#include "Player.h"
#include "PoolMgr.h"
#include "Realm.h"
#include "SavingSystem.h"
#include "ScriptMgr.h"
#include "ServerMotd.h"
#include "SkillDiscovery.h"
#include "SkillExtraItems.h"
#include "SmartAI.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
#include "TicketMgr.h"
#include "Transport.h"
#include "TransportMgr.h"
#include "UpdateTime.h"
#include "Util.h"
#include "VMapFactory.h"
#include "VMapManager2.h"
#include "Vehicle.h"
#include "Warden.h"
#include "WardenCheckMgr.h"
#include "WaypointMovementGenerator.h"
#include "WeatherMgr.h"
#include "WhoListCache.h"
#include "World.h"
#include "WorldPacket.h"
#include <boost/asio/ip/address.hpp>
#include <cmath>

#ifdef ELUNA
#include "LuaEngine.h"
#endif

std::atomic_long World::m_stopEvent = false;
uint8 World::m_ExitCode = SHUTDOWN_EXIT_CODE;
uint32 World::m_worldLoopCounter = 0;

float World::m_MaxVisibleDistanceOnContinents = DEFAULT_VISIBILITY_DISTANCE;
float World::m_MaxVisibleDistanceInInstances  = DEFAULT_VISIBILITY_INSTANCE;
float World::m_MaxVisibleDistanceInBGArenas   = DEFAULT_VISIBILITY_BGARENAS;

Realm realm;

/// World constructor
World::World()
{
    m_playerLimit = 0;
    m_allowedSecurityLevel = SEC_PLAYER;
    m_allowMovement = true;
    m_ShutdownMask = 0;
    m_ShutdownTimer = 0;
    m_maxActiveSessionCount = 0;
    m_maxQueuedSessionCount = 0;
    m_PlayerCount = 0;
    m_MaxPlayerCount = 0;
    m_NextDailyQuestReset = 0;
    m_NextWeeklyQuestReset = 0;
    m_NextMonthlyQuestReset = 0;
    m_NextRandomBGReset = 0;
    m_NextCalendarOldEventsDeletionTime = 0;
    m_NextGuildReset = 0;
    m_defaultDbcLocale = LOCALE_enUS;
    mail_expire_check_timer = 0;
    m_isClosed = false;
    m_CleaningFlags = 0;
}

/// World destructor
World::~World()
{
    ///- Empty the kicked session set
    while (!m_sessions.empty())
    {
        // not remove from queue, prevent loading new sessions
        delete m_sessions.begin()->second;
        m_sessions.erase(m_sessions.begin());
    }

    while (!m_offlineSessions.empty())
    {
        delete m_offlineSessions.begin()->second;
        m_offlineSessions.erase(m_offlineSessions.begin());
    }

    CliCommandHolder* command = nullptr;
    while (cliCmdQueue.next(command))
        delete command;

    VMAP::VMapFactory::clear();
    MMAP::MMapFactory::clear();

    //TODO free addSessQueue
}

/*static*/ World* World::instance()
{
    static World instance;
    return &instance;
}

/// Find a player in a specified zone
Player* World::FindPlayerInZone(uint32 zone)
{
    ///- circle through active sessions and return the first player found in the zone
    SessionMap::const_iterator itr;
    for (itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (!itr->second)
            continue;

        Player* player = itr->second->GetPlayer();
        if (!player)
            continue;

        if (player->IsInWorld() && player->GetZoneId() == zone)
            return player;
    }
    return nullptr;
}

bool World::IsClosed() const
{
    return m_isClosed;
}

void World::SetClosed(bool val)
{
    m_isClosed = val;

    // Invert the value, for simplicity for scripters.
    sScriptMgr->OnOpenStateChange(!val);
}

/// Find a session by its id
WorldSession* World::FindSession(uint32 id) const
{
    SessionMap::const_iterator itr = m_sessions.find(id);

    if (itr != m_sessions.end())
        return itr->second;                                 // also can return nullptr for kicked session
    else
        return nullptr;
}

WorldSession* World::FindOfflineSession(uint32 id) const
{
    SessionMap::const_iterator itr = m_offlineSessions.find(id);
    if (itr != m_offlineSessions.end())
        return itr->second;
    else
        return nullptr;
}

WorldSession* World::FindOfflineSessionForCharacterGUID(ObjectGuid::LowType guidLow) const
{
    if (m_offlineSessions.empty())
        return nullptr;

    for (SessionMap::const_iterator itr = m_offlineSessions.begin(); itr != m_offlineSessions.end(); ++itr)
        if (itr->second->GetGuidLow() == guidLow)
            return itr->second;

    return nullptr;
}

/// Remove a given session
bool World::KickSession(uint32 id)
{
    ///- Find the session, kick the user, but we can't delete session at this moment to prevent iterator invalidation
    SessionMap::const_iterator itr = m_sessions.find(id);

    if (itr != m_sessions.end() && itr->second)
    {
        if (itr->second->PlayerLoading())
            return false;

        itr->second->KickPlayer("KickSession", false);
    }

    return true;
}

void World::AddSession(WorldSession* s)
{
    addSessQueue.add(s);
}

void World::AddSession_(WorldSession* s)
{
    ASSERT (s);

    // kick existing session with same account (if any)
    // if character on old session is being loaded, then return
    if (!KickSession(s->GetAccountId()))
    {
        s->KickPlayer("kick existing session with same account");
        delete s; // session not added yet in session list, so not listed in queue
        return;
    }

    SessionMap::const_iterator old = m_sessions.find(s->GetAccountId());
    if (old != m_sessions.end())
    {
        WorldSession* oldSession = old->second;

        if (!RemoveQueuedPlayer(oldSession) && CONF_GET_INT("DisconnectToleranceInterval"))
            m_disconnects[s->GetAccountId()] = GameTime::GetGameTime();

        // pussywizard:
        if (oldSession->HandleSocketClosed())
        {
            // there should be no offline session if current one is logged onto a character
            SessionMap::iterator iter;
            if ((iter = m_offlineSessions.find(oldSession->GetAccountId())) != m_offlineSessions.end())
            {
                WorldSession* tmp = iter->second;
                m_offlineSessions.erase(iter);
                tmp->SetShouldSetOfflineInDB(false);
                delete tmp;
            }
            oldSession->SetOfflineTime(GameTime::GetGameTime());
            m_offlineSessions[oldSession->GetAccountId()] = oldSession;
        }
        else
        {
            oldSession->SetShouldSetOfflineInDB(false); // pussywizard: don't set offline in db because new session for that acc is already created
            delete oldSession;
        }
    }

    m_sessions[s->GetAccountId()] = s;

    uint32 Sessions = GetActiveAndQueuedSessionCount();
    uint32 pLimit = GetPlayerAmountLimit();

    // don't count this session when checking player limit
    --Sessions;

    if (pLimit > 0 && Sessions >= pLimit && AccountMgr::IsPlayerAccount(s->GetSecurity()) && !s->CanSkipQueue() && !HasRecentlyDisconnected(s))
    {
        AddQueuedPlayer (s);
        UpdateMaxSessionCounters();
        return;
    }

    s->SendAuthResponse(AUTH_OK, true);
    s->SendAddonsInfo();
    s->SendClientCacheVersion(CONF_GET_INT("ClientCacheVersion"));
    s->SendTutorialsData();

    UpdateMaxSessionCounters();
}

bool World::HasRecentlyDisconnected(WorldSession* session)
{
    if (!session)
        return false;

    if (uint32 tolerance = CONF_GET_INT("DisconnectToleranceInterval"))
    {
        for (DisconnectMap::iterator i = m_disconnects.begin(); i != m_disconnects.end();)
        {
            if ((GameTime::GetGameTime() - i->second) < tolerance)
            {
                if (i->first == session->GetAccountId())
                    return true;
                ++i;
            }
            else
                m_disconnects.erase(i++);
        }
    }
    return false;
}

int32 World::GetQueuePos(WorldSession* sess)
{
    uint32 position = 1;

    for (Queue::const_iterator iter = m_QueuedPlayer.begin(); iter != m_QueuedPlayer.end(); ++iter, ++position)
        if ((*iter) == sess)
            return position;

    return 0;
}

void World::AddQueuedPlayer(WorldSession* sess)
{
    sess->SetInQueue(true);
    m_QueuedPlayer.push_back(sess);

    // The 1st SMSG_AUTH_RESPONSE needs to contain other info too.
    sess->SendAuthResponse(AUTH_WAIT_QUEUE, false, GetQueuePos(sess));
}

bool World::RemoveQueuedPlayer(WorldSession* sess)
{
    uint32 sessions = GetActiveSessionCount();

    uint32 position = 1;
    Queue::iterator iter = m_QueuedPlayer.begin();

    // search to remove and count skipped positions
    bool found = false;

    for (; iter != m_QueuedPlayer.end(); ++iter, ++position)
    {
        if (*iter == sess)
        {
            sess->SetInQueue(false);
            sess->ResetTimeOutTime(false);
            iter = m_QueuedPlayer.erase(iter);
            found = true;
            break;
        }
    }

    // if session not queued then it was an active session
    if (!found)
    {
        ASSERT(sessions > 0);
        --sessions;
    }

    // accept first in queue
    if ((!GetPlayerAmountLimit() || sessions < GetPlayerAmountLimit()) && !m_QueuedPlayer.empty())
    {
        WorldSession* pop_sess = m_QueuedPlayer.front();
        pop_sess->SetInQueue(false);
        pop_sess->ResetTimeOutTime(false);
        pop_sess->SendAuthWaitQue(0);
        pop_sess->SendAddonsInfo();

        pop_sess->SendClientCacheVersion(CONF_GET_INT("ClientCacheVersion"));
        pop_sess->SendAccountDataTimes(GLOBAL_CACHE_MASK);
        pop_sess->SendTutorialsData();

        m_QueuedPlayer.pop_front();

        // update iter to point first queued socket or end() if queue is empty now
        iter = m_QueuedPlayer.begin();
        position = 1;
    }

    // update queue position from iter to end()
    for (; iter != m_QueuedPlayer.end(); ++iter, ++position)
        (*iter)->SendAuthWaitQue(position);

    return found;
}

/// Initialize config values
void World::LoadConfigSettings(bool reload)
{
    if (reload)
    {
        if (!sConfigMgr->Reload())
        {
            LOG_ERROR("server", "World settings reload fail: can't read settings.");
            return;
        }

        sLog->LoadFromConfig();
    }

#ifdef ELUNA
    ///- Initialize Lua Engine
    if (!reload)
    {
        LOG_INFO("server", "Initialize Eluna Lua Engine...");
        Eluna::Initialize();
    }
#endif

    sScriptMgr->OnBeforeConfigLoad(reload);

    sGameConfig->Load(reload);

    // load update time related configs
    sWorldUpdateTime.LoadFromConfig();

    if (reload)
    {
        m_timers[WUPDATE_UPTIME].SetInterval(CONF_GET_INT("UpdateUptimeInterval") * MINUTE * IN_MILLISECONDS);
        m_timers[WUPDATE_UPTIME].Reset();
    }

    auto rateAggro = CONF_GET_FLOAT("Rate.Creature.Aggro");

    // Visibility on continents
    m_MaxVisibleDistanceOnContinents = CONF_GET_FLOAT("Visibility.Distance.Continents");
    if (m_MaxVisibleDistanceOnContinents < 45 * rateAggro)
    {
        LOG_ERROR("server.loading", "Visibility.Distance.Continents can't be less max aggro radius %f", 45 * rateAggro);
        m_MaxVisibleDistanceOnContinents = 45 * rateAggro;
    }
    else if (m_MaxVisibleDistanceOnContinents > MAX_VISIBILITY_DISTANCE)
    {
        LOG_ERROR("server.loading", "Visibility.Distance.Continents can't be greater %f", MAX_VISIBILITY_DISTANCE);
        m_MaxVisibleDistanceOnContinents = MAX_VISIBILITY_DISTANCE;
    }

    // Visibility in instances
    m_MaxVisibleDistanceInInstances = CONF_GET_FLOAT("Visibility.Distance.Instances");
    if (m_MaxVisibleDistanceInInstances < 45 * rateAggro)
    {
        LOG_ERROR("server.loading", "Visibility.Distance.Instances can't be less max aggro radius %f", 45 * rateAggro);
        m_MaxVisibleDistanceInInstances = 45 * rateAggro;
    }
    else if (m_MaxVisibleDistanceInInstances > MAX_VISIBILITY_DISTANCE)
    {
        LOG_ERROR("server.loading", "Visibility.Distance.Instances can't be greater %f", MAX_VISIBILITY_DISTANCE);
        m_MaxVisibleDistanceInInstances = MAX_VISIBILITY_DISTANCE;
    }

    // Visibility in BG/Arenas
    m_MaxVisibleDistanceInBGArenas = CONF_GET_FLOAT("Visibility.Distance.BGArenas");
    if (m_MaxVisibleDistanceInBGArenas < 45 * rateAggro)
    {
        LOG_ERROR("server.loading", "Visibility.Distance.BGArenas can't be less max aggro radius %f", 45 * rateAggro);
        m_MaxVisibleDistanceInBGArenas = 45 * rateAggro;
    }
    else if (m_MaxVisibleDistanceInBGArenas > MAX_VISIBILITY_DISTANCE)
    {
        LOG_ERROR("server.loading", "Visibility.Distance.BGArenas can't be greater %f", MAX_VISIBILITY_DISTANCE);
        m_MaxVisibleDistanceInBGArenas = MAX_VISIBILITY_DISTANCE;
    }

    ///- Read the "Data" directory from the config file
    std::string dataPath = sConfigMgr->GetOption<std::string>("DataDir", "./");
    if (dataPath.empty() || (dataPath.at(dataPath.length() - 1) != '/' && dataPath.at(dataPath.length() - 1) != '\\'))
        dataPath.push_back('/');

#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_UNIX || WARHEAD_PLATFORM == WARHEAD_PLATFORM_APPLE
    if (dataPath[0] == '~')
    {
        const char* home = getenv("HOME");
        if (home)
            dataPath.replace(0, 1, home);
    }
#endif

    if (reload && (dataPath != m_dataPath))
        LOG_ERROR("server.loading", "DataDir option can't be changed at worldserver.conf reload, using current value (%s).", m_dataPath.c_str());
    else
        m_dataPath = dataPath;

    bool enableIndoor = CONF_GET_BOOL("vmap.enableIndoorCheck");
    bool enableLOS = CONF_GET_BOOL("vmap.enableLOS");
    bool enableHeight = CONF_GET_BOOL("vmap.enableHeight");
    bool enablePetLOS = CONF_GET_BOOL("vmap.petLOS");

    if (!enableHeight)
        LOG_ERROR("server", "VMap height checking disabled! Creatures movements and other various things WILL be broken! Expect no support.");

    VMAP::VMapFactory::createOrGetVMapManager()->setEnableLineOfSightCalc(enableLOS);
    VMAP::VMapFactory::createOrGetVMapManager()->setEnableHeightCalc(enableHeight);

    if (!reload)
    {
        auto VMAPBoolToString = [](bool value)
        {
            return value ? "Enable" : "Disable";
        };

        LOG_INFO("server.loading", "Loading data configurations...");
        LOG_INFO("server.loading", "> Using DataDir:        %s", m_dataPath.c_str());
        LOG_INFO("server.loading", "");
        LOG_INFO("server.loading", "Loading VMap configurations...");
        LOG_INFO("server.loading", "> Line Of Sight:        %s", VMAPBoolToString(enableLOS));
        LOG_INFO("server.loading", "> Get Height:           %s", VMAPBoolToString(enableHeight));
        LOG_INFO("server.loading", "> Indoor Check:         %s", VMAPBoolToString(enableIndoor));
        LOG_INFO("server.loading", "> Pet LOS:              %s", VMAPBoolToString(enablePetLOS));
    }

    if (reload)
    {
        m_timers[WUPDATE_AUTOBROADCAST].SetInterval(CONF_GET_INT("AutoBroadcast.Timer"));
        m_timers[WUPDATE_AUTOBROADCAST].Reset();
    }

    MMAP::MMapFactory::InitializeDisabledMaps();

    // call ScriptMgr if we're reloading the configuration
    sScriptMgr->OnAfterConfigLoad(reload);
}

/// Initialize the World
void World::SetInitialWorldSettings()
{
    ///- Server startup begin
    uint32 startupBegin = getMSTime();

    ///- Initialize the random number generator
    srand((unsigned int)GameTime::GetGameTime());

    ///- Initialize detour memory management
    dtAllocSetCustom(dtCustomAlloc, dtCustomFree);

    LOG_INFO("server", "Initializing Scripts...");
    sScriptMgr->Initialize();

    ///- Initialize VMapManager function pointers (to untangle game/collision circular deps)
    VMAP::VMapManager2* vmmgr2 = VMAP::VMapFactory::createOrGetVMapManager();
    vmmgr2->GetLiquidFlagsPtr = &GetLiquidFlags;
    vmmgr2->IsVMAPDisabledForPtr = &DisableMgr::IsVMAPDisabledFor;

    ///- Initialize config settings
    LoadConfigSettings();

    ///- Initialize Allowed Security Level
    LoadDBAllowedSecurityLevel();

    ///- Init highest guids before any table loading to prevent using not initialized guids in some code.
    sObjectMgr->SetHighestGuids();

    if (!sConfigMgr->isDryRun())
    {
        ///- Check the existence of the map files for all starting areas.
        if (!MapManager::ExistMapAndVMap(0, -6240.32f, 331.033f)
                || !MapManager::ExistMapAndVMap(0, -8949.95f, -132.493f)
                || !MapManager::ExistMapAndVMap(1, -618.518f, -4251.67f)
                || !MapManager::ExistMapAndVMap(0, 1676.35f, 1677.45f)
                || !MapManager::ExistMapAndVMap(1, 10311.3f, 832.463f)
                || !MapManager::ExistMapAndVMap(1, -2917.58f, -257.98f)
                || (CONF_GET_INT("Expansion") && (
                        !MapManager::ExistMapAndVMap(530, 10349.6f, -6357.29f) ||
                        !MapManager::ExistMapAndVMap(530, -3961.64f, -13931.2f))))
        {
            exit(1);
        }
    }

    ///- Initialize pool manager
    sPoolMgr->Initialize();

    ///- Initialize game event manager
    sGameEventMgr->Initialize();

    ///- Loading strings. Getting no records means core load has to be canceled because no error message can be output.
    LOG_INFO("server", " ");
    LOG_INFO("server", "Loading Warhead strings...");
    if (!sGameLocale->LoadWarheadStrings())
        exit(1);                                            // Error message displayed in function already

    ///- Update the realm entry in the database with the realm type from the config file
    //No SQL injection as values are treated as integers

    // not send custom type REALM_FFA_PVP to realm list
    uint32 server_type;
    if (IsFFAPvPRealm())
        server_type = REALM_TYPE_PVP;
    else
        server_type = CONF_GET_INT("GameType");

    uint32 realm_zone = CONF_GET_INT("RealmZone");

    LoginDatabase.PExecute("UPDATE realmlist SET icon = %u, timezone = %u WHERE id = '%d'", server_type, realm_zone, realm.Id.Realm);      // One-time query

    ///- Custom Hook for loading DB items
    sScriptMgr->OnLoadCustomDatabaseTable();

    ///- Load the DBC files
    LOG_INFO("server", "Initialize data stores...");
    LoadDBCStores(m_dataPath);
    DetectDBCLang();

    // Load IP Location Database
    sIPLocation->Load();

    std::vector<uint32> mapIds;
    for (auto const& map : sMapStore)
        mapIds.emplace_back(map->MapID);

    vmmgr2->InitializeThreadUnsafe(mapIds);

    MMAP::MMapManager* mmmgr = MMAP::MMapFactory::createOrGetMMapManager();
    mmmgr->InitializeThreadUnsafe(mapIds);

    LOG_INFO("server", "Loading Game Graveyard...");
    sGraveyard->LoadGraveyardFromDB();

    LOG_INFO("server", "Loading spell dbc data corrections...");
    sSpellMgr->LoadDbcDataCorrections();

    LOG_INFO("server", "Loading SpellInfo store...");
    sSpellMgr->LoadSpellInfoStore();

    LOG_INFO("server", "Loading Spell Rank Data...");
    sSpellMgr->LoadSpellRanks();

    LOG_INFO("server", "Loading Spell Specific And Aura State...");
    sSpellMgr->LoadSpellSpecificAndAuraState();

    LOG_INFO("server", "Loading SkillLineAbilityMultiMap Data...");
    sSpellMgr->LoadSkillLineAbilityMap();

    LOG_INFO("server", "Loading spell custom attributes...");
    sSpellMgr->LoadSpellCustomAttr();

    LOG_INFO("server", "Loading GameObject models...");
    LoadGameObjectModelList(m_dataPath);

    LOG_INFO("server", "Loading Script Names...");
    sObjectMgr->LoadScriptNames();

    LOG_INFO("server", "Loading Instance Template...");
    sObjectMgr->LoadInstanceTemplate();

    // xinef: Global Storage, should be loaded asap
    LOG_INFO("server", "Load Global Player Data...");
    sWorld->LoadGlobalPlayerDataStore();

    // Must be called before `creature_respawn`/`gameobject_respawn` tables
    LOG_INFO("server", "Loading instances...");
    sInstanceSaveMgr->LoadInstances();

    LOG_INFO("server.loading", "Loading Game locale texts...");
    sGameLocale->LoadAllLocales();

    LOG_INFO("server", "Loading Page Texts...");
    sObjectMgr->LoadPageTexts();

    LOG_INFO("server", "Loading Game Object Templates...");         // must be after LoadPageTexts
    sObjectMgr->LoadGameObjectTemplate();

    LOG_INFO("server", "Loading Game Object template addons...");
    sObjectMgr->LoadGameObjectTemplateAddons();

    LOG_INFO("server", "Loading Transport templates...");
    sTransportMgr->LoadTransportTemplates();

    LOG_INFO("server", "Loading Spell Required Data...");
    sSpellMgr->LoadSpellRequired();

    LOG_INFO("server", "Loading Spell Group types...");
    sSpellMgr->LoadSpellGroups();

    LOG_INFO("server", "Loading Spell Learn Skills...");
    sSpellMgr->LoadSpellLearnSkills();                           // must be after LoadSpellRanks

    LOG_INFO("server", "Loading Spell Proc Event conditions...");
    sSpellMgr->LoadSpellProcEvents();

    LOG_INFO("server", "Loading Spell Proc conditions and data...");
    sSpellMgr->LoadSpellProcs();

    LOG_INFO("server", "Loading Spell Bonus Data...");
    sSpellMgr->LoadSpellBonusess();

    LOG_INFO("server", "Loading Aggro Spells Definitions...");
    sSpellMgr->LoadSpellThreats();

    LOG_INFO("server", "Loading Mixology bonuses...");
    sSpellMgr->LoadSpellMixology();

    LOG_INFO("server", "Loading Spell Group Stack Rules...");
    sSpellMgr->LoadSpellGroupStackRules();

    LOG_INFO("server", "Loading NPC Texts...");
    sObjectMgr->LoadGossipText();

    LOG_INFO("server", "Loading Enchant Spells Proc datas...");
    sSpellMgr->LoadSpellEnchantProcData();

    LOG_INFO("server", "Loading Item Random Enchantments Table...");
    LoadRandomEnchantmentsTable();

    LOG_INFO("server", "Loading Disables");
    DisableMgr::LoadDisables();                                  // must be before loading quests and items

    LOG_INFO("server", "Loading Items...");                         // must be after LoadRandomEnchantmentsTable and LoadPageTexts
    sObjectMgr->LoadItemTemplates();

    LOG_INFO("server", "Loading Item set names...");                // must be after LoadItemPrototypes
    sObjectMgr->LoadItemSetNames();

    LOG_INFO("server", "Loading Creature Model Based Info Data...");
    sObjectMgr->LoadCreatureModelInfo();

    LOG_INFO("server", "Loading Creature templates...");
    sObjectMgr->LoadCreatureTemplates();

    LOG_INFO("server", "Loading Equipment templates...");           // must be after LoadCreatureTemplates
    sObjectMgr->LoadEquipmentTemplates();

    LOG_INFO("server", "Loading Creature template addons...");
    sObjectMgr->LoadCreatureTemplateAddons();

    LOG_INFO("server", "Loading Reputation Reward Rates...");
    sObjectMgr->LoadReputationRewardRate();

    LOG_INFO("server", "Loading Creature Reputation OnKill Data...");
    sObjectMgr->LoadReputationOnKill();

    LOG_INFO("server", "Loading Reputation Spillover Data..." );
    sObjectMgr->LoadReputationSpilloverTemplate();

    LOG_INFO("server", "Loading Points Of Interest Data...");
    sObjectMgr->LoadPointsOfInterest();

    LOG_INFO("server", "Loading Creature Base Stats...");
    sObjectMgr->LoadCreatureClassLevelStats();

    LOG_INFO("server", "Loading Creature Data...");
    sObjectMgr->LoadCreatures();

    LOG_INFO("server", "Loading Temporary Summon Data...");
    sObjectMgr->LoadTempSummons();                               // must be after LoadCreatureTemplates() and LoadGameObjectTemplates()

    LOG_INFO("server", "Loading pet levelup spells...");
    sSpellMgr->LoadPetLevelupSpellMap();

    LOG_INFO("server", "Loading pet default spells additional to levelup spells...");
    sSpellMgr->LoadPetDefaultSpells();

    LOG_INFO("server", "Loading Creature Addon Data...");
    sObjectMgr->LoadCreatureAddons();                            // must be after LoadCreatureTemplates() and LoadCreatures()

    LOG_INFO("server", "Loading Gameobject Data...");
    sObjectMgr->LoadGameobjects();

    LOG_INFO("server", "Loading GameObject Addon Data...");
    sObjectMgr->LoadGameObjectAddons();                          // must be after LoadGameObjectTemplate() and LoadGameobjects()

    LOG_INFO("server", "Loading GameObject Quest Items...");
    sObjectMgr->LoadGameObjectQuestItems();

    LOG_INFO("server", "Loading Creature Quest Items...");
    sObjectMgr->LoadCreatureQuestItems();

    LOG_INFO("server", "Loading Creature Linked Respawn...");
    sObjectMgr->LoadLinkedRespawn();                             // must be after LoadCreatures(), LoadGameObjects()

    LOG_INFO("server", "Loading Weather Data...");
    WeatherMgr::LoadWeatherData();

    LOG_INFO("server", "Loading Quests...");
    sObjectMgr->LoadQuests();                                    // must be loaded after DBCs, creature_template, item_template, gameobject tables

    LOG_INFO("server", "Checking Quest Disables");
    DisableMgr::CheckQuestDisables();                           // must be after loading quests

    LOG_INFO("server", "Loading Quest POI");
    sObjectMgr->LoadQuestPOI();

    LOG_INFO("server", "Loading Quests Starters and Enders...");
    sObjectMgr->LoadQuestStartersAndEnders();                    // must be after quest load

    LOG_INFO("server", "Loading Objects Pooling Data...");
    sPoolMgr->LoadFromDB();

    LOG_INFO("server", "Loading Game Event Data...");               // must be after loading pools fully
    sGameEventMgr->LoadHolidayDates();                           // Must be after loading DBC
    sGameEventMgr->LoadFromDB();                                 // Must be after loading holiday dates

    LOG_INFO("server", "Loading UNIT_NPC_FLAG_SPELLCLICK Data..."); // must be after LoadQuests
    sObjectMgr->LoadNPCSpellClickSpells();

    LOG_INFO("server", "Loading Vehicle Template Accessories...");
    sObjectMgr->LoadVehicleTemplateAccessories();                // must be after LoadCreatureTemplates() and LoadNPCSpellClickSpells()

    LOG_INFO("server", "Loading Vehicle Accessories...");
    sObjectMgr->LoadVehicleAccessories();                       // must be after LoadCreatureTemplates() and LoadNPCSpellClickSpells()

    LOG_INFO("server", "Loading SpellArea Data...");                // must be after quest load
    sSpellMgr->LoadSpellAreas();

    LOG_INFO("server", "Loading Area Trigger definitions");
    sObjectMgr->LoadAreaTriggers();

    LOG_INFO("server", "Loading Area Trigger Teleport definitions...");
    sObjectMgr->LoadAreaTriggerTeleports();

    LOG_INFO("server", "Loading Access Requirements...");
    sObjectMgr->LoadAccessRequirements();                        // must be after item template load

    LOG_INFO("server", "Loading Quest Area Triggers...");
    sObjectMgr->LoadQuestAreaTriggers();                         // must be after LoadQuests

    LOG_INFO("server", "Loading Tavern Area Triggers...");
    sObjectMgr->LoadTavernAreaTriggers();

    LOG_INFO("server", "Loading AreaTrigger script names...");
    sObjectMgr->LoadAreaTriggerScripts();

    LOG_INFO("server", "Loading LFG entrance positions..."); // Must be after areatriggers
    sLFGMgr->LoadLFGDungeons();

    LOG_INFO("server", "Loading Dungeon boss data...");
    sObjectMgr->LoadInstanceEncounters();

    LOG_INFO("server", "Loading LFG rewards...");
    sLFGMgr->LoadRewards();

    LOG_INFO("server", "Loading Graveyard-zone links...");
    sGraveyard->LoadGraveyardZones();

    LOG_INFO("server", "Loading spell pet auras...");
    sSpellMgr->LoadSpellPetAuras();

    LOG_INFO("server", "Loading Spell target coordinates...");
    sSpellMgr->LoadSpellTargetPositions();

    LOG_INFO("server", "Loading enchant custom attributes...");
    sSpellMgr->LoadEnchantCustomAttr();

    LOG_INFO("server", "Loading linked spells...");
    sSpellMgr->LoadSpellLinked();

    LOG_INFO("server", "Loading Player Create Data...");
    sObjectMgr->LoadPlayerInfo();

    LOG_INFO("server", "Loading Exploration BaseXP Data...");
    sObjectMgr->LoadExplorationBaseXP();

    LOG_INFO("server", "Loading Pet Name Parts...");
    sObjectMgr->LoadPetNames();

    CharacterDatabaseCleaner::CleanDatabase();

    LOG_INFO("server", "Loading the max pet number...");
    sObjectMgr->LoadPetNumber();

    LOG_INFO("server", "Loading pet level stats...");
    sObjectMgr->LoadPetLevelInfo();

    LOG_INFO("server", "Loading Player level dependent mail rewards...");
    sObjectMgr->LoadMailLevelRewards();

    // Loot tables
    LoadLootTables();

    LOG_INFO("server", "Loading Skill Discovery Table...");
    LoadSkillDiscoveryTable();

    LOG_INFO("server", "Loading Skill Extra Item Table...");
    LoadSkillExtraItemTable();

    LOG_INFO("server", "Loading Skill Perfection Data Table...");
    LoadSkillPerfectItemTable();

    LOG_INFO("server", "Loading Skill Fishing base level requirements...");
    sObjectMgr->LoadFishingBaseSkillLevel();

    LOG_INFO("server", "Loading Achievements...");
    sAchievementMgr->LoadAchievementReferenceList();

    LOG_INFO("server", "Loading Achievement Criteria Lists...");
    sAchievementMgr->LoadAchievementCriteriaList();

    LOG_INFO("server", "Loading Achievement Criteria Data...");
    sAchievementMgr->LoadAchievementCriteriaData();

    LOG_INFO("server", "Loading Achievement Rewards...");
    sAchievementMgr->LoadRewards();

    LOG_INFO("server", "Loading Completed Achievements...");
    sAchievementMgr->LoadCompletedAchievements();

    ///- Load dynamic data tables from the database
    LOG_INFO("server", "Loading Item Auctions...");
    sAuctionMgr->LoadAuctionItems();

    LOG_INFO("server", "Loading Auctions...");
    sAuctionMgr->LoadAuctions();

    sGuildMgr->LoadGuilds();

    LOG_INFO("server", "Loading ArenaTeams...");
    sArenaTeamMgr->LoadArenaTeams();

    LOG_INFO("server", "Loading Groups...");
    sGroupMgr->LoadGroups();

    LOG_INFO("server", "Loading ReservedNames...");
    sObjectMgr->LoadReservedPlayersNames();

    LOG_INFO("server", "Loading GameObjects for quests...");
    sObjectMgr->LoadGameObjectForQuests();

    LOG_INFO("server", "Loading BattleMasters...");
    sBattlegroundMgr->LoadBattleMastersEntry();

    LOG_INFO("server", "Loading GameTeleports...");
    sObjectMgr->LoadGameTele();

    LOG_INFO("server", "Loading Gossip menu...");
    sObjectMgr->LoadGossipMenu();

    LOG_INFO("server", "Loading Gossip menu options...");
    sObjectMgr->LoadGossipMenuItems();

    LOG_INFO("server", "Loading Vendors...");
    sObjectMgr->LoadVendors();                                   // must be after load CreatureTemplate and ItemTemplate

    LOG_INFO("server", "Loading Trainers...");
    sObjectMgr->LoadTrainerSpell();                              // must be after load CreatureTemplate

    LOG_INFO("server", "Loading Waypoints...");
    sWaypointMgr->Load();

    LOG_INFO("server", "Loading SmartAI Waypoints...");
    sSmartWaypointMgr->LoadFromDB();

    LOG_INFO("server", "Loading Creature Formations...");
    sFormationMgr->LoadCreatureFormations();

    LOG_INFO("server", "Loading World States...");              // must be loaded before battleground, outdoor PvP and conditions
    LoadWorldStates();

    LOG_INFO("server", "Loading Conditions...");
    sConditionMgr->LoadConditions();

    LOG_INFO("server", "Loading faction change achievement pairs...");
    sObjectMgr->LoadFactionChangeAchievements();

    LOG_INFO("server", "Loading faction change spell pairs...");
    sObjectMgr->LoadFactionChangeSpells();

    LOG_INFO("server", "Loading faction change item pairs...");
    sObjectMgr->LoadFactionChangeItems();

    LOG_INFO("server", "Loading faction change reputation pairs...");
    sObjectMgr->LoadFactionChangeReputations();

    LOG_INFO("server", "Loading faction change title pairs...");
    sObjectMgr->LoadFactionChangeTitles();

    LOG_INFO("server", "Loading faction change quest pairs...");
    sObjectMgr->LoadFactionChangeQuests();

    LOG_INFO("server", "Loading GM tickets...");
    sTicketMgr->LoadTickets();

    LOG_INFO("server", "Loading GM surveys...");
    sTicketMgr->LoadSurveys();

    LOG_INFO("server", "Loading client addons...");
    AddonMgr::LoadFromDB();

    // pussywizard:
    LOG_INFO("server", "Deleting invalid mail items...");
    LOG_INFO("server", " ");
    CharacterDatabase.Query("DELETE mi FROM mail_items mi LEFT JOIN item_instance ii ON mi.item_guid = ii.guid WHERE ii.guid IS NULL");
    CharacterDatabase.Query("DELETE mi FROM mail_items mi LEFT JOIN mail m ON mi.mail_id = m.id WHERE m.id IS NULL");
    CharacterDatabase.Query("UPDATE mail m LEFT JOIN mail_items mi ON m.id = mi.mail_id SET m.has_items=0 WHERE m.has_items<>0 AND mi.mail_id IS NULL");

    ///- Handle outdated emails (delete/return)
    LOG_INFO("server", "Returning old mails...");
    LOG_INFO("server", " ");
    sObjectMgr->ReturnOrDeleteOldMails(false);

    ///- Load AutoBroadCast
    LOG_INFO("server", "Loading Autobroadcasts...");
    LoadAutobroadcasts();

    ///- Load and initialize scripts
    sObjectMgr->LoadSpellScripts();                              // must be after load Creature/Gameobject(Template/Data)
    sObjectMgr->LoadEventScripts();                              // must be after load Creature/Gameobject(Template/Data)
    sObjectMgr->LoadWaypointScripts();

    LOG_INFO("server", "Loading spell script names...");
    sObjectMgr->LoadSpellScriptNames();

    LOG_INFO("server", "Loading Creature Texts...");
    sCreatureTextMgr->LoadCreatureTexts();

    LOG_INFO("server", "Loading Creature Text Locales...");
    sCreatureTextMgr->LoadCreatureTextLocales();

    LOG_INFO("server", "Loading Scripts...");
    sScriptMgr->LoadDatabase();

    LOG_INFO("server", "Validating spell scripts...");
    sObjectMgr->ValidateSpellScripts();

    LOG_INFO("server", "Loading SmartAI scripts...");
    sSmartScriptMgr->LoadSmartAIFromDB();

    LOG_INFO("server", "Loading Calendar data...");
    sCalendarMgr->LoadFromDB();

    LOG_INFO("server", "Initializing SpellInfo precomputed data..."); // must be called after loading items, professions, spells and pretty much anything
    LOG_INFO("server", " ");
    sObjectMgr->InitializeSpellInfoPrecomputedData();

    LOG_INFO("server.loading", "Initialize commands...");
    Warhead::ChatCommands::LoadCommandMap();

    ///- Initialize game time and timers
    LOG_INFO("server", "Initialize game time and timers");
    LOG_INFO("server", " ");
    GameTime::UpdateGameTimers();

    LoginDatabase.PExecute("INSERT INTO uptime (realmid, starttime, uptime, revision) VALUES(%u, %u, 0, '%s')",
                           realm.Id.Realm, uint32(GameTime::GetStartTime()), GitRevision::GetFullVersion()); // One-time query

    m_timers[WUPDATE_WEATHERS].SetInterval(1 * IN_MILLISECONDS);
    m_timers[WUPDATE_AUCTIONS].SetInterval(MINUTE * IN_MILLISECONDS);
    m_timers[WUPDATE_AUCTIONS].SetCurrent(MINUTE * IN_MILLISECONDS);
    m_timers[WUPDATE_UPTIME].SetInterval(CONF_GET_INT("UpdateUptimeInterval") *MINUTE * IN_MILLISECONDS);
    //Update "uptime" table based on configuration entry in minutes.

    m_timers[WUPDATE_CORPSES].SetInterval(20 * MINUTE * IN_MILLISECONDS);

    // clean logs table every 14 days by default
    m_timers[WUPDATE_AUTOBROADCAST].SetInterval(CONF_GET_INT("AutoBroadcast.Timer"));

    m_timers[WUPDATE_PINGDB].SetInterval(CONF_GET_INT("MaxPingTime") * MINUTE * IN_MILLISECONDS);  // Mysql ping time in minutes

    // our speed up
    m_timers[WUPDATE_5_SECS].SetInterval(5 * IN_MILLISECONDS);

    mail_expire_check_timer = GameTime::GetGameTime() + 6 * 3600;

    ///- Initilize static helper structures
    AIRegistry::Initialize();

    ///- Initialize MapManager
    LOG_INFO("server", "Starting Map System");
    LOG_INFO("server", " ");
    sMapMgr->Initialize();

    LOG_INFO("server", "Starting Game Event system...");
    LOG_INFO("server", " ");
    uint32 nextGameEvent = sGameEventMgr->StartSystem();
    m_timers[WUPDATE_EVENTS].SetInterval(nextGameEvent);    //depend on next event

    // Delete all characters which have been deleted X days before
    Player::DeleteOldCharacters();

    // Delete all custom channels which haven't been used for PreserveCustomChannelDuration days.
    Channel::CleanOldChannelsInDB();

    LOG_INFO("server.loading", "Initializing Opcodes...");
    opcodeTable.Initialize();

    LOG_INFO("server", "Starting Arena Season...");
    LOG_INFO("server", " ");
    sGameEventMgr->StartArenaSeason();

    sTicketMgr->Initialize();

    ///- Initialize Battlegrounds
    LOG_INFO("server", "Starting Battleground System");
    sBattlegroundMgr->CreateInitialBattlegrounds();
    sBattlegroundMgr->InitAutomaticArenaPointDistribution();

    ///- Initialize outdoor pvp
    LOG_INFO("server", "Starting Outdoor PvP System");
    sOutdoorPvPMgr->InitOutdoorPvP();

    ///- Initialize Battlefield
    LOG_INFO("server", "Starting Battlefield System");
    sBattlefieldMgr->InitBattlefield();

    LOG_INFO("server", "Loading Transports...");
    sTransportMgr->SpawnContinentTransports();

    ///- Initialize Warden
    LOG_INFO("server", "Loading Warden Checks..." );
    sWardenCheckMgr->LoadWardenChecks();

    LOG_INFO("server", "Loading Warden Action Overrides..." );
    sWardenCheckMgr->LoadWardenOverrides();

    LOG_INFO("server", "Deleting expired bans...");
    LoginDatabase.Execute("DELETE FROM ip_banned WHERE unbandate <= UNIX_TIMESTAMP() AND unbandate<>bandate");      // One-time query

    LOG_INFO("server", "Calculate next daily quest reset time...");
    InitDailyQuestResetTime();

    LOG_INFO("server", "Calculate next weekly quest reset time..." );
    InitWeeklyQuestResetTime();

    LOG_INFO("server", "Calculate next monthly quest reset time...");
    InitMonthlyQuestResetTime();

    LOG_INFO("server", "Calculate random battleground reset time..." );
    InitRandomBGResetTime();

    LOG_INFO("server", "Calculate deletion of old calendar events time...");
    InitCalendarOldEventsDeletionTime();

    LOG_INFO("server", "Calculate Guild cap reset time...");
    LOG_INFO("server", " ");
    InitGuildResetTime();

    LOG_INFO("server", "Load Petitions...");
    sPetitionMgr->LoadPetitions();

    LOG_INFO("server", "Load Petition Signs...");
    sPetitionMgr->LoadSignatures();

    LOG_INFO("server", "Load Stored Loot Items...");
    sLootItemStorage->LoadStorageFromDB();

    LOG_INFO("server", "Load Channel Rights...");
    ChannelMgr::LoadChannelRights();

    LOG_INFO("server", "Load Channels...");
    ChannelMgr::LoadChannels();

#ifdef ELUNA
    ///- Run eluna scripts.
    // in multithread foreach: run scripts
    sEluna->RunScripts();
    sEluna->OnConfigLoad(false, false); // Must be done after Eluna is initialized and scripts have run.
#endif

    if (CONF_GET_BOOL("PreloadAllNonInstancedMapGrids"))
    {
        LOG_INFO("server", "Loading all grids for all non-instanced maps...");

        for (uint32 i = 0; i < sMapStore.GetNumRows(); ++i)
        {
            MapEntry const* mapEntry = sMapStore.LookupEntry(i);

            if (mapEntry && !mapEntry->Instanceable())
            {
                Map* map = sMapMgr->CreateBaseMap(mapEntry->MapID);

                if (map)
                {
                    LOG_INFO("server", ">> Loading all grids for map %u", map->GetId());
                    map->LoadAllCells();
                }
            }
        }
    }

    uint32 startupDuration = GetMSTimeDiffToNow(startupBegin);
    LOG_INFO("server", " ");
    LOG_INFO("server", "WORLD: World initialized in %u minutes %u seconds", (startupDuration / 60000), ((startupDuration % 60000) / 1000)); // outError for red color in console
    LOG_INFO("server", " ");

    if (sConfigMgr->isDryRun())
    {
        LOG_INFO("server", "AzerothCore dry run completed, terminating.");
        exit(0);
    }
}

void World::DetectDBCLang()
{
    uint8 m_lang_confid = sConfigMgr->GetOption<int32>("DBC.Locale", 255);

    if (m_lang_confid != 255 && m_lang_confid >= TOTAL_LOCALES)
    {
        LOG_ERROR("server", "Incorrect DBC.Locale! Must be >= 0 and < %d (set to 0)", TOTAL_LOCALES);
        m_lang_confid = LOCALE_enUS;
    }

    ChrRacesEntry const* race = sChrRacesStore.LookupEntry(1);
    std::string availableLocalsStr;

    uint8 default_locale = TOTAL_LOCALES;
    for (uint8 i = default_locale - 1; i < TOTAL_LOCALES; --i) // -1 will be 255 due to uint8
    {
        if (race->name[i][0] != '\0')                     // check by race names
        {
            default_locale = i;
            m_availableDbcLocaleMask |= (1 << i);
            availableLocalsStr += localeNames[i];
            availableLocalsStr += " ";
        }
    }

    if (default_locale != m_lang_confid && m_lang_confid < TOTAL_LOCALES &&
            (m_availableDbcLocaleMask & (1 << m_lang_confid)))
    {
        default_locale = m_lang_confid;
    }

    if (default_locale >= TOTAL_LOCALES)
    {
        LOG_ERROR("server", "Unable to determine your DBC Locale! (corrupt DBC?)");
        exit(1);
    }

    m_defaultDbcLocale = LocaleConstant(default_locale);

    LOG_INFO("server", "Using %s DBC Locale as default. All available DBC locales: %s", localeNames[GetDefaultDbcLocale()], availableLocalsStr.empty() ? "<none>" : availableLocalsStr.c_str());
    LOG_INFO("server", " ");
}

void World::LoadAutobroadcasts()
{
    uint32 oldMSTime = getMSTime();

    m_Autobroadcasts.clear();
    m_AutobroadcastsWeights.clear();

    uint32 realmId = sConfigMgr->GetOption<int32>("RealmID", 0);
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_AUTOBROADCAST);
    stmt->setInt32(0, realmId);
    PreparedQueryResult result = LoginDatabase.Query(stmt);

    if (!result)
    {
        LOG_INFO("server", ">> Loaded 0 autobroadcasts definitions. DB table `autobroadcast` is empty for this realm!");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        uint8 id = fields[0].GetUInt8();

        m_Autobroadcasts[id] = fields[2].GetString();
        m_AutobroadcastsWeights[id] = fields[1].GetUInt8();

        ++count;
    } while (result->NextRow());

    LOG_INFO("server", ">> Loaded %u autobroadcast definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server", " ");
}

/// Update the World !
void World::Update(uint32 diff)
{
    ///- Update the game time and check for shutdown time
    _UpdateGameTime();
    time_t currentGameTime = GameTime::GetGameTime();

    sWorldUpdateTime.UpdateWithDiff(diff);

    // Record update if recording set in log and diff is greater then minimum set in log
    sWorldUpdateTime.RecordUpdateTime(getMSTime(), diff, GetActiveSessionCount());

    DynamicVisibilityMgr::Update(GetActiveSessionCount());

    ///- Update the different timers
    for (int i = 0; i < WUPDATE_COUNT; ++i)
    {
        if (m_timers[i].GetCurrent() >= 0)
            m_timers[i].Update(diff);
        else
            m_timers[i].SetCurrent(0);
    }

    // pussywizard: our speed up and functionality
    if (m_timers[WUPDATE_5_SECS].Passed())
    {
        m_timers[WUPDATE_5_SECS].Reset();

        // moved here from HandleCharEnumOpcode
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_EXPIRED_BANS);
        CharacterDatabase.Execute(stmt);

        // copy players hashmapholder to avoid mutex
        WhoListCacheMgr::Update();
    }

    /// Handle daily quests reset time
    if (currentGameTime > m_NextDailyQuestReset)
        ResetDailyQuests();

    /// Handle weekly quests reset time
    if (currentGameTime > m_NextWeeklyQuestReset)
        ResetWeeklyQuests();

    /// Handle monthly quests reset time
    if (currentGameTime > m_NextMonthlyQuestReset)
        ResetMonthlyQuests();

    if (currentGameTime > m_NextRandomBGReset)
        ResetRandomBG();

    if (currentGameTime > m_NextCalendarOldEventsDeletionTime)
        CalendarDeleteOldEvents();

    if (currentGameTime > m_NextGuildReset)
        ResetGuildCap();

    // pussywizard:
    // acquire mutex now, this is kind of waiting for listing thread to finish it's work (since it can't process next packet)
    // so we don't have to do it in every packet that modifies auctions
    AsyncAuctionListingMgr::SetAuctionListingAllowed(false);
    {
        std::lock_guard<std::mutex> guard(AsyncAuctionListingMgr::GetLock());

        // pussywizard: handle auctions when the timer has passed
        if (m_timers[WUPDATE_AUCTIONS].Passed())
        {
            m_timers[WUPDATE_AUCTIONS].Reset();

            // pussywizard: handle expired auctions, auctions expired when realm was offline are also handled here (not during loading when many required things aren't loaded yet)
            sAuctionMgr->Update();
        }

        AsyncAuctionListingMgr::Update(diff);

        if (currentGameTime > mail_expire_check_timer)
        {
            sObjectMgr->ReturnOrDeleteOldMails(true);
            mail_expire_check_timer = currentGameTime + 6 * 3600;
        }

        sWorldUpdateTime.RecordUpdateTimeReset();
        UpdateSessions(diff);
        sWorldUpdateTime.RecordUpdateTimeDuration("UpdateSessions");
    }

    // end of section with mutex
    AsyncAuctionListingMgr::SetAuctionListingAllowed(true);

    /// <li> Handle weather updates when the timer has passed
    if (m_timers[WUPDATE_WEATHERS].Passed())
    {
        m_timers[WUPDATE_WEATHERS].Reset();
        WeatherMgr::Update(uint32(m_timers[WUPDATE_WEATHERS].GetInterval()));
    }

    sWorldUpdateTime.RecordUpdateTimeReset();
    sLFGMgr->Update(diff, 0); // pussywizard: remove obsolete stuff before finding compatibility during map update
    sWorldUpdateTime.RecordUpdateTimeDuration("UpdateLFGMgr");

    sMapMgr->Update(diff);
    sWorldUpdateTime.RecordUpdateTimeDuration("UpdateMapMgr");

    if (CONF_GET_BOOL("AutoBroadcast.On"))
    {
        if (m_timers[WUPDATE_AUTOBROADCAST].Passed())
        {
            m_timers[WUPDATE_AUTOBROADCAST].Reset();
            SendAutoBroadcast();
        }
    }

    sBattlegroundMgr->Update(diff);
    sWorldUpdateTime.RecordUpdateTimeDuration("UpdateBattlegroundMgr");

    sOutdoorPvPMgr->Update(diff);
    sWorldUpdateTime.RecordUpdateTimeDuration("UpdateOutdoorPvPMgr");

    sBattlefieldMgr->Update(diff);
    sWorldUpdateTime.RecordUpdateTimeDuration("BattlefieldMgr");

    sLFGMgr->Update(diff, 2); // pussywizard: handle created proposals
    sWorldUpdateTime.RecordUpdateTimeDuration("UpdateLFGMgr2");

    // execute callbacks from sql queries that were queued recently
    ProcessQueryCallbacks();
    sWorldUpdateTime.RecordUpdateTimeDuration("ProcessQueryCallbacks");

    /// <li> Update uptime table
    if (m_timers[WUPDATE_UPTIME].Passed())
    {
        uint32 tmpDiff = GameTime::GetUptime();
        uint32 maxOnlinePlayers = GetMaxPlayerCount();

        m_timers[WUPDATE_UPTIME].Reset();

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_UPTIME_PLAYERS);

        stmt->setUInt32(0, tmpDiff);
        stmt->setUInt16(1, uint16(maxOnlinePlayers));
        stmt->setUInt32(2, realm.Id.Realm);
        stmt->setUInt32(3, uint32(GameTime::GetStartTime()));

        LoginDatabase.Execute(stmt);
    }

    ///- Erase corpses once every 20 minutes
    if (m_timers[WUPDATE_CORPSES].Passed())
    {
        m_timers[WUPDATE_CORPSES].Reset();
        sMapMgr->DoForAllMaps([](Map* map)
        {
            map->RemoveOldCorpses();
        });
    }

    ///- Process Game events when necessary
    if (m_timers[WUPDATE_EVENTS].Passed())
    {
        m_timers[WUPDATE_EVENTS].Reset();                   // to give time for Update() to be processed
        uint32 nextGameEvent = sGameEventMgr->Update();
        m_timers[WUPDATE_EVENTS].SetInterval(nextGameEvent);
        m_timers[WUPDATE_EVENTS].Reset();
    }

    ///- Ping to keep MySQL connections alive
    if (m_timers[WUPDATE_PINGDB].Passed())
    {
        m_timers[WUPDATE_PINGDB].Reset();
#if defined(ENABLE_EXTRAS) && defined(ENABLE_EXTRA_LOGS)
        LOG_DEBUG("server", "Ping MySQL to keep connection alive");
#endif
        CharacterDatabase.KeepAlive();
        LoginDatabase.KeepAlive();
        WorldDatabase.KeepAlive();
    }

    // update the instance reset times
    sInstanceSaveMgr->Update();

    // And last, but not least handle the issued cli commands
    ProcessCliCommands();

    sScriptMgr->OnWorldUpdate(diff);

    SavingSystemMgr::Update(diff);
}

void World::ForceGameEventUpdate()
{
    m_timers[WUPDATE_EVENTS].Reset();                   // to give time for Update() to be processed
    uint32 nextGameEvent = sGameEventMgr->Update();
    m_timers[WUPDATE_EVENTS].SetInterval(nextGameEvent);
    m_timers[WUPDATE_EVENTS].Reset();
}

/// Send a packet to all players (except self if mentioned)
void World::SendGlobalMessage(WorldPacket* packet, WorldSession* self, TeamId teamId)
{
    SessionMap::const_iterator itr;
    for (itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (itr->second &&
                itr->second->GetPlayer() &&
                itr->second->GetPlayer()->IsInWorld() &&
                itr->second != self &&
                (teamId == TEAM_NEUTRAL || itr->second->GetPlayer()->GetTeamId() == teamId))
        {
            itr->second->SendPacket(packet);
        }
    }
}

/// Send a packet to all GMs (except self if mentioned)
void World::SendGlobalGMMessage(WorldPacket* packet, WorldSession* self, TeamId teamId)
{
    SessionMap::iterator itr;
    for (itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (itr->second &&
                itr->second->GetPlayer() &&
                itr->second->GetPlayer()->IsInWorld() &&
                itr->second != self &&
                !AccountMgr::IsPlayerAccount(itr->second->GetSecurity()) &&
                (teamId == TEAM_NEUTRAL || itr->second->GetPlayer()->GetTeamId() == teamId))
        {
            itr->second->SendPacket(packet);
        }
    }
}

namespace Warhead
{
    class WorldWorldTextBuilder
    {
    public:
        typedef std::vector<WorldPacket*> WorldPacketList;
        explicit WorldWorldTextBuilder(uint32 textId, va_list* args = nullptr) : i_textId(textId), i_args(args) {}
        void operator()(WorldPacketList& data_list, LocaleConstant loc_idx)
        {
            char const* text = sGameLocale->GetWarheadString(i_textId, loc_idx);

            if (i_args)
            {
                // we need copy va_list before use or original va_list will corrupted
                va_list ap;
                va_copy(ap, *i_args);

                char str[2048];
                vsnprintf(str, 2048, text, ap);
                va_end(ap);

                do_helper(data_list, &str[0]);
            }
            else
                do_helper(data_list, (char*)text);
        }
    private:
        char* lineFromMessage(char*& pos) { char* start = strtok(pos, "\n"); pos = nullptr; return start; }
        void do_helper(WorldPacketList& data_list, char* text)
        {
            char* pos = text;
            while (char* line = lineFromMessage(pos))
            {
                WorldPacket* data = new WorldPacket();
                ChatHandler::BuildChatPacket(*data, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, nullptr, nullptr, line);
                data_list.push_back(data);
            }
        }

        uint32 i_textId;
        va_list* i_args;
    };
}                                                           // namespace Warhead

/// Send a System Message to all players (except self if mentioned)
void World::SendWorldText(uint32 string_id, ...)
{
    va_list ap;
    va_start(ap, string_id);

    Warhead::WorldWorldTextBuilder wt_builder(string_id, &ap);
    Warhead::Game::Locale::LocalizedPacketListDo<Warhead::WorldWorldTextBuilder> wt_do(wt_builder);
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (!itr->second || !itr->second->GetPlayer() || !itr->second->GetPlayer()->IsInWorld())
            continue;

        wt_do(itr->second->GetPlayer());
    }

    va_end(ap);
}

/// Send a System Message to all GMs (except self if mentioned)
void World::SendGMText(uint32 string_id, ...)
{
    va_list ap;
    va_start(ap, string_id);

    Warhead::WorldWorldTextBuilder wt_builder(string_id, &ap);
    Warhead::Game::Locale::LocalizedPacketListDo<Warhead::WorldWorldTextBuilder> wt_do(wt_builder);
    for (SessionMap::iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (!itr->second || !itr->second->GetPlayer() || !itr->second->GetPlayer()->IsInWorld())
            continue;

        if (AccountMgr::IsPlayerAccount(itr->second->GetSecurity()))
            continue;

        wt_do(itr->second->GetPlayer());
    }

    va_end(ap);
}

/// DEPRECATED, only for debug purpose. Send a System Message to all players (except self if mentioned)
void World::SendGlobalText(const char* text, WorldSession* self)
{
    WorldPacket data;

    // need copy to prevent corruption by strtok call in LineFromMessage original string
    char* buf = strdup(text);
    char* pos = buf;

    while (char* line = ChatHandler::LineFromMessage(pos))
    {
        ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, nullptr, nullptr, line);
        SendGlobalMessage(&data, self);
    }

    free(buf);
}

/// Send a packet to all players (or players selected team) in the zone (except self if mentioned)
bool World::SendZoneMessage(uint32 zone, WorldPacket* packet, WorldSession* self, TeamId teamId)
{
    bool foundPlayerToSend = false;
    SessionMap::const_iterator itr;

    for (itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (itr->second &&
                itr->second->GetPlayer() &&
                itr->second->GetPlayer()->IsInWorld() &&
                itr->second->GetPlayer()->GetZoneId() == zone &&
                itr->second != self &&
                (teamId == TEAM_NEUTRAL || itr->second->GetPlayer()->GetTeamId() == teamId))
        {
            itr->second->SendPacket(packet);
            foundPlayerToSend = true;
        }
    }

    return foundPlayerToSend;
}

/// Send a System Message to all players in the zone (except self if mentioned)
void World::SendZoneText(uint32 zone, const char* text, WorldSession* self, TeamId teamId)
{
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, nullptr, nullptr, text);
    SendZoneMessage(zone, &data, self, teamId);
}

/// Kick (and save) all players
void World::KickAll()
{
    m_QueuedPlayer.clear();                                 // prevent send queue update packet and login queued sessions

    // session not removed at kick and will removed in next update tick
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        itr->second->KickPlayer("KickAll sessions");

    // pussywizard: kick offline sessions
    for (SessionMap::const_iterator itr = m_offlineSessions.begin(); itr != m_offlineSessions.end(); ++itr)
        itr->second->KickPlayer("KickAll offline sessions");
}

/// Kick (and save) all players with security level less `sec`
void World::KickAllLess(AccountTypes sec)
{
    // session not removed at kick and will removed in next update tick
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetSecurity() < sec)
            itr->second->KickPlayer("KickAllLess");
}

/// Update the game time
void World::_UpdateGameTime()
{
    ///- update the time
    time_t lastGameTime = GameTime::GetGameTime();
    GameTime::UpdateGameTimers();

    uint32 elapsed = uint32(GameTime::GetGameTime() - lastGameTime);

    ///- if there is a shutdown timer
    if (!IsStopped() && m_ShutdownTimer > 0 && elapsed > 0)
    {
        ///- ... and it is overdue, stop the world (set m_stopEvent)
        if (m_ShutdownTimer <= elapsed)
        {
            if (!(m_ShutdownMask & SHUTDOWN_MASK_IDLE) || GetActiveAndQueuedSessionCount() == 0)
                m_stopEvent = true;                         // exist code already set
            else
                m_ShutdownTimer = 1;                        // minimum timer value to wait idle state
        }
        ///- ... else decrease it and if necessary display a shutdown countdown to the users
        else
        {
            m_ShutdownTimer -= elapsed;

            ShutdownMsg();
        }
    }
}

/// Shutdown the server
void World::ShutdownServ(uint32 time, uint32 options, uint8 exitcode)
{
    // ignore if server shutdown at next tick
    if (IsStopped())
        return;

    m_ShutdownMask = options;
    m_ExitCode = exitcode;

    ///- If the shutdown time is 0, set m_stopEvent (except if shutdown is 'idle' with remaining sessions)
    if (time == 0)
    {
        if (!(options & SHUTDOWN_MASK_IDLE) || GetActiveAndQueuedSessionCount() == 0)
            m_stopEvent = true;                             // exist code already set
        else
            m_ShutdownTimer = 1;                            //So that the session count is re-evaluated at next world tick
    }
    ///- Else set the shutdown timer and warn users
    else
    {
        m_ShutdownTimer = time;
        ShutdownMsg(true);
    }

    sScriptMgr->OnShutdownInitiate(ShutdownExitCode(exitcode), ShutdownMask(options));
}

/// Display a shutdown message to the user(s)
void World::ShutdownMsg(bool show, Player* player)
{
    // not show messages for idle shutdown mode
    if (m_ShutdownMask & SHUTDOWN_MASK_IDLE)
        return;

    ///- Display a message every 12 hours, hours, 5 minutes, minute, 5 seconds and finally seconds
    if (show ||
            (m_ShutdownTimer < 5 * MINUTE && (m_ShutdownTimer % 15) == 0) || // < 5 min; every 15 sec
            (m_ShutdownTimer < 15 * MINUTE && (m_ShutdownTimer % MINUTE) == 0) || // < 15 min ; every 1 min
            (m_ShutdownTimer < 30 * MINUTE && (m_ShutdownTimer % (5 * MINUTE)) == 0) || // < 30 min ; every 5 min
            (m_ShutdownTimer < 12 * HOUR && (m_ShutdownTimer % HOUR) == 0) || // < 12 h ; every 1 h
            (m_ShutdownTimer > 12 * HOUR && (m_ShutdownTimer % (12 * HOUR)) == 0)) // > 12 h ; every 12 h
    {
        std::string str = secsToTimeString(m_ShutdownTimer).append(".");

        ServerMessageType msgid = (m_ShutdownMask & SHUTDOWN_MASK_RESTART) ? SERVER_MSG_RESTART_TIME : SERVER_MSG_SHUTDOWN_TIME;

        SendServerMessage(msgid, str.c_str(), player);
#if defined(ENABLE_EXTRAS) && defined(ENABLE_EXTRA_LOGS)
        LOG_DEBUG("server", "Server is %s in %s", (m_ShutdownMask & SHUTDOWN_MASK_RESTART ? "restart" : "shuttingdown"), str.c_str());
#endif
    }
}

/// Cancel a planned server shutdown
void World::ShutdownCancel()
{
    // nothing cancel or too later
    if (!m_ShutdownTimer || m_stopEvent)
        return;

    ServerMessageType msgid = (m_ShutdownMask & SHUTDOWN_MASK_RESTART) ? SERVER_MSG_RESTART_CANCELLED : SERVER_MSG_SHUTDOWN_CANCELLED;

    m_ShutdownMask = 0;
    m_ShutdownTimer = 0;
    m_ExitCode = SHUTDOWN_EXIT_CODE;                       // to default value
    SendServerMessage(msgid);

#if defined(ENABLE_EXTRAS) && defined(ENABLE_EXTRA_LOGS)
    LOG_DEBUG("server", "Server %s cancelled.", (m_ShutdownMask & SHUTDOWN_MASK_RESTART ? "restart" : "shuttingdown"));
#endif

    sScriptMgr->OnShutdownCancel();
}

/// Send a server message to the user(s)
void World::SendServerMessage(ServerMessageType type, const char* text, Player* player)
{
    WorldPacket data(SMSG_SERVER_MESSAGE, 50);              // guess size
    data << uint32(type);
    if (type <= SERVER_MSG_STRING)
        data << text;

    if (player)
        player->GetSession()->SendPacket(&data);
    else
        SendGlobalMessage(&data);
}

void World::UpdateSessions(uint32 diff)
{
    ///- Add new sessions
    WorldSession* sess = nullptr;
    while (addSessQueue.next(sess))
        AddSession_ (sess);

    ///- Then send an update signal to remaining ones
    for (SessionMap::iterator itr = m_sessions.begin(), next; itr != m_sessions.end(); itr = next)
    {
        next = itr;
        ++next;

        ///- and remove not active sessions from the list
        WorldSession* pSession = itr->second;
        WorldSessionFilter updater(pSession);

        // pussywizard:
        if (pSession->HandleSocketClosed())
        {
            if (!RemoveQueuedPlayer(pSession) && CONF_GET_INT("DisconnectToleranceInterval"))
                m_disconnects[pSession->GetAccountId()] = GameTime::GetGameTime();
            m_sessions.erase(itr);
            // there should be no offline session if current one is logged onto a character
            SessionMap::iterator iter;
            if ((iter = m_offlineSessions.find(pSession->GetAccountId())) != m_offlineSessions.end())
            {
                WorldSession* tmp = iter->second;
                m_offlineSessions.erase(iter);
                tmp->SetShouldSetOfflineInDB(false);
                delete tmp;
            }
            pSession->SetOfflineTime(GameTime::GetGameTime());
            m_offlineSessions[pSession->GetAccountId()] = pSession;
            continue;
        }

        if (!pSession->Update(diff, updater))
        {
            if (!RemoveQueuedPlayer(pSession) && CONF_GET_INT("DisconnectToleranceInterval"))
                m_disconnects[pSession->GetAccountId()] = GameTime::GetGameTime();
            m_sessions.erase(itr);
            if (m_offlineSessions.find(pSession->GetAccountId()) != m_offlineSessions.end()) // pussywizard: don't set offline in db because offline session for that acc is present (character is in world)
                pSession->SetShouldSetOfflineInDB(false);
            delete pSession;
        }
    }

    // pussywizard:
    if (m_offlineSessions.empty())
        return;
    uint32 currTime = GameTime::GetGameTime();
    for (SessionMap::iterator itr = m_offlineSessions.begin(), next; itr != m_offlineSessions.end(); itr = next)
    {
        next = itr;
        ++next;
        WorldSession* pSession = itr->second;
        if (!pSession->GetPlayer() || pSession->GetOfflineTime() + 60 < currTime || pSession->IsKicked())
        {
            m_offlineSessions.erase(itr);
            if (m_sessions.find(pSession->GetAccountId()) != m_sessions.end())
                pSession->SetShouldSetOfflineInDB(false); // pussywizard: don't set offline in db because new session for that acc is already created
            delete pSession;
        }
    }
}

// This handles the issued and queued CLI commands
void World::ProcessCliCommands()
{
    CliCommandHolder::Print zprint = nullptr;
    void* callbackArg = nullptr;
    CliCommandHolder* command = nullptr;

    while (cliCmdQueue.next(command))
    {
        LOG_INFO("server.worldserver", "CLI command under processing...");

        zprint = command->m_print;
        callbackArg = command->m_callbackArg;
        CliHandler handler(callbackArg, zprint);
        handler.ParseCommands(command->m_command);

        if (command->m_commandFinished)
            command->m_commandFinished(callbackArg, !handler.HasSentErrorMessage());

        delete command;
    }
}

void World::SendAutoBroadcast()
{
    if (m_Autobroadcasts.empty())
        return;

    uint32 weight = 0;
    AutobroadcastsWeightMap selectionWeights;

    std::string msg;

    for (AutobroadcastsWeightMap::const_iterator it = m_AutobroadcastsWeights.begin(); it != m_AutobroadcastsWeights.end(); ++it)
    {
        if (it->second)
        {
            weight += it->second;
            selectionWeights[it->first] = it->second;
        }
    }

    if (weight)
    {
        uint32 selectedWeight = urand(0, weight - 1);
        weight = 0;
        for (AutobroadcastsWeightMap::const_iterator it = selectionWeights.begin(); it != selectionWeights.end(); ++it)
        {
            weight += it->second;
            if (selectedWeight < weight)
            {
                msg = m_Autobroadcasts[it->first];
                break;
            }
        }
    }
    else
        msg = m_Autobroadcasts[urand(0, m_Autobroadcasts.size())];

    uint32 abcenter = CONF_GET_INT("AutoBroadcast.Center");

    if (abcenter == 0)
        sWorld->SendWorldText(LANG_AUTO_BROADCAST, msg.c_str());

    else if (abcenter == 1)
    {
        WorldPacket data(SMSG_NOTIFICATION, (msg.size() + 1));
        data << msg;
        sWorld->SendGlobalMessage(&data);
    }

    else if (abcenter == 2)
    {
        sWorld->SendWorldText(LANG_AUTO_BROADCAST, msg.c_str());

        WorldPacket data(SMSG_NOTIFICATION, (msg.size() + 1));
        data << msg;
        sWorld->SendGlobalMessage(&data);
    }

#if defined(ENABLE_EXTRAS) && defined(ENABLE_EXTRA_LOGS)
    LOG_DEBUG("server", "AutoBroadcast: '%s'", msg.c_str());
#endif
}

void World::UpdateRealmCharCount(uint32 accountId)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_COUNT);
    stmt->setUInt32(0, accountId);
    _queryProcessor.AddCallback(CharacterDatabase.AsyncQuery(stmt).WithPreparedCallback(std::bind(&World::_UpdateRealmCharCount, this, std::placeholders::_1)));
}

void World::_UpdateRealmCharCount(PreparedQueryResult resultCharCount)
{
    if (resultCharCount)
    {
        Field* fields = resultCharCount->Fetch();
        uint32 accountId = fields[0].GetUInt32();
        uint8 charCount = uint8(fields[1].GetUInt64());

        LoginDatabaseTransaction trans = LoginDatabase.BeginTransaction();

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_REALM_CHARACTERS_BY_REALM);
        stmt->setUInt32(0, accountId);
        stmt->setUInt32(1, realm.Id.Realm);
        trans->Append(stmt);

        stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_REALM_CHARACTERS);
        stmt->setUInt8(0, charCount);
        stmt->setUInt32(1, accountId);
        stmt->setUInt32(2, realm.Id.Realm);
        trans->Append(stmt);

        LoginDatabase.CommitTransaction(trans);
    }
}

// int8 dayOfWeek: 0 (sunday) to 6 (saturday)
time_t World::GetNextTimeWithDayAndHour(int8 dayOfWeek, int8 hour)
{
    if (hour < 0 || hour > 23)
        hour = 0;
    time_t curr = GameTime::GetGameTime();
    tm localTm;
    localtime_r(&curr, &localTm);
    localTm.tm_hour = hour;
    localTm.tm_min  = 0;
    localTm.tm_sec  = 0;
    uint32 add;
    if (dayOfWeek < 0 || dayOfWeek > 6)
        dayOfWeek = (localTm.tm_wday + 1) % 7;
    if (localTm.tm_wday >= dayOfWeek)
        add = (7 - (localTm.tm_wday - dayOfWeek)) * DAY;
    else
        add = (dayOfWeek - localTm.tm_wday) * DAY;
    return mktime(&localTm) + add;
}

// int8 month: 0 (january) to 11 (december)
time_t World::GetNextTimeWithMonthAndHour(int8 month, int8 hour)
{
    if (hour < 0 || hour > 23)
        hour = 0;
    time_t curr = GameTime::GetGameTime();
    tm localTm;
    localtime_r(&curr, &localTm);
    localTm.tm_mday = 1;
    localTm.tm_hour = hour;
    localTm.tm_min  = 0;
    localTm.tm_sec  = 0;
    if (month < 0 || month > 11)
    {
        month = (localTm.tm_mon + 1) % 12;
        if (month == 0)
            localTm.tm_year += 1;
    }
    else if (localTm.tm_mon >= month)
        localTm.tm_year += 1;
    localTm.tm_mon = month;
    return mktime(&localTm);
}

void World::InitWeeklyQuestResetTime()
{
    time_t wstime = time_t(sWorld->getWorldState(WS_WEEKLY_QUEST_RESET_TIME));
    m_NextWeeklyQuestReset = wstime ? wstime : GetNextTimeWithDayAndHour(4, 6);
    if (!wstime)
        sWorld->setWorldState(WS_WEEKLY_QUEST_RESET_TIME, uint64(m_NextWeeklyQuestReset));
}

void World::InitDailyQuestResetTime()
{
    time_t wstime = time_t(sWorld->getWorldState(WS_DAILY_QUEST_RESET_TIME));
    m_NextDailyQuestReset = wstime ? wstime : GetNextTimeWithDayAndHour(-1, 6);
    if (!wstime)
        sWorld->setWorldState(WS_DAILY_QUEST_RESET_TIME, uint64(m_NextDailyQuestReset));
}

void World::InitMonthlyQuestResetTime()
{
    time_t wstime = time_t(sWorld->getWorldState(WS_MONTHLY_QUEST_RESET_TIME));
    m_NextMonthlyQuestReset = wstime ? wstime : GetNextTimeWithMonthAndHour(-1, 6);
    if (!wstime)
        sWorld->setWorldState(WS_MONTHLY_QUEST_RESET_TIME, uint64(m_NextMonthlyQuestReset));
}

void World::InitRandomBGResetTime()
{
    time_t wstime = time_t(sWorld->getWorldState(WS_BG_DAILY_RESET_TIME));
    m_NextRandomBGReset = wstime ? wstime : GetNextTimeWithDayAndHour(-1, 6);
    if (!wstime)
        sWorld->setWorldState(WS_BG_DAILY_RESET_TIME, uint64(m_NextRandomBGReset));
}

void World::InitCalendarOldEventsDeletionTime()
{
    time_t now = GameTime::GetGameTime();
    time_t currentDeletionTime = getWorldState(WS_DAILY_CALENDAR_DELETION_OLD_EVENTS_TIME);
    time_t nextDeletionTime = currentDeletionTime ? currentDeletionTime : GetNextTimeWithDayAndHour(-1, CONF_GET_INT("Calendar.DeleteOldEventsHour"));

    // If the reset time saved in the worldstate is before now it means the server was offline when the reset was supposed to occur.
    // In this case we set the reset time in the past and next world update will do the reset and schedule next one in the future.
    if (currentDeletionTime < now)
        m_NextCalendarOldEventsDeletionTime = nextDeletionTime - DAY;
    else
        m_NextCalendarOldEventsDeletionTime = nextDeletionTime;

    if (!currentDeletionTime)
        sWorld->setWorldState(WS_DAILY_CALENDAR_DELETION_OLD_EVENTS_TIME, uint64(m_NextCalendarOldEventsDeletionTime));
}

void World::InitGuildResetTime()
{
    time_t wstime = time_t(getWorldState(WS_GUILD_DAILY_RESET_TIME));
    m_NextGuildReset = wstime ? wstime : GetNextTimeWithDayAndHour(-1, 6);
    if (!wstime)
        sWorld->setWorldState(WS_GUILD_DAILY_RESET_TIME, uint64(m_NextGuildReset));
}

void World::ResetDailyQuests()
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_QUEST_STATUS_DAILY);
    CharacterDatabase.Execute(stmt);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetDailyQuestStatus();

    m_NextDailyQuestReset = GetNextTimeWithDayAndHour(-1, 6);
    sWorld->setWorldState(WS_DAILY_QUEST_RESET_TIME, uint64(m_NextDailyQuestReset));

    // change available dailies
    sPoolMgr->ChangeDailyQuests();
}

void World::LoadDBAllowedSecurityLevel()
{
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_REALMLIST_SECURITY_LEVEL);
    stmt->setInt32(0, int32(realm.Id.Realm));
    PreparedQueryResult result = LoginDatabase.Query(stmt);

    if (result)
        SetPlayerSecurityLimit(AccountTypes(result->Fetch()->GetUInt8()));
}

void World::SetPlayerSecurityLimit(AccountTypes _sec)
{
    AccountTypes sec = _sec < SEC_CONSOLE ? _sec : SEC_PLAYER;
    bool update = sec > m_allowedSecurityLevel;
    m_allowedSecurityLevel = sec;
    if (update)
        KickAllLess(m_allowedSecurityLevel);
}

void World::ResetWeeklyQuests()
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_QUEST_STATUS_WEEKLY);
    CharacterDatabase.Execute(stmt);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetWeeklyQuestStatus();

    m_NextWeeklyQuestReset = GetNextTimeWithDayAndHour(4, 6);
    sWorld->setWorldState(WS_WEEKLY_QUEST_RESET_TIME, uint64(m_NextWeeklyQuestReset));

    // change available weeklies
    sPoolMgr->ChangeWeeklyQuests();
}

void World::ResetMonthlyQuests()
{
    LOG_INFO("server", "Monthly quests reset for all characters.");

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_QUEST_STATUS_MONTHLY);
    CharacterDatabase.Execute(stmt);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetMonthlyQuestStatus();

    m_NextMonthlyQuestReset = GetNextTimeWithMonthAndHour(-1, 6);
    sWorld->setWorldState(WS_MONTHLY_QUEST_RESET_TIME, uint64(m_NextMonthlyQuestReset));
}

void World::ResetEventSeasonalQuests(uint16 event_id)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_QUEST_STATUS_SEASONAL);
    stmt->setUInt16(0, event_id);
    CharacterDatabase.Execute(stmt);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetSeasonalQuestStatus(event_id);
}

void World::ResetRandomBG()
{
#if defined(ENABLE_EXTRAS) && defined(ENABLE_EXTRA_LOGS)
    LOG_DEBUG("server", "Random BG status reset for all characters.");
#endif

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_BATTLEGROUND_RANDOM);
    CharacterDatabase.Execute(stmt);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->SetRandomWinner(false);

    m_NextRandomBGReset = GetNextTimeWithDayAndHour(-1, 6);
    sWorld->setWorldState(WS_BG_DAILY_RESET_TIME, uint64(m_NextRandomBGReset));
}

void World::CalendarDeleteOldEvents()
{
    LOG_INFO("server", "Calendar deletion of old events.");

    m_NextCalendarOldEventsDeletionTime = time_t(m_NextCalendarOldEventsDeletionTime + DAY);
    sWorld->setWorldState(WS_DAILY_CALENDAR_DELETION_OLD_EVENTS_TIME, uint64(m_NextCalendarOldEventsDeletionTime));
    sCalendarMgr->DeleteOldEvents();
}

void World::ResetGuildCap()
{
    LOG_INFO("server", "Guild Daily Cap reset.");

    m_NextGuildReset = GetNextTimeWithDayAndHour(-1, 6);
    sWorld->setWorldState(WS_GUILD_DAILY_RESET_TIME, uint64(m_NextGuildReset));

    sGuildMgr->ResetTimes();
}

void World::UpdateMaxSessionCounters()
{
    m_maxActiveSessionCount = std::max(m_maxActiveSessionCount, uint32(m_sessions.size() - m_QueuedPlayer.size()));
    m_maxQueuedSessionCount = std::max(m_maxQueuedSessionCount, uint32(m_QueuedPlayer.size()));
}

void World::LoadDBVersion()
{
    QueryResult result = WorldDatabase.Query("SELECT db_version, cache_id FROM version LIMIT 1");
    if (result)
    {
        Field* fields = result->Fetch();

        m_DBVersion = fields[0].GetString();

        // will be overwrite by config values if different and non-0
        sGameConfig->AddOption<int32>("ClientCacheVersion", fields[1].GetUInt32());
    }

    if (m_DBVersion.empty())
        m_DBVersion = "Unknown world database.";
}

void World::LoadDBRevision()
{
    QueryResult resultWorld     = WorldDatabase.Query("SELECT date FROM version_db_world ORDER BY date DESC LIMIT 1");
    QueryResult resultCharacter = CharacterDatabase.Query("SELECT date FROM version_db_characters ORDER BY date DESC LIMIT 1");
    QueryResult resultAuth      = LoginDatabase.Query("SELECT date FROM version_db_auth ORDER BY date DESC LIMIT 1");

    if (resultWorld)
    {
        Field* fields = resultWorld->Fetch();

        m_WorldDBRevision = fields[0].GetString();
    }
    if (resultCharacter)
    {
        Field* fields = resultCharacter->Fetch();

        m_CharacterDBRevision = fields[0].GetString();
    }
    if (resultAuth)
    {
        Field* fields = resultAuth->Fetch();

        m_AuthDBRevision = fields[0].GetString();
    }

    if (m_WorldDBRevision.empty())
    {
        m_WorldDBRevision = "Unkown World Database Revision";
    }
    if (m_CharacterDBRevision.empty())
    {
        m_CharacterDBRevision = "Unkown Character Database Revision";
    }
    if (m_AuthDBRevision.empty())
    {
        m_AuthDBRevision = "Unkown Auth Database Revision";
    }
}

void World::UpdateAreaDependentAuras()
{
    SessionMap::const_iterator itr;
    for (itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second && itr->second->GetPlayer() && itr->second->GetPlayer()->IsInWorld())
        {
            itr->second->GetPlayer()->UpdateAreaDependentAuras(itr->second->GetPlayer()->GetAreaId());
            itr->second->GetPlayer()->UpdateZoneDependentAuras(itr->second->GetPlayer()->GetZoneId());
        }
}

void World::LoadWorldStates()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = CharacterDatabase.Query("SELECT entry, value FROM worldstates");

    if (!result)
    {
        LOG_INFO("server", ">> Loaded 0 world states. DB table `worldstates` is empty!");
        LOG_INFO("server", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        m_worldstates[fields[0].GetUInt32()] = fields[1].GetUInt32();
        ++count;
    } while (result->NextRow());

    LOG_INFO("server", ">> Loaded %u world states in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server", " ");
}

// Setting a worldstate will save it to DB
void World::setWorldState(uint32 index, uint64 value)
{
    WorldStatesMap::const_iterator it = m_worldstates.find(index);
    if (it != m_worldstates.end())
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_WORLDSTATE);

        stmt->setUInt32(0, uint32(value));
        stmt->setUInt32(1, index);

        CharacterDatabase.Execute(stmt);
    }
    else
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_WORLDSTATE);

        stmt->setUInt32(0, index);
        stmt->setUInt32(1, uint32(value));

        CharacterDatabase.Execute(stmt);
    }
    m_worldstates[index] = value;
}

uint64 World::getWorldState(uint32 index) const
{
    WorldStatesMap::const_iterator it = m_worldstates.find(index);
    return it != m_worldstates.end() ? it->second : 0;
}

void World::ProcessQueryCallbacks()
{
    _queryProcessor.ProcessReadyCallbacks();
}

void World::LoadGlobalPlayerDataStore()
{
    uint32 oldMSTime = getMSTime();

    _globalPlayerDataStore.clear();
    QueryResult result = CharacterDatabase.Query("SELECT guid, account, name, gender, race, class, level FROM characters WHERE deleteDate IS NULL");
    if (!result)
    {
        LOG_INFO("server", ">> Loaded 0 Players data.");
        return;
    }

    uint32 count = 0;

    // query to load number of mails by receiver
    std::map<uint32, uint16> _mailCountMap;
    QueryResult mailCountResult = CharacterDatabase.Query("SELECT receiver, COUNT(receiver) FROM mail GROUP BY receiver");
    if (mailCountResult)
    {
        do
        {
            Field* fields = mailCountResult->Fetch();
            _mailCountMap[fields[0].GetUInt32()] = uint16(fields[1].GetUInt64());
        } while (mailCountResult->NextRow());
    }

    do
    {
        Field* fields = result->Fetch();
        ObjectGuid::LowType guidLow = fields[0].GetUInt32();

        // count mails
        uint16 mailCount = 0;
        std::map<uint32, uint16>::const_iterator itr = _mailCountMap.find(guidLow);
        if (itr != _mailCountMap.end())
            mailCount = itr->second;

        AddGlobalPlayerData(
            guidLow,               /*guid*/
            fields[1].GetUInt32(), /*accountId*/
            fields[2].GetString(), /*name*/
            fields[3].GetUInt8(),  /*gender*/
            fields[4].GetUInt8(),  /*race*/
            fields[5].GetUInt8(),  /*class*/
            fields[6].GetUInt8(),  /*level*/
            mailCount,             /*mail count*/
            0                      /*guild id*/);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server", ">> Loaded %d Players data in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server", " ");
}

void World::AddGlobalPlayerData(ObjectGuid::LowType guid, uint32 accountId, std::string const& name, uint8 gender, uint8 race, uint8 playerClass, uint8 level, uint16 mailCount, uint32 guildId)
{
    GlobalPlayerData data;

    data.guidLow = guid;
    data.accountId = accountId;
    data.name = name;
    data.level = level;
    data.race = race;
    data.playerClass = playerClass;
    data.gender = gender;
    data.mailCount = mailCount;
    data.guildId = guildId;
    data.groupId = 0;
    data.arenaTeamId[0] = 0;
    data.arenaTeamId[1] = 0;
    data.arenaTeamId[2] = 0;

    _globalPlayerDataStore[guid] = data;
    _globalPlayerNameStore[name] = guid;
}

void World::UpdateGlobalPlayerData(ObjectGuid::LowType guid, uint8 mask, std::string const& name, uint8 level, uint8 gender, uint8 race, uint8 playerClass)
{
    GlobalPlayerDataMap::iterator itr = _globalPlayerDataStore.find(guid);
    if (itr == _globalPlayerDataStore.end())
        return;

    if (mask & PLAYER_UPDATE_DATA_LEVEL)
        itr->second.level = level;
    if (mask & PLAYER_UPDATE_DATA_RACE)
        itr->second.race = race;
    if (mask & PLAYER_UPDATE_DATA_CLASS)
        itr->second.playerClass = playerClass;
    if (mask & PLAYER_UPDATE_DATA_GENDER)
        itr->second.gender = gender;
    if (mask & PLAYER_UPDATE_DATA_NAME)
        itr->second.name = name;

    WorldPacket data(SMSG_INVALIDATE_PLAYER, 8);
    data << guid;
    SendGlobalMessage(&data);
}

void World::UpdateGlobalPlayerMails(ObjectGuid::LowType guid, int16 count, bool add)
{
    GlobalPlayerDataMap::iterator itr = _globalPlayerDataStore.find(guid);
    if (itr == _globalPlayerDataStore.end())
        return;

    if (!add)
    {
        itr->second.mailCount = count;
        return;
    }

    int16 icount = (int16)itr->second.mailCount;
    if (count < 0 && abs(count) > icount)
        count = -icount;
    itr->second.mailCount = uint16(icount + count); // addition or subtraction
}

void World::UpdateGlobalPlayerGuild(ObjectGuid::LowType guid, uint32 guildId)
{
    GlobalPlayerDataMap::iterator itr = _globalPlayerDataStore.find(guid);
    if (itr == _globalPlayerDataStore.end())
        return;

    itr->second.guildId = guildId;
}
void World::UpdateGlobalPlayerGroup(ObjectGuid::LowType guid, uint32 groupId)
{
    GlobalPlayerDataMap::iterator itr = _globalPlayerDataStore.find(guid);
    if (itr == _globalPlayerDataStore.end())
        return;

    itr->second.groupId = groupId;
}

void World::UpdateGlobalPlayerArenaTeam(ObjectGuid::LowType guid, uint8 slot, uint32 arenaTeamId)
{
    GlobalPlayerDataMap::iterator itr = _globalPlayerDataStore.find(guid);
    if (itr == _globalPlayerDataStore.end())
        return;

    itr->second.arenaTeamId[slot] = arenaTeamId;
}

void World::UpdateGlobalNameData(ObjectGuid::LowType guidLow, std::string const& oldName, std::string const& newName)
{
    _globalPlayerNameStore.erase(oldName);
    _globalPlayerNameStore[newName] = guidLow;
}

void World::DeleteGlobalPlayerData(ObjectGuid::LowType guid, std::string const& name)
{
    if (guid)
        _globalPlayerDataStore.erase(guid);
    if (!name.empty())
        _globalPlayerNameStore.erase(name);
}

GlobalPlayerData const* World::GetGlobalPlayerData(ObjectGuid::LowType guid) const
{
    // Get data from global storage
    GlobalPlayerDataMap::const_iterator itr = _globalPlayerDataStore.find(guid);
    if (itr != _globalPlayerDataStore.end())
        return &itr->second;

    // Player is not in the global storage, try to get it from the Database
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_DATA_BY_GUID);

    stmt->setUInt32(0, guid);

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        // Player was not in the global storage, but it was found in the database
        // Let's add it to the global storage
        Field* fields = result->Fetch();

        std::string name = fields[2].GetString();

        LOG_INFO("server", "Player %s [GUID: %u] was not found in the global storage, but it was found in the database.", name.c_str(), guid);

        sWorld->AddGlobalPlayerData(
            fields[0].GetUInt32(), /*guid*/
            fields[1].GetUInt32(), /*accountId*/
            fields[2].GetString(), /*name*/
            fields[3].GetUInt8(),  /*gender*/
            fields[4].GetUInt8(),  /*race*/
            fields[5].GetUInt8(),  /*class*/
            fields[6].GetUInt8(),  /*level*/
            0,                     /*mail count*/
            0                      /*guild id*/
        );

        itr = _globalPlayerDataStore.find(guid);
        if (itr != _globalPlayerDataStore.end())
        {
            LOG_INFO("server", "Player %s [GUID: %u] added to the global storage.", name.c_str(), guid);
            return &itr->second;
        }
    }

    // Player not found
    return nullptr;
}

ObjectGuid World::GetGlobalPlayerGUID(std::string const& name) const
{
    // Get data from global storage
    GlobalPlayerNameMap::const_iterator itr = _globalPlayerNameStore.find(name);
    if (itr != _globalPlayerNameStore.end())
        return ObjectGuid::Create<HighGuid::Player>(itr->second);

    // Player is not in the global storage, try to get it from the Database
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_DATA_BY_NAME);

    stmt->setString(0, name);

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
    {
        // Player was not in the global storage, but it was found in the database
        // Let's add it to the global storage
        Field* fields = result->Fetch();

        ObjectGuid::LowType guidLow = fields[0].GetUInt32();

        LOG_INFO("server", "Player %s [GUID: %u] was not found in the global storage, but it was found in the database.", name.c_str(), guidLow);

        sWorld->AddGlobalPlayerData(
            guidLow,               /*guid*/
            fields[1].GetUInt32(), /*accountId*/
            fields[2].GetString(), /*name*/
            fields[3].GetUInt8(),  /*gender*/
            fields[4].GetUInt8(),  /*race*/
            fields[5].GetUInt8(),  /*class*/
            fields[6].GetUInt8(),  /*level*/
            0,                     /*mail count*/
            0                      /*guild id*/
        );

        itr = _globalPlayerNameStore.find(name);
        if (itr != _globalPlayerNameStore.end())
        {
            LOG_INFO("server", "Player %s [GUID: %u] added to the global storage.", name.c_str(), guidLow);

            return ObjectGuid::Create<HighGuid::Player>(guidLow);
        }
    }

    // Player not found
    return ObjectGuid::Empty;
}

void World::RemoveOldCorpses()
{
    m_timers[WUPDATE_CORPSES].SetCurrent(m_timers[WUPDATE_CORPSES].GetInterval());
}

bool World::IsPvPRealm() const
{
    return CONF_GET_INT("GameType") == REALM_TYPE_PVP || CONF_GET_INT("GameType") == REALM_TYPE_RPPVP || CONF_GET_INT("GameType") == REALM_TYPE_FFA_PVP;
}

bool World::IsFFAPvPRealm() const
{
    return CONF_GET_INT("GameType") == REALM_TYPE_FFA_PVP;
}

uint32 World::GetNextWhoListUpdateDelaySecs()
{
    if (m_timers[WUPDATE_5_SECS].Passed())
        return 1;

    uint32 t = m_timers[WUPDATE_5_SECS].GetInterval() - m_timers[WUPDATE_5_SECS].GetCurrent();
    t = std::min(t, (uint32)m_timers[WUPDATE_5_SECS].GetInterval());

    return uint32(ceil(t / 1000.0f));
}

uint16 World::GetConfigMaxSkillValue() const
{
    uint16 lvl = uint16(CONF_GET_INT("MaxPlayerLevel"));
    return lvl > 60 ? 300 + ((lvl - 60) * 75) / 10 : lvl * 5;
}

CliCommandHolder::CliCommandHolder(void* callbackArg, char const* command, Print zprint, CommandFinished commandFinished)
    : m_callbackArg(callbackArg), m_command(strdup(command)), m_print(zprint), m_commandFinished(commandFinished)
{
}

CliCommandHolder::~CliCommandHolder()
{
    free(m_command);
}
