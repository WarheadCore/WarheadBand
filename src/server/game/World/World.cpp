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

/** \file
    \ingroup world
*/

#include "World.h"
#include "AccountMgr.h"
#include "AchievementMgr.h"
#include "AddonMgr.h"
#include "ArenaTeamMgr.h"
#include "AsyncAuctionListing.h"
#include "AuctionHouseMgr.h"
#include "Autobroadcast.h"
#include "BattlefieldMgr.h"
#include "BattlegroundMgr.h"
#include "CalendarMgr.h"
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
#include "ExternalMail.h"
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
#include "Log.h"
#include "LootItemStorage.h"
#include "LootMgr.h"
#include "MMapFactory.h"
#include "MapMgr.h"
#include "Metric.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "OutdoorPvPMgr.h"
#include "PetitionMgr.h"
#include "Player.h"
#include "PoolMgr.h"
#include "Realm.h"
#include "ScriptMgr.h"
#include "ServerMotd.h"
#include "SkillDiscovery.h"
#include "SkillExtraItems.h"
#include "SmartAI.h"
#include "SpellMgr.h"
#include "TextBuilder.h"
#include "TicketMgr.h"
#include "Timer.h"
#include "Tokenize.h"
#include "Transport.h"
#include "TransportMgr.h"
#include "UpdateTime.h"
#include "Util.h"
#include "VMapFactory.h"
#include "VMapMgr2.h"
#include "Vehicle.h"
#include "Warden.h"
#include "WardenCheckMgr.h"
#include "WaypointMovementGenerator.h"
#include "WeatherMgr.h"
#include "WhoListCacheMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "TaskScheduler.h"
#include <boost/asio/ip/address.hpp>
#include <cmath>

namespace
{
    TaskScheduler playersSaveScheduler;
}

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
    m_NextDailyQuestReset = 0s;
    m_NextWeeklyQuestReset = 0s;
    m_NextMonthlyQuestReset = 0s;
    m_NextRandomBGReset = 0s;
    m_NextCalendarOldEventsDeletionTime = 0s;
    m_NextGuildReset = 0s;
    m_defaultDbcLocale = LOCALE_enUS;
    mail_expire_check_timer = 0s;
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
            m_disconnects[s->GetAccountId()] = GameTime::GetGameTime().count();

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
            oldSession->SetOfflineTime(GameTime::GetGameTime().count());
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

    FinalizePlayerWorldSession(s);

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
            if ((GameTime::GetGameTime().count() - i->second) < tolerance)
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
        pop_sess->SendAuthWaitQueue(0);
        pop_sess->SendAccountDataTimes(GLOBAL_CACHE_MASK);

        FinalizePlayerWorldSession(pop_sess);

        m_QueuedPlayer.pop_front();

        // update iter to point first queued socket or end() if queue is empty now
        iter = m_QueuedPlayer.begin();
        position = 1;
    }

    // update queue position from iter to end()
    for (; iter != m_QueuedPlayer.end(); ++iter, ++position)
        (*iter)->SendAuthWaitQueue(position);

    return found;
}

/// Initialize config values
void World::LoadConfigSettings(bool reload)
{
    if (reload)
    {
        if (!sConfigMgr->Reload())
        {
            LOG_ERROR("server.loading", "World settings reload fail: can't read settings.");
            return;
        }

        sLog->LoadFromConfig();
        sMetric->LoadFromConfigs();
    }

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
        LOG_ERROR("server.loading", "Visibility.Distance.Continents can't be less max aggro radius {}", 45 * rateAggro);
        m_MaxVisibleDistanceOnContinents = 45 * rateAggro;
    }
    else if (m_MaxVisibleDistanceOnContinents > MAX_VISIBILITY_DISTANCE)
    {
        LOG_ERROR("server.loading", "Visibility.Distance.Continents can't be greater {}", MAX_VISIBILITY_DISTANCE);
        m_MaxVisibleDistanceOnContinents = MAX_VISIBILITY_DISTANCE;
    }

    // Visibility in instances
    m_MaxVisibleDistanceInInstances = CONF_GET_FLOAT("Visibility.Distance.Instances");
    if (m_MaxVisibleDistanceInInstances < 45 * rateAggro)
    {
        LOG_ERROR("server.loading", "Visibility.Distance.Instances can't be less max aggro radius {}", 45 * rateAggro);
        m_MaxVisibleDistanceInInstances = 45 * rateAggro;
    }
    else if (m_MaxVisibleDistanceInInstances > MAX_VISIBILITY_DISTANCE)
    {
        LOG_ERROR("server.loading", "Visibility.Distance.Instances can't be greater {}", MAX_VISIBILITY_DISTANCE);
        m_MaxVisibleDistanceInInstances = MAX_VISIBILITY_DISTANCE;
    }

    // Visibility in BG/Arenas
    m_MaxVisibleDistanceInBGArenas = CONF_GET_FLOAT("Visibility.Distance.BGArenas");
    if (m_MaxVisibleDistanceInBGArenas < 45 * rateAggro)
    {
        LOG_ERROR("server.loading", "Visibility.Distance.BGArenas can't be less max aggro radius {}", 45 * rateAggro);
        m_MaxVisibleDistanceInBGArenas = 45 * rateAggro;
    }
    else if (m_MaxVisibleDistanceInBGArenas > MAX_VISIBILITY_DISTANCE)
    {
        LOG_ERROR("server.loading", "Visibility.Distance.BGArenas can't be greater {}", MAX_VISIBILITY_DISTANCE);
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
        LOG_ERROR("server.loading", "DataDir option can't be changed at worldserver.conf reload, using current value ({}).", m_dataPath);
    else
        m_dataPath = dataPath;

    bool enableIndoor = CONF_GET_BOOL("vmap.enableIndoorCheck");
    bool enableLOS = CONF_GET_BOOL("vmap.enableLOS");
    bool enableHeight = CONF_GET_BOOL("vmap.enableHeight");
    bool enablePetLOS = CONF_GET_BOOL("vmap.petLOS");

    if (!enableHeight)
        LOG_ERROR("server.loading", "VMap height checking disabled! Creatures movements and other various things WILL be broken! Expect no support.");

    VMAP::VMapFactory::createOrGetVMapMgr()->setEnableLineOfSightCalc(enableLOS);
    VMAP::VMapFactory::createOrGetVMapMgr()->setEnableHeightCalc(enableHeight);

    if (!reload)
    {
        auto VMAPBoolToString = [](bool value)
        {
            return value ? "Enable" : "Disable";
        };

        LOG_INFO("server.loading", "Loading data configurations...");
        LOG_INFO("server.loading", "> Using DataDir:        {}", m_dataPath);
        LOG_INFO("server.loading", "");
        LOG_INFO("server.loading", "Loading VMap configurations...");
        LOG_INFO("server.loading", "> Line Of Sight:        {}", VMAPBoolToString(enableLOS));
        LOG_INFO("server.loading", "> Get Height:           {}", VMAPBoolToString(enableHeight));
        LOG_INFO("server.loading", "> Indoor Check:         {}", VMAPBoolToString(enableIndoor));
        LOG_INFO("server.loading", "> Pet LOS:              {}", VMAPBoolToString(enablePetLOS));
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
    srand((unsigned int)GameTime::GetGameTime().count());

    ///- Initialize detour memory management
    dtAllocSetCustom(dtCustomAlloc, dtCustomFree);

    ///- Initialize VMapMgr function pointers (to untangle game/collision circular deps)
    VMAP::VMapMgr2* vmmgr2 = VMAP::VMapFactory::createOrGetVMapMgr();
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
        if (!MapMgr::ExistMapAndVMap(0, -6240.32f, 331.033f)
                || !MapMgr::ExistMapAndVMap(0, -8949.95f, -132.493f)
                || !MapMgr::ExistMapAndVMap(1, -618.518f, -4251.67f)
                || !MapMgr::ExistMapAndVMap(0, 1676.35f, 1677.45f)
                || !MapMgr::ExistMapAndVMap(1, 10311.3f, 832.463f)
                || !MapMgr::ExistMapAndVMap(1, -2917.58f, -257.98f)
                || (CONF_GET_INT("Expansion") && (
                        !MapMgr::ExistMapAndVMap(530, 10349.6f, -6357.29f) ||
                        !MapMgr::ExistMapAndVMap(530, -3961.64f, -13931.2f))))
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

    LoginDatabase.PExecute("UPDATE realmlist SET icon = {}, timezone = {} WHERE id = '{}'", server_type, realm_zone, realm.Id.Realm);      // One-time query

    ///- Custom Hook for loading DB items
    sScriptMgr->OnLoadCustomDatabaseTable();

    ///- Load the DBC files
    LOG_INFO("server.loading", "Initialize data stores...");
    LoadDBCStores(m_dataPath);
    DetectDBCLang();

    // Load IP Location Database
    sIPLocation->Load();

    std::vector<uint32> mapIds;
    for (auto const map : sMapStore)
    {
        mapIds.emplace_back(map->MapID);
    }

    vmmgr2->InitializeThreadUnsafe(mapIds);

    MMAP::MMapMgr* mmmgr = MMAP::MMapFactory::createOrGetMMapMgr();
    mmmgr->InitializeThreadUnsafe(mapIds);

    LOG_INFO("server.loading", "Loading Game Graveyard...");
    sGraveyard->LoadGraveyardFromDB();

    LOG_INFO("server.loading", "Loading spell dbc data corrections...");
    sSpellMgr->LoadDbcDataCorrections();

    LOG_INFO("server.loading", "Loading SpellInfo store...");
    sSpellMgr->LoadSpellInfoStore();

    LOG_INFO("server.loading", "Loading Spell Rank Data...");
    sSpellMgr->LoadSpellRanks();

    LOG_INFO("server.loading", "Loading Spell Specific And Aura State...");
    sSpellMgr->LoadSpellSpecificAndAuraState();

    LOG_INFO("server.loading", "Loading SkillLineAbilityMultiMap Data...");
    sSpellMgr->LoadSkillLineAbilityMap();

    LOG_INFO("server.loading", "Loading spell custom attributes...");
    sSpellMgr->LoadSpellCustomAttr();

    LOG_INFO("server.loading", "Loading GameObject models...");
    LoadGameObjectModelList(m_dataPath);

    LOG_INFO("server.loading", "Loading Script Names...");
    sObjectMgr->LoadScriptNames();

    LOG_INFO("server.loading", "Loading Instance Template...");
    sObjectMgr->LoadInstanceTemplate();

    LOG_INFO("server.loading", "Load Character Cache...");
    sCharacterCache->LoadCharacterCacheStorage();

    // Must be called before `creature_respawn`/`gameobject_respawn` tables
    LOG_INFO("server.loading", "Loading instances...");
    sInstanceSaveMgr->LoadInstances();

    LOG_INFO("server.loading", "Loading Game locale texts...");
    sGameLocale->LoadAllLocales();

    LOG_INFO("server", "Loading Page Texts...");
    sObjectMgr->LoadPageTexts();

    LOG_INFO("server.loading", "Loading Game Object Templates...");         // must be after LoadPageTexts
    sObjectMgr->LoadGameObjectTemplate();

    LOG_INFO("server.loading", "Loading Game Object template addons...");
    sObjectMgr->LoadGameObjectTemplateAddons();

    LOG_INFO("server.loading", "Loading Transport templates...");
    sTransportMgr->LoadTransportTemplates();

    LOG_INFO("server.loading", "Loading Spell Required Data...");
    sSpellMgr->LoadSpellRequired();

    LOG_INFO("server.loading", "Loading Spell Group types...");
    sSpellMgr->LoadSpellGroups();

    LOG_INFO("server.loading", "Loading Spell Learn Skills...");
    sSpellMgr->LoadSpellLearnSkills();                           // must be after LoadSpellRanks

    LOG_INFO("server.loading", "Loading Spell Proc Event conditions...");
    sSpellMgr->LoadSpellProcEvents();

    LOG_INFO("server.loading", "Loading Spell Proc conditions and data...");
    sSpellMgr->LoadSpellProcs();

    LOG_INFO("server.loading", "Loading Spell Bonus Data...");
    sSpellMgr->LoadSpellBonusess();

    LOG_INFO("server.loading", "Loading Aggro Spells Definitions...");
    sSpellMgr->LoadSpellThreats();

    LOG_INFO("server.loading", "Loading Mixology bonuses...");
    sSpellMgr->LoadSpellMixology();

    LOG_INFO("server.loading", "Loading Spell Group Stack Rules...");
    sSpellMgr->LoadSpellGroupStackRules();

    LOG_INFO("server.loading", "Loading NPC Texts...");
    sObjectMgr->LoadGossipText();

    LOG_INFO("server.loading", "Loading Enchant Spells Proc datas...");
    sSpellMgr->LoadSpellEnchantProcData();

    LOG_INFO("server.loading", "Loading Item Random Enchantments Table...");
    LoadRandomEnchantmentsTable();

    LOG_INFO("server.loading", "Loading Disables");
    DisableMgr::LoadDisables();                                  // must be before loading quests and items

    LOG_INFO("server.loading", "Loading Items...");                         // must be after LoadRandomEnchantmentsTable and LoadPageTexts
    sObjectMgr->LoadItemTemplates();

    LOG_INFO("server.loading", "Loading Item set names...");                // must be after LoadItemPrototypes
    sObjectMgr->LoadItemSetNames();

    LOG_INFO("server.loading", "Loading Creature Model Based Info Data...");
    sObjectMgr->LoadCreatureModelInfo();

    LOG_INFO("server.loading", "Loading Creature templates...");
    sObjectMgr->LoadCreatureTemplates();

    LOG_INFO("server.loading", "Loading Equipment templates...");           // must be after LoadCreatureTemplates
    sObjectMgr->LoadEquipmentTemplates();

    LOG_INFO("server.loading", "Loading Creature template addons...");
    sObjectMgr->LoadCreatureTemplateAddons();

    LOG_INFO("server.loading", "Loading Reputation Reward Rates...");
    sObjectMgr->LoadReputationRewardRate();

    LOG_INFO("server.loading", "Loading Creature Reputation OnKill Data...");
    sObjectMgr->LoadReputationOnKill();

    LOG_INFO("server.loading", "Loading Reputation Spillover Data..." );
    sObjectMgr->LoadReputationSpilloverTemplate();

    LOG_INFO("server.loading", "Loading Points Of Interest Data...");
    sObjectMgr->LoadPointsOfInterest();

    LOG_INFO("server.loading", "Loading Creature Base Stats...");
    sObjectMgr->LoadCreatureClassLevelStats();

    LOG_INFO("server.loading", "Loading Creature Data...");
    sObjectMgr->LoadCreatures();

    LOG_INFO("server.loading", "Loading Temporary Summon Data...");
    sObjectMgr->LoadTempSummons();                               // must be after LoadCreatureTemplates() and LoadGameObjectTemplates()

    LOG_INFO("server.loading", "Loading pet levelup spells...");
    sSpellMgr->LoadPetLevelupSpellMap();

    LOG_INFO("server.loading", "Loading pet default spells additional to levelup spells...");
    sSpellMgr->LoadPetDefaultSpells();

    LOG_INFO("server.loading", "Loading Creature Addon Data...");
    sObjectMgr->LoadCreatureAddons();                            // must be after LoadCreatureTemplates() and LoadCreatures()

    LOG_INFO("server.loading", "Loading Gameobject Data...");
    sObjectMgr->LoadGameobjects();

    LOG_INFO("server.loading", "Loading GameObject Addon Data...");
    sObjectMgr->LoadGameObjectAddons();                          // must be after LoadGameObjectTemplate() and LoadGameobjects()

    LOG_INFO("server.loading", "Loading GameObject Quest Items...");
    sObjectMgr->LoadGameObjectQuestItems();

    LOG_INFO("server.loading", "Loading Creature Quest Items...");
    sObjectMgr->LoadCreatureQuestItems();

    LOG_INFO("server.loading", "Loading Creature Linked Respawn...");
    sObjectMgr->LoadLinkedRespawn();                             // must be after LoadCreatures(), LoadGameObjects()

    LOG_INFO("server.loading", "Loading Weather Data...");
    WeatherMgr::LoadWeatherData();

    LOG_INFO("server.loading", "Loading Quests...");
    sObjectMgr->LoadQuests();                                    // must be loaded after DBCs, creature_template, item_template, gameobject tables

    LOG_INFO("server.loading", "Checking Quest Disables");
    DisableMgr::CheckQuestDisables();                           // must be after loading quests

    LOG_INFO("server.loading", "Loading Quest POI");
    sObjectMgr->LoadQuestPOI();

    // Loading Quests Starters and Enders
    sObjectMgr->LoadQuestStartersAndEnders();                    // must be after quest load

    LOG_INFO("server.loading", "Loading Quest Money Rewards...");
    sObjectMgr->LoadQuestMoneyRewards();

    LOG_INFO("server.loading", "Loading Objects Pooling Data...");
    sPoolMgr->LoadFromDB();

    LOG_INFO("server.loading", "Loading Game Event Data...");               // must be after loading pools fully
    sGameEventMgr->LoadHolidayDates();                           // Must be after loading DBC
    sGameEventMgr->LoadFromDB();                                 // Must be after loading holiday dates

    LOG_INFO("server.loading", "Loading UNIT_NPC_FLAG_SPELLCLICK Data..."); // must be after LoadQuests
    sObjectMgr->LoadNPCSpellClickSpells();

    LOG_INFO("server.loading", "Loading Vehicle Template Accessories...");
    sObjectMgr->LoadVehicleTemplateAccessories();                // must be after LoadCreatureTemplates() and LoadNPCSpellClickSpells()

    LOG_INFO("server.loading", "Loading Vehicle Accessories...");
    sObjectMgr->LoadVehicleAccessories();                       // must be after LoadCreatureTemplates() and LoadNPCSpellClickSpells()

    LOG_INFO("server.loading", "Loading SpellArea Data...");                // must be after quest load
    sSpellMgr->LoadSpellAreas();

    LOG_INFO("server.loading", "Loading Area Trigger definitions");
    sObjectMgr->LoadAreaTriggers();

    LOG_INFO("server.loading", "Loading Area Trigger Teleport definitions...");
    sObjectMgr->LoadAreaTriggerTeleports();

    LOG_INFO("server.loading", "Loading Access Requirements...");
    sObjectMgr->LoadAccessRequirements();                        // must be after item template load

    LOG_INFO("server.loading", "Loading Quest Area Triggers...");
    sObjectMgr->LoadQuestAreaTriggers();                         // must be after LoadQuests

    LOG_INFO("server.loading", "Loading Tavern Area Triggers...");
    sObjectMgr->LoadTavernAreaTriggers();

    LOG_INFO("server.loading", "Loading AreaTrigger script names...");
    sObjectMgr->LoadAreaTriggerScripts();

    LOG_INFO("server.loading", "Loading LFG entrance positions..."); // Must be after areatriggers
    sLFGMgr->LoadLFGDungeons();

    LOG_INFO("server.loading", "Loading Dungeon boss data...");
    sObjectMgr->LoadInstanceEncounters();

    LOG_INFO("server.loading", "Loading LFG rewards...");
    sLFGMgr->LoadRewards();

    LOG_INFO("server.loading", "Loading Graveyard-zone links...");
    sGraveyard->LoadGraveyardZones();

    LOG_INFO("server.loading", "Loading spell pet auras...");
    sSpellMgr->LoadSpellPetAuras();

    LOG_INFO("server.loading", "Loading Spell target coordinates...");
    sSpellMgr->LoadSpellTargetPositions();

    LOG_INFO("server.loading", "Loading enchant custom attributes...");
    sSpellMgr->LoadEnchantCustomAttr();

    LOG_INFO("server.loading", "Loading linked spells...");
    sSpellMgr->LoadSpellLinked();

    LOG_INFO("server.loading", "Loading Player Create Data...");
    sObjectMgr->LoadPlayerInfo();

    LOG_INFO("server.loading", "Loading Exploration BaseXP Data...");
    sObjectMgr->LoadExplorationBaseXP();

    LOG_INFO("server.loading", "Loading Pet Name Parts...");
    sObjectMgr->LoadPetNames();

    CharacterDatabaseCleaner::CleanDatabase();

    LOG_INFO("server.loading", "Loading the max pet number...");
    sObjectMgr->LoadPetNumber();

    LOG_INFO("server.loading", "Loading pet level stats...");
    sObjectMgr->LoadPetLevelInfo();

    LOG_INFO("server.loading", "Loading Player level dependent mail rewards...");
    sObjectMgr->LoadMailLevelRewards();

    // Loot tables
    LoadLootTables();

    LOG_INFO("server.loading", "Loading Skill Discovery Table...");
    LoadSkillDiscoveryTable();

    LOG_INFO("server.loading", "Loading Skill Extra Item Table...");
    LoadSkillExtraItemTable();

    LOG_INFO("server.loading", "Loading Skill Perfection Data Table...");
    LoadSkillPerfectItemTable();

    LOG_INFO("server.loading", "Loading Skill Fishing base level requirements...");
    sObjectMgr->LoadFishingBaseSkillLevel();

    LOG_INFO("server.loading", "Loading Achievements...");
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
    LOG_INFO("server.loading", "Loading Item Auctions...");
    sAuctionMgr->LoadAuctionItems();

    LOG_INFO("server", "Loading Auctions...");
    sAuctionMgr->LoadAuctions();

    sGuildMgr->LoadGuilds();

    LOG_INFO("server.loading", "Loading ArenaTeams...");
    sArenaTeamMgr->LoadArenaTeams();

    LOG_INFO("server.loading", "Loading Groups...");
    sGroupMgr->LoadGroups();

    LOG_INFO("server.loading", "Loading ReservedNames...");
    sObjectMgr->LoadReservedPlayersNames();

    LOG_INFO("server.loading", "Loading GameObjects for quests...");
    sObjectMgr->LoadGameObjectForQuests();

    LOG_INFO("server.loading", "Loading BattleMasters...");
    sBattlegroundMgr->LoadBattleMastersEntry();

    LOG_INFO("server.loading", "Loading GameTeleports...");
    sObjectMgr->LoadGameTele();

    LOG_INFO("server.loading", "Loading Gossip menu...");
    sObjectMgr->LoadGossipMenu();

    LOG_INFO("server.loading", "Loading Gossip menu options...");
    sObjectMgr->LoadGossipMenuItems();

    LOG_INFO("server.loading", "Loading Vendors...");
    sObjectMgr->LoadVendors();                                   // must be after load CreatureTemplate and ItemTemplate

    LOG_INFO("server.loading", "Loading Trainers...");
    sObjectMgr->LoadTrainerSpell();                              // must be after load CreatureTemplate

    LOG_INFO("server.loading", "Loading Waypoints...");
    sWaypointMgr->Load();

    LOG_INFO("server.loading", "Loading SmartAI Waypoints...");
    sSmartWaypointMgr->LoadFromDB();

    LOG_INFO("server.loading", "Loading Creature Formations...");
    sFormationMgr->LoadCreatureFormations();

    LOG_INFO("server.loading", "Loading World States...");              // must be loaded before battleground, outdoor PvP and conditions
    LoadWorldStates();

    LOG_INFO("server.loading", "Loading Conditions...");
    sConditionMgr->LoadConditions();

    LOG_INFO("server.loading", "Loading faction change achievement pairs...");
    sObjectMgr->LoadFactionChangeAchievements();

    LOG_INFO("server.loading", "Loading faction change spell pairs...");
    sObjectMgr->LoadFactionChangeSpells();

    LOG_INFO("server.loading", "Loading faction change item pairs...");
    sObjectMgr->LoadFactionChangeItems();

    LOG_INFO("server.loading", "Loading faction change reputation pairs...");
    sObjectMgr->LoadFactionChangeReputations();

    LOG_INFO("server.loading", "Loading faction change title pairs...");
    sObjectMgr->LoadFactionChangeTitles();

    LOG_INFO("server.loading", "Loading faction change quest pairs...");
    sObjectMgr->LoadFactionChangeQuests();

    LOG_INFO("server.loading", "Loading GM tickets...");
    sTicketMgr->LoadTickets();

    LOG_INFO("server.loading", "Loading GM surveys...");
    sTicketMgr->LoadSurveys();

    LOG_INFO("server.loading", "Loading client addons...");
    AddonMgr::LoadFromDB();

    // pussywizard:
    LOG_INFO("server.loading", "Deleting invalid mail items...");
    LOG_INFO("server.loading", " ");
    CharacterDatabase.Execute("DELETE mi FROM mail_items mi LEFT JOIN item_instance ii ON mi.item_guid = ii.guid WHERE ii.guid IS NULL");
    CharacterDatabase.Execute("DELETE mi FROM mail_items mi LEFT JOIN mail m ON mi.mail_id = m.id WHERE m.id IS NULL");
    CharacterDatabase.Execute("UPDATE mail m LEFT JOIN mail_items mi ON m.id = mi.mail_id SET m.has_items=0 WHERE m.has_items<>0 AND mi.mail_id IS NULL");

    ///- Handle outdated emails (delete/return)
    LOG_INFO("server.loading", "Returning old mails...");
    LOG_INFO("server.loading", " ");
    sObjectMgr->ReturnOrDeleteOldMails(false);

    ///- Load AutoBroadCast
    LOG_INFO("server.loading", "Loading Autobroadcasts...");
    sAutobroadcastMgr->Load();

    ///- Load and initialize scripts
    sObjectMgr->LoadSpellScripts();                              // must be after load Creature/Gameobject(Template/Data)
    sObjectMgr->LoadEventScripts();                              // must be after load Creature/Gameobject(Template/Data)
    sObjectMgr->LoadWaypointScripts();

    LOG_INFO("server.loading", "Loading spell script names...");
    sObjectMgr->LoadSpellScriptNames();

    LOG_INFO("server.loading", "Loading Creature Texts...");
    sCreatureTextMgr->LoadCreatureTexts();

    LOG_INFO("server.loading", "Loading Creature Text Locales...");
    sCreatureTextMgr->LoadCreatureTextLocales();

    LOG_INFO("server.loading", "Loading Scripts...");
    sScriptMgr->LoadDatabase();

    LOG_INFO("server.loading", "Validating spell scripts...");
    sObjectMgr->ValidateSpellScripts();

    LOG_INFO("server.loading", "Loading SmartAI scripts...");
    sSmartScriptMgr->LoadSmartAIFromDB();

    LOG_INFO("server.loading", "Loading Calendar data...");
    sCalendarMgr->LoadFromDB();

    LOG_INFO("server.loading", "Initializing SpellInfo precomputed data..."); // must be called after loading items, professions, spells and pretty much anything
    LOG_INFO("server.loading", " ");
    sObjectMgr->InitializeSpellInfoPrecomputedData();

    LOG_INFO("server.loading", "Initialize commands...");
    Warhead::ChatCommands::LoadCommandMap();

    ///- Initialize game time and timers
    LOG_INFO("server", "Initialize game time and timers");
    LOG_INFO("server", " ");
    GameTime::UpdateGameTimers();

    LoginDatabase.PExecute("INSERT INTO uptime (realmid, starttime, uptime, revision) VALUES({}, {}, 0, '{}')",
                           realm.Id.Realm, uint32(GameTime::GetStartTime().count()), GitRevision::GetFullVersion()); // One-time query

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

    m_timers[WUPDATE_WHO_LIST].SetInterval(5 * IN_MILLISECONDS); // update who list cache every 5 seconds

    mail_expire_check_timer = GameTime::GetGameTime() + 6h;

    ///- Initilize static helper structures
    AIRegistry::Initialize();

    ///- Initialize MapMgr
    LOG_INFO("server.loading", "Starting Map System");
    LOG_INFO("server.loading", " ");
    sMapMgr->Initialize();

    LOG_INFO("server.loading", "Starting Game Event system...");
    LOG_INFO("server.loading", " ");
    uint32 nextGameEvent = sGameEventMgr->StartSystem();
    m_timers[WUPDATE_EVENTS].SetInterval(nextGameEvent);    //depend on next event

    // Delete all characters which have been deleted X days before
    Player::DeleteOldCharacters();

    // Delete all custom channels which haven't been used for PreserveCustomChannelDuration days.
    Channel::CleanOldChannelsInDB();

    LOG_INFO("server.loading", "Initializing Opcodes...");
    opcodeTable.Initialize();

    LOG_INFO("server.loading", "Starting Arena Season...");
    LOG_INFO("server.loading", " ");
    sGameEventMgr->StartArenaSeason();

    sTicketMgr->Initialize();

    ///- Initialize Battlegrounds
    LOG_INFO("server.loading", "Starting Battleground System");
    sBattlegroundMgr->CreateInitialBattlegrounds();
    sBattlegroundMgr->InitAutomaticArenaPointDistribution();

    ///- Initialize outdoor pvp
    LOG_INFO("server.loading", "Starting Outdoor PvP System");
    sOutdoorPvPMgr->InitOutdoorPvP();

    ///- Initialize Battlefield
    LOG_INFO("server.loading", "Starting Battlefield System");
    sBattlefieldMgr->InitBattlefield();

    LOG_INFO("server.loading", "Loading Transports...");
    sTransportMgr->SpawnContinentTransports();

    ///- Initialize Warden
    LOG_INFO("server.loading", "Loading Warden Checks..." );
    sWardenCheckMgr->LoadWardenChecks();

    LOG_INFO("server.loading", "Loading Warden Action Overrides..." );
    sWardenCheckMgr->LoadWardenOverrides();

    LOG_INFO("server.loading", "Deleting expired bans...");
    LoginDatabase.Execute("DELETE FROM ip_banned WHERE unbandate <= UNIX_TIMESTAMP() AND unbandate<>bandate");      // One-time query

    LOG_INFO("server.loading", "Calculate next daily quest reset time...");
    InitDailyQuestResetTime();

    LOG_INFO("server.loading", "Calculate next weekly quest reset time..." );
    InitWeeklyQuestResetTime();

    LOG_INFO("server.loading", "Calculate next monthly quest reset time...");
    InitMonthlyQuestResetTime();

    LOG_INFO("server.loading", "Calculate random battleground reset time..." );
    InitRandomBGResetTime();

    LOG_INFO("server.loading", "Calculate deletion of old calendar events time...");
    InitCalendarOldEventsDeletionTime();

    LOG_INFO("server.loading", "Calculate Guild cap reset time...");
    LOG_INFO("server.loading", " ");
    InitGuildResetTime();

    LOG_INFO("server.loading", "Load Petitions...");
    sPetitionMgr->LoadPetitions();

    LOG_INFO("server.loading", "Load Petition Signs...");
    sPetitionMgr->LoadSignatures();

    LOG_INFO("server.loading", "Load Stored Loot Items...");
    sLootItemStorage->LoadStorageFromDB();

    LOG_INFO("server.loading", "Load Channel Rights...");
    ChannelMgr::LoadChannelRights();

    LOG_INFO("server.loading", "Load Channels...");
    ChannelMgr::LoadChannels();

    LOG_INFO("server.loading", "Load external mail...");
    sExternalMail->LoadSystem();

    sScriptMgr->OnBeforeWorldInitialized();

    if (CONF_GET_BOOL("PreloadAllNonInstancedMapGrids"))
    {
        LOG_INFO("server.loading", "Loading all grids for all non-instanced maps...");

        for (uint32 i = 0; i < sMapStore.GetNumRows(); ++i)
        {
            MapEntry const* mapEntry = sMapStore.LookupEntry(i);

            if (mapEntry && !mapEntry->Instanceable())
            {
                Map* map = sMapMgr->CreateBaseMap(mapEntry->MapID);

                if (map)
                {
                    LOG_INFO("server.loading", ">> Loading all grids for map {}", map->GetId());
                    map->LoadAllCells();
                }
            }
        }
    }

    uint32 startupDuration = GetMSTimeDiffToNow(startupBegin);

    LOG_INFO("server.loading", " ");
    LOG_INFO("server.loading", "WORLD: World initialized in {} minutes {} seconds", (startupDuration / 60000), ((startupDuration % 60000) / 1000)); // outError for red color in console
    LOG_INFO("server.loading", " ");

    METRIC_EVENT("events", "World initialized", "World initialized in " + std::to_string(startupDuration / 60000) + " minutes " + std::to_string((startupDuration % 60000) / 1000) + " seconds");

    if (sConfigMgr->isDryRun())
    {
        sMapMgr->UnloadAll();
        LOG_INFO("server.loading", "Dry run completed, terminating.");
        exit(0);
    }
}

void World::DetectDBCLang()
{
    uint8 m_lang_confid = sConfigMgr->GetOption<int32>("DBC.Locale", 255);

    if (m_lang_confid != 255 && m_lang_confid >= TOTAL_LOCALES)
    {
        LOG_ERROR("server.loading", "Incorrect DBC.Locale! Must be >= 0 and < {} (set to 0)", TOTAL_LOCALES);
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
        LOG_ERROR("server.loading", "Unable to determine your DBC Locale! (corrupt DBC?)");
        exit(1);
    }

    m_defaultDbcLocale = LocaleConstant(default_locale);

    LOG_INFO("server.loading", "Using {} DBC Locale as default. All available DBC locales: {}", localeNames[GetDefaultDbcLocale()], availableLocalsStr.empty() ? "<none>" : availableLocalsStr);
    LOG_INFO("server.loading", " ");
}

/// Update the World !
void World::Update(uint32 diff)
{
    METRIC_TIMER("world_update_time_total");

    ///- Update the game time and check for shutdown time
    _UpdateGameTime();

    Seconds currentGameTime = GameTime::GetGameTime();

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
    }

    ///- Update Who List Cache
    if (m_timers[WUPDATE_WHO_LIST].Passed())
    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update who list"));
        m_timers[WUPDATE_WHO_LIST].Reset();
        sWhoListCacheMgr->Update();
    }

    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Check quest reset times"));

        /// Handle daily quests reset time
        if (currentGameTime > m_NextDailyQuestReset)
        {
            ResetDailyQuests();
        }

        /// Handle weekly quests reset time
        if (currentGameTime > m_NextWeeklyQuestReset)
        {
            ResetWeeklyQuests();
        }

        /// Handle monthly quests reset time
        if (currentGameTime > m_NextMonthlyQuestReset)
        {
            ResetMonthlyQuests();
        }
    }

    if (currentGameTime > m_NextRandomBGReset)
    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Reset random BG"));
        ResetRandomBG();
    }

    if (currentGameTime > m_NextCalendarOldEventsDeletionTime)
    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Delete old calendar events"));
        CalendarDeleteOldEvents();
    }

    if (currentGameTime > m_NextGuildReset)
    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Reset guild cap"));
        ResetGuildCap();
    }

    // pussywizard:
    // acquire mutex now, this is kind of waiting for listing thread to finish it's work (since it can't process next packet)
    // so we don't have to do it in every packet that modifies auctions
    AsyncAuctionListingMgr::SetAuctionListingAllowed(false);
    {
        std::lock_guard<std::mutex> guard(AsyncAuctionListingMgr::GetLock());

        // pussywizard: handle auctions when the timer has passed
        if (m_timers[WUPDATE_AUCTIONS].Passed())
        {
            METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update expired auctions"));

            m_timers[WUPDATE_AUCTIONS].Reset();

            // pussywizard: handle expired auctions, auctions expired when realm was offline are also handled here (not during loading when many required things aren't loaded yet)
            sAuctionMgr->Update();
        }

        AsyncAuctionListingMgr::Update(diff);

        if (currentGameTime > mail_expire_check_timer)
        {
            sObjectMgr->ReturnOrDeleteOldMails(true);
            mail_expire_check_timer = currentGameTime + 6h;
        }

        /// <li> Handle session updates when the timer has passed
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update sessions"));
        UpdateSessions(diff);
    }

    // end of section with mutex
    AsyncAuctionListingMgr::SetAuctionListingAllowed(true);

    /// <li> Handle weather updates when the timer has passed
    if (m_timers[WUPDATE_WEATHERS].Passed())
    {
        m_timers[WUPDATE_WEATHERS].Reset();
        WeatherMgr::Update(uint32(m_timers[WUPDATE_WEATHERS].GetInterval()));
    }

    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update LFG 0"));
        sLFGMgr->Update(diff, 0); // pussywizard: remove obsolete stuff before finding compatibility during map update
    }

    {
        ///- Update objects when the timer has passed (maps, transport, creatures, ...)
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update maps"));
        sMapMgr->Update(diff);
    }

    if (CONF_GET_BOOL("AutoBroadcast.On"))
    {
        if (m_timers[WUPDATE_AUTOBROADCAST].Passed())
        {
            METRIC_TIMER("world_update_time", METRIC_TAG("type", "Send autobroadcast"));
            m_timers[WUPDATE_AUTOBROADCAST].Reset();
            sAutobroadcastMgr->Send();
        }
    }

    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update battlegrounds"));
        sBattlegroundMgr->Update(diff);
    }

    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update outdoor pvp"));
        sOutdoorPvPMgr->Update(diff);
    }

    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update battlefields"));
        sBattlefieldMgr->Update(diff);
    }

    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update LFG 2"));
        sLFGMgr->Update(diff, 2); // pussywizard: handle created proposals
    }

    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Process query callbacks"));
        // execute callbacks from sql queries that were queued recently
        ProcessQueryCallbacks();
    }

    /// <li> Update uptime table
    if (m_timers[WUPDATE_UPTIME].Passed())
    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update uptime"));

        m_timers[WUPDATE_UPTIME].Reset();

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_UPTIME_PLAYERS);

        stmt->SetData(0, uint32(GameTime::GetUptime().count()));
        stmt->SetData(1, uint16(GetMaxPlayerCount()));
        stmt->SetData(2, realm.Id.Realm);
        stmt->SetData(3, uint32(GameTime::GetStartTime().count()));

        LoginDatabase.Execute(stmt);
    }

    ///- Erase corpses once every 20 minutes
    if (m_timers[WUPDATE_CORPSES].Passed())
    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Remove old corpses"));
        m_timers[WUPDATE_CORPSES].Reset();

        sMapMgr->DoForAllMaps([](Map* map)
        {
            map->RemoveOldCorpses();
        });
    }

    ///- Process Game events when necessary
    if (m_timers[WUPDATE_EVENTS].Passed())
    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update game events"));
        m_timers[WUPDATE_EVENTS].Reset();                   // to give time for Update() to be processed
        uint32 nextGameEvent = sGameEventMgr->Update();
        m_timers[WUPDATE_EVENTS].SetInterval(nextGameEvent);
        m_timers[WUPDATE_EVENTS].Reset();
    }

    ///- Ping to keep MySQL connections alive
    if (m_timers[WUPDATE_PINGDB].Passed())
    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Ping MySQL"));
        m_timers[WUPDATE_PINGDB].Reset();
        LOG_DEBUG("sql.driver", "Ping MySQL to keep connection alive");
        CharacterDatabase.KeepAlive();
        LoginDatabase.KeepAlive();
        WorldDatabase.KeepAlive();
    }

    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update instance reset times"));
        // update the instance reset times
        sInstanceSaveMgr->Update();
    }

    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Process cli commands"));
        // And last, but not least handle the issued cli commands
        ProcessCliCommands();
    }

    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update world scripts"));
        sScriptMgr->OnWorldUpdate(diff);
    }

    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update playersSaveScheduler"));
        playersSaveScheduler.Update(diff);
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update external mail system"));
        sExternalMail->Update(diff);
    }

    {
        METRIC_TIMER("world_update_time", METRIC_TAG("type", "Update metrics"));
        // Stats logger update
        sMetric->Update();
        METRIC_VALUE("update_time_diff", diff);
    }
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
    Seconds lastGameTime = GameTime::GetGameTime();
    GameTime::UpdateGameTimers();

    Seconds elapsed = GameTime::GetGameTime() - lastGameTime;

    ///- if there is a shutdown timer
    if (!IsStopped() && m_ShutdownTimer > 0 && elapsed > 0s)
    {
        ///- ... and it is overdue, stop the world (set m_stopEvent)
        if (m_ShutdownTimer <= elapsed.count())
        {
            if (!(m_ShutdownMask & SHUTDOWN_MASK_IDLE) || GetActiveAndQueuedSessionCount() == 0)
                m_stopEvent = true;                         // exist code already set
            else
                m_ShutdownTimer = 1;                        // minimum timer value to wait idle state
        }
        ///- ... else decrease it and if necessary display a shutdown countdown to the users
        else
        {
            m_ShutdownTimer -= elapsed.count();

            ShutdownMsg();
        }
    }
}

/// Shutdown the server
void World::ShutdownServ(uint32 time, uint32 options, uint8 exitcode, const std::string& reason)
{
    // ignore if server shutdown at next tick
    if (IsStopped())
        return;

    m_ShutdownMask = options;
    m_ExitCode = exitcode;

    auto const& playersOnline = GetActiveSessionCount();

    if (time < 5 && playersOnline)
    {
        // Set time to 5s for save all players
        time = 5;
    }

    playersSaveScheduler.CancelAll();

    if (time >= 5)
    {
        playersSaveScheduler.Schedule(Seconds(time - 5), [this](TaskContext /*context*/)
        {
            if (!GetActiveSessionCount())
            {
                LOG_INFO("server", "> No players online. Skip save before shutdown");
                return;
            }

            LOG_INFO("server", "> Save players before shutdown server");
            ObjectAccessor::SaveAllPlayers();
        });
    }

    FMT_LOG_WARN("server", "Time left until shutdown/restart: {}", time);

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
        ShutdownMsg(true, nullptr, reason);
    }

    sScriptMgr->OnShutdownInitiate(ShutdownExitCode(exitcode), ShutdownMask(options));
}

/// Display a shutdown message to the user(s)
void World::ShutdownMsg(bool show, Player* player, const std::string& reason)
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
        std::string str = Warhead::Time::ToTimeString<Seconds>(m_ShutdownTimer).append(".");

        if (!reason.empty())
        {
            str += " - " + reason;
        }

        ServerMessageType msgid = (m_ShutdownMask & SHUTDOWN_MASK_RESTART) ? SERVER_MSG_RESTART_TIME : SERVER_MSG_SHUTDOWN_TIME;

        SendServerMessage(msgid, str.c_str(), player);
        LOG_DEBUG("server.worldserver", "Server is {} in {}", (m_ShutdownMask & SHUTDOWN_MASK_RESTART ? "restart" : "shuttingdown"), str);
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

    LOG_DEBUG("server.worldserver", "Server {} cancelled.", (m_ShutdownMask & SHUTDOWN_MASK_RESTART ? "restart" : "shuttingdown"));

    sScriptMgr->OnShutdownCancel();
}

/// Send a server message to the user(s)
void World::SendServerMessage(ServerMessageType type, std::string_view text, Player* player)
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
    {
        METRIC_DETAILED_NO_THRESHOLD_TIMER("world_update_time",
            METRIC_TAG("type", "Add sessions"),
            METRIC_TAG("parent_type", "Update sessions"));

        ///- Add new sessions
        WorldSession* sess = nullptr;
        while (addSessQueue.next(sess))
        {
            AddSession_(sess);
        }
    }

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
                m_disconnects[pSession->GetAccountId()] = GameTime::GetGameTime().count();
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
            pSession->SetOfflineTime(GameTime::GetGameTime().count());
            m_offlineSessions[pSession->GetAccountId()] = pSession;
            continue;
        }

        [[maybe_unused]] uint32 currentSessionId = itr->first;
        METRIC_DETAILED_TIMER("world_update_sessions_time", METRIC_TAG("account_id", std::to_string(currentSessionId)));

        if (!pSession->Update(diff, updater))
        {
            if (!RemoveQueuedPlayer(pSession) && CONF_GET_INT("DisconnectToleranceInterval"))
                m_disconnects[pSession->GetAccountId()] = GameTime::GetGameTime().count();
            m_sessions.erase(itr);
            if (m_offlineSessions.find(pSession->GetAccountId()) != m_offlineSessions.end()) // pussywizard: don't set offline in db because offline session for that acc is present (character is in world)
                pSession->SetShouldSetOfflineInDB(false);
            delete pSession;
        }
    }

    // pussywizard:
    if (m_offlineSessions.empty())
        return;
    uint32 currTime = GameTime::GetGameTime().count();
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

void World::UpdateRealmCharCount(uint32 accountId)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_COUNT);
    stmt->SetData(0, accountId);
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
        stmt->SetData(0, accountId);
        stmt->SetData(1, realm.Id.Realm);
        trans->Append(stmt);

        stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_REALM_CHARACTERS);
        stmt->SetData(0, charCount);
        stmt->SetData(1, accountId);
        stmt->SetData(2, realm.Id.Realm);
        trans->Append(stmt);

        LoginDatabase.CommitTransaction(trans);
    }
}

// int8 dayOfWeek: 0 (sunday) to 6 (saturday)
time_t World::GetNextTimeWithDayAndHour(int8 dayOfWeek, int8 hour)
{
    if (hour < 0 || hour > 23)
        hour = 0;
    time_t curr = GameTime::GetGameTime().count();
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
    time_t curr = GameTime::GetGameTime().count();
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
    Seconds wstime = sWorld->getWorldState(WS_WEEKLY_QUEST_RESET_TIME);
    m_NextWeeklyQuestReset = wstime > 0s ? wstime : Seconds(GetNextTimeWithDayAndHour(4, 6));

    if (wstime == 0s)
    {
        sWorld->setWorldState(WS_WEEKLY_QUEST_RESET_TIME, m_NextWeeklyQuestReset);
    }
}

void World::InitDailyQuestResetTime()
{
    Seconds wstime = sWorld->getWorldState(WS_DAILY_QUEST_RESET_TIME);
    m_NextDailyQuestReset = wstime > 0s ? wstime : Seconds(GetNextTimeWithDayAndHour(-1, 6));

    if (wstime == 0s)
    {
        sWorld->setWorldState(WS_DAILY_QUEST_RESET_TIME, m_NextDailyQuestReset);
    }
}

void World::InitMonthlyQuestResetTime()
{
    Seconds wstime = sWorld->getWorldState(WS_MONTHLY_QUEST_RESET_TIME);
    m_NextMonthlyQuestReset = wstime > 0s ? wstime : Seconds(GetNextTimeWithMonthAndHour(-1, 6));

    if (wstime == 0s)
    {
        sWorld->setWorldState(WS_MONTHLY_QUEST_RESET_TIME, m_NextMonthlyQuestReset);
    }
}

void World::InitRandomBGResetTime()
{
    Seconds wstime = sWorld->getWorldState(WS_BG_DAILY_RESET_TIME);
    m_NextRandomBGReset = wstime > 0s ? wstime : Seconds(GetNextTimeWithDayAndHour(-1, 6));

    if (wstime == 0s)
    {
        sWorld->setWorldState(WS_BG_DAILY_RESET_TIME, m_NextRandomBGReset);
    }
}

void World::InitCalendarOldEventsDeletionTime()
{
    Seconds now = GameTime::GetGameTime();
    Seconds currentDeletionTime = getWorldState(WS_DAILY_CALENDAR_DELETION_OLD_EVENTS_TIME);
    Seconds nextDeletionTime = currentDeletionTime > 0s ? currentDeletionTime : Seconds(GetNextTimeWithDayAndHour(-1, CONF_GET_INT("Calendar.DeleteOldEventsHour")));

    // If the reset time saved in the worldstate is before now it means the server was offline when the reset was supposed to occur.
    // In this case we set the reset time in the past and next world update will do the reset and schedule next one in the future.
    if (currentDeletionTime < now)
        m_NextCalendarOldEventsDeletionTime = nextDeletionTime - 1_days;
    else
        m_NextCalendarOldEventsDeletionTime = nextDeletionTime;

    if (currentDeletionTime == 0s)
        sWorld->setWorldState(WS_DAILY_CALENDAR_DELETION_OLD_EVENTS_TIME, m_NextCalendarOldEventsDeletionTime);
}

void World::InitGuildResetTime()
{
    Seconds wstime = getWorldState(WS_GUILD_DAILY_RESET_TIME);
    m_NextGuildReset = wstime > 0s ? wstime : Seconds(GetNextTimeWithDayAndHour(-1, 6));

    if (wstime == 0s)
        sWorld->setWorldState(WS_GUILD_DAILY_RESET_TIME, m_NextGuildReset);
}

void World::ResetDailyQuests()
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_QUEST_STATUS_DAILY);
    CharacterDatabase.Execute(stmt);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetDailyQuestStatus();

    m_NextDailyQuestReset = Seconds(GetNextTimeWithDayAndHour(-1, 6));
    sWorld->setWorldState(WS_DAILY_QUEST_RESET_TIME, m_NextDailyQuestReset);

    // change available dailies
    sPoolMgr->ChangeDailyQuests();
}

void World::LoadDBAllowedSecurityLevel()
{
    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_REALMLIST_SECURITY_LEVEL);
    stmt->SetData(0, int32(realm.Id.Realm));
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

    m_NextWeeklyQuestReset = Seconds(GetNextTimeWithDayAndHour(4, 6));
    sWorld->setWorldState(WS_WEEKLY_QUEST_RESET_TIME, m_NextWeeklyQuestReset);

    // change available weeklies
    sPoolMgr->ChangeWeeklyQuests();
}

void World::ResetMonthlyQuests()
{
    LOG_INFO("server.worldserver", "Monthly quests reset for all characters.");

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_QUEST_STATUS_MONTHLY);
    CharacterDatabase.Execute(stmt);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetMonthlyQuestStatus();

    m_NextMonthlyQuestReset = Seconds(GetNextTimeWithMonthAndHour(-1, 6));
    sWorld->setWorldState(WS_MONTHLY_QUEST_RESET_TIME, m_NextMonthlyQuestReset);
}

void World::ResetEventSeasonalQuests(uint16 event_id)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_QUEST_STATUS_SEASONAL);
    stmt->SetData(0, event_id);
    CharacterDatabase.Execute(stmt);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetSeasonalQuestStatus(event_id);
}

void World::ResetRandomBG()
{
    LOG_DEBUG("server.worldserver", "Random BG status reset for all characters.");

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_BATTLEGROUND_RANDOM);
    CharacterDatabase.Execute(stmt);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->SetRandomWinner(false);

    m_NextRandomBGReset = Seconds(GetNextTimeWithDayAndHour(-1, 6));
    sWorld->setWorldState(WS_BG_DAILY_RESET_TIME, m_NextRandomBGReset);
}

void World::CalendarDeleteOldEvents()
{
    LOG_INFO("server.worldserver", "Calendar deletion of old events.");

    m_NextCalendarOldEventsDeletionTime += 1_days;
    sWorld->setWorldState(WS_DAILY_CALENDAR_DELETION_OLD_EVENTS_TIME, m_NextCalendarOldEventsDeletionTime);
    sCalendarMgr->DeleteOldEvents();
}

void World::ResetGuildCap()
{
    LOG_INFO("server.worldserver", "Guild Daily Cap reset.");

    m_NextGuildReset = Seconds(GetNextTimeWithDayAndHour(-1, 6));
    sWorld->setWorldState(WS_GUILD_DAILY_RESET_TIME, m_NextGuildReset);

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
        LOG_INFO("server.loading", ">> Loaded 0 world states. DB table `worldstates` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        m_worldstates[fields[0].GetUInt32()] = Seconds(fields[1].GetUInt32());
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} world states in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

// Setting a worldstate will save it to DB
void World::setWorldState(uint32 index, Seconds timeValue)
{
    auto const& it = m_worldstates.find(index);
    if (it != m_worldstates.end())
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_WORLDSTATE);
        stmt->SetData(0, uint32(timeValue.count()));
        stmt->SetData(1, index);
        CharacterDatabase.Execute(stmt);
    }
    else
    {
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_WORLDSTATE);
        stmt->SetData(0, index);
        stmt->SetData(1, uint32(timeValue.count()));
        CharacterDatabase.Execute(stmt);
    }

    m_worldstates[index] = timeValue;
}

Seconds World::getWorldState(uint32 index) const
{
    auto const& itr = m_worldstates.find(index);
    return itr != m_worldstates.end() ? itr->second : 0s;
}

void World::ProcessQueryCallbacks()
{
    _queryProcessor.ProcessReadyCallbacks();
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

void World::FinalizePlayerWorldSession(WorldSession* session)
{
    uint32 cacheVersion = CONF_GET_UINT("ClientCacheVersion");
    sScriptMgr->OnBeforeFinalizePlayerWorldSession(cacheVersion);

    session->SendAddonsInfo();
    session->SendClientCacheVersion(cacheVersion);
    session->SendTutorialsData();
}

uint16 World::GetConfigMaxSkillValue() const
{
    uint16 lvl = uint16(CONF_GET_INT("MaxPlayerLevel"));
    return lvl > 60 ? 300 + ((lvl - 60) * 75) / 10 : lvl * 5;
}

template<typename Worker>
void World::DoForAllPlayers(Worker&& worker)
{
    if (m_sessions.empty())
        return;

    for (auto const& [accountID, session] : m_sessions)
    {
        Player* player = session->GetPlayer();
        if (!player)
            return;

        worker(player);
    }
}

template<typename Worker>
void World::DoForAllPlayersIsInWorld(Worker&& worker)
{
    if (m_sessions.empty())
        return;

    for (auto const& [accountID, session] : m_sessions)
    {
        Player* player = session->GetPlayer();
        if (!player || !player->IsInWorld())
            return;

        worker(player);
    }
}

template<typename Worker>
void World::DoForAllGM(Worker&& worker)
{
    if (m_sessions.empty())
        return;

    for (auto const& [accountID, session] : m_sessions)
    {
        Player* player = session->GetPlayer();
        if (!player || !player->IsInWorld())
            return;

        if (AccountMgr::IsPlayerAccount(player->GetSession()->GetSecurity()))
            continue;

        worker(player);
    }
}

CliCommandHolder::CliCommandHolder(void* callbackArg, char const* command, Print zprint, CommandFinished commandFinished)
    : m_callbackArg(callbackArg), m_command(strdup(command)), m_print(zprint), m_commandFinished(commandFinished)
{
}

CliCommandHolder::~CliCommandHolder()
{
    free(m_command);
}
