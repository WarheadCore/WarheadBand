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

#ifndef SC_SCRIPTMGR_H
#define SC_SCRIPTMGR_H

#include "DatabaseEnvFwd.h"
#include "Duration.h"
#include "LFG.h"
#include <string_view>

// Core class
class AchievementGlobalMgr;
class AchievementMgr;
class ArenaTeam;
class AuctionEntry;
class AuctionHouseMgr;
class AuctionHouseObject;
class Aura;
class AuraApplication;
class AuraEffect;
class AuraScript;
class Battleground;
class BattlegroundMap;
class BattlegroundQueue;
class Channel;
class ChatHandler;
class Creature;
class CreatureAI;
class DynamicObject;
class GameObject;
class GameObjectAI;
class GridMap;
class Group;
class Guardian;
class Guild;
class InstanceMap;
class InstanceSave;
class InstanceScript;
class Item;
class LootStore;
class LootTemplate;
class MailDraft;
class MailReceiver;
class MailSender;
class Map;
class MapInstanced;
class Object;
class OutdoorPvP;
class Pet;
class Player;
class Quest;
class Spell;
class SpellCastTargets;
class SpellInfo;
class SpellScript;
class TempSummon;
class Transport;
class Unit;
class Vehicle;
class Weather;
class WorldObject;
class WorldPacket;
class WorldSession;
class WorldSocket;

enum ArenaTeamInfoType : uint8;
enum AuraRemoveMode : uint8;
enum BattlegroundDesertionType : uint8;
enum ContentLevels : uint8;
enum DamageEffectType : uint8;
enum EnchantmentSlot : uint8;
enum EncounterCreditType : uint8;
enum InventoryResult : uint8;
enum MailCheckMask : uint8;
enum PetType : uint8;
enum WeaponAttackType : uint8;
enum WeatherState : uint32;
enum ShutdownExitCode : uint8;
enum ShutdownMask : uint8;

struct AchievementCriteriaEntry;
struct AchievementEntry;
struct AreaTrigger;
struct CompletedAchievementData;
struct Condition;
struct ConditionSourceInfo;
struct CreatureTemplate;
struct CriteriaProgress;
struct DungeonEncounter;
struct DungeonProgressionRequirements;
struct GroupQueueInfo;
struct InstanceTemplate;
struct ItemSetEffect;
struct ItemTemplate;
struct Loot;
struct LootStoreItem;
struct MapDifficulty;
struct MapEntry;
struct MovementInfo;
struct PvPDifficultyEntry;
struct ScalingStatValuesEntry;
struct SpellModifier;
struct TargetInfo;
struct VendorItem;
struct QuestStatusData;

// Dynamic linking class
class ModuleReference;

// Script objects
class SpellScriptLoader;

//
struct OutdoorPvPData;

namespace lfg
{
    struct LFGDungeonData;
}

namespace Warhead
{
    namespace Asio
    {
        class IoContext;
    }

    namespace ChatCommands
    {
        struct ChatCommandBuilder;
    }
}

// Manages registration, loading, and execution of scripts.
class WH_GAME_API ScriptMgr
{
public: /* Initialization */
    static ScriptMgr* instance();

    void Initialize();
    void LoadDatabase();
    void FillSpellSummary();

    void IncreaseScriptCount() { ++_scriptCount; }
    void DecreaseScriptCount() { --_scriptCount; }

    std::string_view ScriptsVersion() const { return "Integrated Warhead Scripts"; }

    uint32 GetScriptCount() const { return _scriptCount; }

    typedef void(*ScriptLoaderCallbackType)();
    typedef void(*ModulesLoaderCallbackType)();

    /// Sets the script loader callback which is invoked to load scripts
    /// (Workaround for circular dependency game <-> scripts)
    void SetScriptLoader(ScriptLoaderCallbackType script_loader_callback)
    {
        _script_loader_callback = script_loader_callback;
    }

    /// Sets the modules loader callback which is invoked to load modules
    /// (Workaround for circular dependency game <-> modules)
    void SetModulesLoader(ModulesLoaderCallbackType script_loader_callback)
    {
        _modules_loader_callback = script_loader_callback;
    }

    /// Set the current script context, which allows the ScriptMgr
    /// to accept new scripts in this context.
    /// Requires a SwapScriptContext() call afterwards to load the new scripts.
    void SetScriptContext(std::string_view context);

    /// Returns the current script context.
    std::string_view GetCurrentScriptContext() const;

    /// Releases all scripts associated with the given script context immediately.
    /// Requires a SwapScriptContext() call afterwards to finish the unloading.
    void ReleaseScriptContext(std::string_view context);

    /// Executes all changed introduced by SetScriptContext and ReleaseScriptContext.
    /// It is possible to combine multiple SetScriptContext and ReleaseScriptContext
    /// calls for better performance (bulk changes).
    void SwapScriptContext(bool initialize = false);

    /// Returns the context name of the static context provided by the worldserver
    static std::string_view GetNameOfStaticContext();

    /// Acquires a strong module reference to the module containing the given script name,
    /// which prevents the shared library which contains the script from unloading.
    /// The shared library is lazy unloaded as soon as all references to it are released.
    std::shared_ptr<ModuleReference> AcquireModuleReferenceOfScriptName(std::string_view scriptname) const;

public: /* Unloading */
    void Unload();

public: /* SpellScriptLoader */
    void CreateSpellScripts(uint32 spellId, std::vector<SpellScript*>& scriptVector, Spell* invoker) const;
    void CreateAuraScripts(uint32 spellId, std::vector<AuraScript*>& scriptVector, Aura* invoker) const;
    SpellScriptLoader* GetSpellScriptLoader(uint32 scriptId);

public: /* ServerScript */
    void OnNetworkStart();
    void OnNetworkStop();
    void OnSocketOpen(std::shared_ptr<WorldSocket> socket);
    void OnSocketClose(std::shared_ptr<WorldSocket> socket);
    bool CanPacketReceive(WorldSession* session, WorldPacket const& packet);
    bool CanPacketSend(WorldSession* session, WorldPacket const& packet);
    void OnIoContext(std::weak_ptr<Warhead::Asio::IoContext> ioContext);

public: /* WorldScript */
    void OnLoadCustomDatabaseTable();
    void OnOpenStateChange(bool open);
    void OnBeforeConfigLoad(bool reload);
    void OnAfterConfigLoad(bool reload);
    void OnBeforeFinalizePlayerWorldSession(uint32& cacheVersion);
    void OnMotdChange(std::string& newMotd);
    void OnShutdownInitiate(ShutdownExitCode code, ShutdownMask mask);
    void OnShutdownCancel();
    void OnWorldUpdate(uint32 diff);
    void OnStartup();
    void OnShutdown();
    void OnBeforeWorldInitialized(Microseconds elapsed);
    void OnAfterUnloadAllMaps();

public: /* FormulaScript */
    void OnHonorCalculation(float& honor, uint8 level, float multiplier);
    void OnGrayLevelCalculation(uint8& grayLevel, uint8 playerLevel);
    void OnColorCodeCalculation(XPColorChar& color, uint8 playerLevel, uint8 mobLevel);
    void OnZeroDifferenceCalculation(uint8& diff, uint8 playerLevel);
    void OnBaseGainCalculation(uint32& gain, uint8 playerLevel, uint8 mobLevel, ContentLevels content);
    void OnGainCalculation(uint32& gain, Player* player, Unit* unit);
    void OnGroupRateCalculation(float& rate, uint32 count, bool isRaid);
    void OnAfterArenaRatingCalculation(Battleground* const bg, int32& winnerMatchmakerChange, int32& loserMatchmakerChange, int32& winnerChange, int32& loserChange);
    void OnBeforeUpdatingPersonalRating(int32& mod, uint32 type);

public: /* MapScript */
    void OnCreateMap(Map* map);
    void OnDestroyMap(Map* map);
    void OnLoadGridMap(Map* map, GridMap* gmap, uint32 gx, uint32 gy);
    void OnUnloadGridMap(Map* map, GridMap* gmap, uint32 gx, uint32 gy);
    void OnPlayerEnterMap(Map* map, Player* player);
    void OnPlayerLeaveMap(Map* map, Player* player);
    void OnMapUpdate(Map* map, uint32 diff);

public: /* InstanceMapScript */
    InstanceScript* CreateInstanceScript(InstanceMap* map);

public: /* ItemScript */
    bool OnQuestAccept(Player* player, Item* item, Quest const* quest);
    bool OnItemUse(Player* player, Item* item, SpellCastTargets const& targets);
    bool OnItemExpire(Player* player, ItemTemplate const* proto);
    bool OnItemRemove(Player* player, Item* item);
    bool OnCastItemCombatSpell(Player* player, Unit* victim, SpellInfo const* spellInfo, Item* item);
    void OnGossipSelect(Player* player, Item* item, uint32 sender, uint32 action);
    void OnGossipSelectCode(Player* player, Item* item, uint32 sender, uint32 action, const char* code);

public: /* CreatureScript */
    bool OnGossipHello(Player* player, Creature* creature);
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action);
    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code);
    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest);
    bool OnQuestSelect(Player* player, Creature* creature, Quest const* quest);
    bool OnQuestComplete(Player* player, Creature* creature, Quest const* quest);
    bool OnQuestReward(Player* player, Creature* creature, Quest const* quest, uint32 opt);
    uint32 GetDialogStatus(Player* player, Creature* creature);
    CreatureAI* GetCreatureAI(Creature* creature);
    void OnCreatureUpdate(Creature* creature, uint32 diff);
    void OnCreatureAddWorld(Creature* creature);
    void OnCreatureRemoveWorld(Creature* creature);

public: /* GameObjectScript */
    bool OnGossipHello(Player* player, GameObject* go);
    bool OnGossipSelect(Player* player, GameObject* go, uint32 sender, uint32 action);
    bool OnGossipSelectCode(Player* player, GameObject* go, uint32 sender, uint32 action, const char* code);
    bool OnQuestAccept(Player* player, GameObject* go, Quest const* quest);
    bool OnQuestReward(Player* player, GameObject* go, Quest const* quest, uint32 opt);
    uint32 GetDialogStatus(Player* player, GameObject* go);
    void OnGameObjectDestroyed(GameObject* go, Player* player);
    void OnGameObjectDamaged(GameObject* go, Player* player);
    void OnGameObjectLootStateChanged(GameObject* go, uint32 state, Unit* unit);
    void OnGameObjectStateChanged(GameObject* go, uint32 state);
    void OnGameObjectUpdate(GameObject* go, uint32 diff);
    GameObjectAI* GetGameObjectAI(GameObject* go);
    void OnGameObjectAddWorld(GameObject* go);
    void OnGameObjectRemoveWorld(GameObject* go);

public: /* AreaTriggerScript */
    bool OnAreaTrigger(Player* player, AreaTrigger const* trigger);

public: /* BattlegroundScript */
    Battleground* CreateBattleground(BattlegroundTypeId typeId);

public: /* OutdoorPvPScript */
    OutdoorPvP* CreateOutdoorPvP(OutdoorPvPData const* data);

public: /* CommandScript */
    std::vector<Warhead::ChatCommands::ChatCommandBuilder> GetChatCommands();

public: /* WeatherScript */
    void OnWeatherChange(Weather* weather, WeatherState state, float grade);
    void OnWeatherUpdate(Weather* weather, uint32 diff);

public: /* AuctionHouseScript */
    void OnAuctionAdd(AuctionHouseObject* ah, AuctionEntry* entry);
    void OnAuctionRemove(AuctionHouseObject* ah, AuctionEntry* entry);
    void OnAuctionSuccessful(AuctionHouseObject* ah, AuctionEntry* entry);
    void OnAuctionExpire(AuctionHouseObject* ah, AuctionEntry* entry);
    void OnBeforeAuctionHouseMgrSendAuctionWonMail(AuctionHouseMgr* auctionHouseMgr, AuctionEntry* auction, Player* bidder, uint32& bidder_accId, bool& sendNotification, bool& updateAchievementCriteria, bool& sendMail);
    void OnBeforeAuctionHouseMgrSendAuctionSalePendingMail(AuctionHouseMgr* auctionHouseMgr, AuctionEntry* auction, Player* owner, uint32& owner_accId, bool& sendMail);
    void OnBeforeAuctionHouseMgrSendAuctionSuccessfulMail(AuctionHouseMgr* auctionHouseMgr, AuctionEntry* auction, Player* owner, uint32& owner_accId, uint32& profit, bool& sendNotification, bool& updateAchievementCriteria, bool& sendMail);
    void OnBeforeAuctionHouseMgrSendAuctionExpiredMail(AuctionHouseMgr* auctionHouseMgr, AuctionEntry* auction, Player* owner, uint32& owner_accId, bool& sendNotification, bool& sendMail);
    void OnBeforeAuctionHouseMgrSendAuctionOutbiddedMail(AuctionHouseMgr* auctionHouseMgr, AuctionEntry* auction, Player* oldBidder, uint32& oldBidder_accId, Player* newBidder, uint32& newPrice, bool& sendNotification, bool& sendMail);
    void OnBeforeAuctionHouseMgrSendAuctionCancelledToBidderMail(AuctionHouseMgr* auctionHouseMgr, AuctionEntry* auction, Player* bidder, uint32& bidder_accId, bool& sendMail);
    void OnBeforeAuctionHouseMgrUpdate();

public: /* ConditionScript */
    bool OnConditionCheck(Condition* condition, ConditionSourceInfo& sourceInfo);

public: /* VehicleScript */
    void OnInstall(Vehicle* veh);
    void OnUninstall(Vehicle* veh);
    void OnReset(Vehicle* veh);
    void OnInstallAccessory(Vehicle* veh, Creature* accessory);
    void OnAddPassenger(Vehicle* veh, Unit* passenger, int8 seatId);
    void OnRemovePassenger(Vehicle* veh, Unit* passenger);

public: /* DynamicObjectScript */
    void OnDynamicObjectUpdate(DynamicObject* dynobj, uint32 diff);

public: /* TransportScript */
    void OnAddPassenger(Transport* transport, Player* player);
    void OnAddCreaturePassenger(Transport* transport, Creature* creature);
    void OnRemovePassenger(Transport* transport, Player* player);
    void OnTransportUpdate(Transport* transport, uint32 diff);
    void OnRelocate(Transport* transport, uint32 waypointId, uint32 mapId, float x, float y, float z);

public: /* AchievementCriteriaScript */
    bool OnCriteriaCheck(uint32 scriptId, Player* source, Unit* target, uint32 criteria_id);

public: /* PlayerScript */
    void OnBeforePlayerUpdate(Player* player, uint32 p_time);
    void OnPlayerUpdate(Player* player, uint32 p_time);
    void OnSendInitialPacketsBeforeAddToMap(Player* player, WorldPacket& data);
    void OnPlayerReleasedGhost(Player* player);
    void OnPVPKill(Player* killer, Player* killed);
    void OnPlayerPVPFlagChange(Player* player, bool state);
    void OnCreatureKill(Player* killer, Creature* killed);
    void OnCreatureKilledByPet(Player* petOwner, Creature* killed);
    void OnPlayerKilledByCreature(Creature* killer, Player* killed);
    void OnPlayerLevelChanged(Player* player, uint8 oldLevel);
    void OnPlayerFreeTalentPointsChanged(Player* player, uint32 newPoints);
    void OnPlayerTalentsReset(Player* player, bool noCost);
    void OnPlayerMoneyChanged(Player* player, int32& amount);
    void OnGivePlayerXP(Player* player, uint32& amount, Unit* victim);
    bool OnPlayerReputationChange(Player* player, uint32 factionID, int32& standing, bool incremental);
    void OnPlayerReputationRankChange(Player* player, uint32 factionID, ReputationRank newRank, ReputationRank oldRank, bool increased);
    void OnPlayerLearnSpell(Player* player, uint32 spellID);
    void OnPlayerForgotSpell(Player* player, uint32 spellID);
    void OnPlayerDuelRequest(Player* target, Player* challenger);
    void OnPlayerDuelStart(Player* player1, Player* player2);
    void OnPlayerDuelEnd(Player* winner, Player* loser, DuelCompleteType type);
    void OnPlayerChat(Player* player, uint32 type, uint32 lang, std::string& msg);
    void OnBeforeSendChatMessage(Player* player, uint32& type, uint32& lang, std::string& msg);
    void OnPlayerChat(Player* player, uint32 type, uint32 lang, std::string& msg, Player* receiver);
    void OnPlayerChat(Player* player, uint32 type, uint32 lang, std::string& msg, Group* group);
    void OnPlayerChat(Player* player, uint32 type, uint32 lang, std::string& msg, Guild* guild);
    void OnPlayerChat(Player* player, uint32 type, uint32 lang, std::string& msg, Channel* channel);
    void OnPlayerEmote(Player* player, uint32 emote);
    void OnPlayerTextEmote(Player* player, uint32 textEmote, uint32 emoteNum, ObjectGuid guid);
    void OnPlayerSpellCast(Player* player, Spell* spell, bool skipCheck);
    void OnPlayerLogin(Player* player);
    void OnPlayerLoadFromDB(Player* player);
    void OnPlayerLogout(Player* player);
    void OnPlayerCreate(Player* player);
    void OnPlayerSave(Player* player);
    void OnPlayerDelete(ObjectGuid guid, uint32 accountId);
    void OnPlayerFailedDelete(ObjectGuid guid, uint32 accountId);
    void OnPlayerBindToInstance(Player* player, Difficulty difficulty, uint32 mapid, bool permanent);
    void OnPlayerUpdateZone(Player* player, uint32 newZone, uint32 newArea);
    void OnPlayerUpdateArea(Player* player, uint32 oldArea, uint32 newArea);
    bool OnBeforePlayerTeleport(Player* player, uint32 mapid, float x, float y, float z, float orientation, uint32 options, Unit* target);
    void OnPlayerUpdateFaction(Player* player);
    void OnPlayerAddToBattleground(Player* player, Battleground* bg);
    void OnPlayerQueueRandomDungeon(Player* player, uint32 & rDungeonId);
    void OnPlayerRemoveFromBattleground(Player* player, Battleground* bg);
    void OnAchievementComplete(Player* player, AchievementEntry const* achievement);
    bool OnBeforeAchievementComplete(Player* player, AchievementEntry const* achievement);
    void OnCriteriaProgress(Player* player, AchievementCriteriaEntry const* criteria);
    bool OnBeforeCriteriaProgress(Player* player, AchievementCriteriaEntry const* criteria);
    void OnAchievementSave(CharacterDatabaseTransaction trans, Player* player, uint16 achiId, CompletedAchievementData const* achiData);
    void OnCriteriaSave(CharacterDatabaseTransaction trans, Player* player, uint16 critId, CriteriaProgress const* criteriaData);
    void OnGossipSelect(Player* player, uint32 menu_id, uint32 sender, uint32 action);
    void OnGossipSelectCode(Player* player, uint32 menu_id, uint32 sender, uint32 action, const char* code);
    void OnPlayerBeingCharmed(Player* player, Unit* charmer, uint32 oldFactionId, uint32 newFactionId);
    void OnAfterPlayerSetVisibleItemSlot(Player* player, uint8 slot, Item* item);
    void OnAfterPlayerMoveItemFromInventory(Player* player, Item* it, uint8 bag, uint8 slot, bool update);
    void OnEquip(Player* player, Item* it, uint8 bag, uint8 slot, bool update);
    void OnPlayerJoinBG(Player* player);
    void OnPlayerJoinArena(Player* player);
    void GetCustomGetArenaTeamId(Player const* player, uint8 slot, uint32& teamID) const;
    void GetCustomArenaPersonalRating(Player const* player, uint8 slot, uint32& rating) const;
    void OnGetMaxPersonalArenaRatingRequirement(Player const* player, uint32 minSlot, uint32& maxArenaRating) const;
    void OnLootItem(Player* player, Item* item, uint32 count, ObjectGuid lootguid);
    void OnCreateItem(Player* player, Item* item, uint32 count);
    void OnQuestRewardItem(Player* player, Item* item, uint32 count);
    bool OnBeforePlayerQuestComplete(Player* player, uint32 quest_id);
    void OnQuestComputeXP(Player* player, Quest const* quest, uint32& xpValue);
    void OnBeforePlayerDurabilityRepair(Player* player, ObjectGuid npcGUID, ObjectGuid itemGUID, float& discountMod, uint8 guildBank);
    void OnBeforeBuyItemFromVendor(Player* player, ObjectGuid vendorguid, uint32 vendorslot, uint32& item, uint8 count, uint8 bag, uint8 slot);
    void OnBeforeStoreOrEquipNewItem(Player* player, uint32 vendorslot, uint32& item, uint8 count, uint8 bag, uint8 slot, ItemTemplate const* pProto, Creature* pVendor, VendorItem const* crItem, bool bStore);
    void OnAfterStoreOrEquipNewItem(Player* player, uint32 vendorslot, Item* item, uint8 count, uint8 bag, uint8 slot, ItemTemplate const* pProto, Creature* pVendor, VendorItem const* crItem, bool bStore);
    void OnAfterUpdateMaxPower(Player* player, Powers& power, float& value);
    void OnAfterUpdateMaxHealth(Player* player, float& value);
    void OnBeforeUpdateAttackPowerAndDamage(Player* player, float& level, float& val2, bool ranged);
    void OnAfterUpdateAttackPowerAndDamage(Player* player, float& level, float& base_attPower, float& attPowerMod, float& attPowerMultiplier, bool ranged);
    void OnBeforeInitTalentForLevel(Player* player, uint8& level, uint32& talentPointsForLevel);
    void OnFirstLogin(Player* player);
    void OnPlayerCompleteQuest(Player* player, Quest const* quest);
    void OnBattlegroundDesertion(Player* player, BattlegroundDesertionType const desertionType);
    bool CanJoinInBattlegroundQueue(Player* player, ObjectGuid BattlemasterGuid, BattlegroundTypeId BGTypeID, uint8 joinAsGroup, GroupJoinBattlegroundResult& err);
    bool ShouldBeRewardedWithMoneyInsteadOfExp(Player* player);
    void OnBeforeTempSummonInitStats(Player* player, TempSummon* tempSummon, uint32& duration);
    void OnBeforeGuardianInitStatsForLevel(Player* player, Guardian* guardian, CreatureTemplate const* cinfo, PetType& petType);
    void OnAfterGuardianInitStatsForLevel(Player* player, Guardian* guardian);
    void OnBeforeLoadPetFromDB(Player* player, uint32& petentry, uint32& petnumber, bool& current, bool& forceLoadFromDB);
    bool CanJoinInArenaQueue(Player* player, ObjectGuid BattlemasterGuid, uint8 arenaslot, BattlegroundTypeId BGTypeID, uint8 joinAsGroup, uint8 IsRated, GroupJoinBattlegroundResult& err);
    bool CanBattleFieldPort(Player* player, uint8 arenaType, BattlegroundTypeId BGTypeID, uint8 action);
    bool CanGroupInvite(Player* player, std::string& membername);
    bool CanGroupAccept(Player* player, Group* group);
    bool CanSellItem(Player* player, Item* item, Creature* creature);
    bool CanSendMail(Player* player, ObjectGuid receiverGuid, ObjectGuid mailbox, std::string& subject, std::string& body, uint32 money, uint32 COD, Item* item);
    void PetitionBuy(Player* player, Creature* creature, uint32& charterid, uint32& cost, uint32& type);
    void PetitionShowList(Player* player, Creature* creature, uint32& CharterEntry, uint32& CharterDispayID, uint32& CharterCost);
    void OnRewardKillRewarder(Player* player, bool isDungeon, float& rate);
    bool CanGiveMailRewardAtGiveLevel(Player* player, uint8 level);
    void OnDeleteFromDB(CharacterDatabaseTransaction trans, uint32 guid);
    bool CanRepopAtGraveyard(Player* player);
    void OnGetMaxSkillValue(Player* player, uint32 skill, int32& result, bool IsPure);
    bool CanAreaExploreAndOutdoor(Player* player);
    void OnVictimRewardBefore(Player* player, Player* victim, uint32& killer_title, uint32& victim_title);
    void OnVictimRewardAfter(Player* player, Player* victim, uint32& killer_title, uint32& victim_rank, float& honor_f);
    void OnCustomScalingStatValueBefore(Player* player, ItemTemplate const* proto, uint8 slot, bool apply, uint32& CustomScalingStatValue);
    void OnCustomScalingStatValue(Player* player, ItemTemplate const* proto, uint32& statType, int32& val, uint8 itemProtoStatNumber, uint32 ScalingStatValue, ScalingStatValuesEntry const* ssv);
    bool CanArmorDamageModifier(Player* player);
    void OnGetFeralApBonus(Player* player, int32& feral_bonus, int32 dpsMod, ItemTemplate const* proto, ScalingStatValuesEntry const* ssv);
    bool CanApplyWeaponDependentAuraDamageMod(Player* player, Item* item, WeaponAttackType attackType, AuraEffect const* aura, bool apply);
    bool CanApplyEquipSpell(Player* player, SpellInfo const* spellInfo, Item* item, bool apply, bool form_change);
    bool CanApplyEquipSpellsItemSet(Player* player, ItemSetEffect* eff);
    bool CanCastItemCombatSpell(Player* player, Unit* target, WeaponAttackType attType, uint32 procVictim, uint32 procEx, Item* item, ItemTemplate const* proto);
    bool CanCastItemUseSpell(Player* player, Item* item, SpellCastTargets const& targets, uint8 cast_count, uint32 glyphIndex);
    void OnApplyAmmoBonuses(Player* player, ItemTemplate const* proto, float& currentAmmoDPS);
    bool CanEquipItem(Player* player, uint8 slot, uint16& dest, Item* pItem, bool swap, bool not_loading);
    bool CanUnequipItem(Player* player, uint16 pos, bool swap);
    bool CanUseItem(Player* player, ItemTemplate const* proto, InventoryResult& result);
    bool CanSaveEquipNewItem(Player* player, Item* item, uint16 pos, bool update);
    bool CanApplyEnchantment(Player* player, Item* item, EnchantmentSlot slot, bool apply, bool apply_dur, bool ignore_condition);
    void OnGetQuestRate(Player* player, float& result);
    bool PassedQuestKilledMonsterCredit(Player* player, Quest const* qinfo, uint32 entry, uint32 real_entry, ObjectGuid guid);
    bool CheckItemInSlotAtLoadInventory(Player* player, Item* item, uint8 slot, uint8& err, uint16& dest);
    bool NotAvoidSatisfy(Player* player, DungeonProgressionRequirements const* ar, uint32 target_map, bool report);
    bool NotVisibleGloballyFor(Player* player, Player const* u);
    void OnGetArenaPersonalRating(Player* player, uint8 slot, uint32& result);
    void OnGetArenaTeamId(Player* player, uint8 slot, uint32& result);
    void OnIsFFAPvP(Player* player, bool& result);
    void OnIsPvP(Player* player, bool& result);
    void OnGetMaxSkillValueForLevel(Player* player, uint16& result);
    bool NotSetArenaTeamInfoField(Player* player, uint8 slot, ArenaTeamInfoType type, uint32 value);
    bool CanJoinLfg(Player* player, uint8 roles, lfg::LfgDungeonSet& dungeons, const std::string& comment);
    bool CanEnterMap(Player* player, MapEntry const* entry, InstanceTemplate const* instance, MapDifficulty const* mapDiff, bool loginCheck);
    bool CanInitTrade(Player* player, Player* target);
    void OnSetServerSideVisibility(Player* player, ServerSideVisibilityType& type, AccountTypes& sec);
    void OnSetServerSideVisibilityDetect(Player* player, ServerSideVisibilityType& type, AccountTypes& sec);
    void OnGiveHonorPoints(Player* player, float& honor, Unit* victim);
    void OnAfterResurrect(Player* player, float restore_percent, bool applySickness);
    void OnPlayerResurrect(Player* player, float restore_percent, bool applySickness);
    bool CanPlayerUseChat(Player* player, uint32 type, uint32 language, std::string& msg);
    bool CanPlayerUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Player* receiver);
    bool CanPlayerUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Group* group);
    bool CanPlayerUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Guild* guild);
    bool CanPlayerUseChat(Player* player, uint32 type, uint32 language, std::string& msg, Channel* channel);
    void OnPlayerLearnTalents(Player* player, uint32 talentId, uint32 talentRank, uint32 spellid);
    void OnPlayerEnterCombat(Player* player, Unit* enemy);
    void OnPlayerLeaveCombat(Player* player);
    void OnQuestAbandon(Player* player, uint32 questId);
    void OnGetDodgeFromAgility(Player* player, float& diminishing, float& nondiminishing);
    void OnGetArmorFromAgility(Player* player, float& value);
    void OnGetMeleeCritFromAgility(Player* player, float& value);
    void OnGetSpellCritFromIntellect(Player* player, float& value);
    void OnGetManaBonusFromIntellect(Player* player, float& value);
    void OnGetShieldBlockValue(Player* player, float& value);
    void OnUpdateAttackPowerAndDamage(Player* player, float& apFromAgility);
    void OnCalculateMinMaxDamage(Player* player, float& damageFromAP);
    bool CanCompleteQuest(Player* player, Quest const* questInfo, QuestStatusData const* questStatusData);
    void OnAddQuest(Player* player, Quest const* quest, Object* questGiver);

    // Anti cheat
    void AnticheatSetSkipOnePacketForASH(Player* player, bool apply);
    void AnticheatSetCanFlybyServer(Player* player, bool apply);
    void AnticheatSetUnderACKmount(Player* player);
    void AnticheatSetRootACKUpd(Player* player);
    void AnticheatUpdateMovementInfo(Player* player, MovementInfo const& movementInfo);
    void AnticheatSetJumpingbyOpcode(Player* player, bool jump);
    bool AnticheatHandleDoubleJump(Player* player, Unit* mover);
    bool AnticheatCheckMovementInfo(Player* player, MovementInfo const& movementInfo, Unit* mover, bool jump);

public: /* AccountScript */
    void OnAccountLogin(uint32 accountId);
    void OnAccountLogout(uint32 accountId);
    void OnLastIpUpdate(uint32 accountId, std::string ip);
    void OnFailedAccountLogin(uint32 accountId);
    void OnEmailChange(uint32 accountId);
    void OnFailedEmailChange(uint32 accountId);
    void OnPasswordChange(uint32 accountId);
    void OnFailedPasswordChange(uint32 accountId);

public: /* GuildScript */
    void OnGuildAddMember(Guild* guild, Player* player, uint8& plRank);
    void OnGuildRemoveMember(Guild* guild, Player* player, bool isDisbanding, bool isKicked);
    void OnGuildMOTDChanged(Guild* guild, const std::string& newMotd);
    void OnGuildInfoChanged(Guild* guild, const std::string& newInfo);
    void OnGuildCreate(Guild* guild, Player* leader, const std::string& name);
    void OnGuildDisband(Guild* guild);
    void OnGuildMemberWitdrawMoney(Guild* guild, Player* player, uint32& amount, bool isRepair);
    void OnGuildMemberDepositMoney(Guild* guild, Player* player, uint32& amount);
    void OnGuildItemMove(Guild* guild, Player* player, Item* pItem, bool isSrcBank, uint8 srcContainer, uint8 srcSlotId,
                         bool isDestBank, uint8 destContainer, uint8 destSlotId);
    void OnGuildEvent(Guild* guild, uint8 eventType, ObjectGuid::LowType playerGuid1, ObjectGuid::LowType playerGuid2, uint8 newRank);
    void OnGuildBankEvent(Guild* guild, uint8 eventType, uint8 tabId, ObjectGuid::LowType playerGuid, uint32 itemOrMoney, uint16 itemStackCount, uint8 destTabId);
    bool CanGuildSendBankList(Guild const* guild, WorldSession* session, uint8 tabId, bool sendAllSlots);

public: /* GroupScript */
    void OnGroupAddMember(Group* group, ObjectGuid guid);
    void OnGroupInviteMember(Group* group, ObjectGuid guid);
    void OnGroupRemoveMember(Group* group, ObjectGuid guid, RemoveMethod method, ObjectGuid kicker, const char* reason);
    void OnGroupChangeLeader(Group* group, ObjectGuid newLeaderGuid, ObjectGuid oldLeaderGuid);
    void OnGroupDisband(Group* group);
    bool CanGroupJoinBattlegroundQueue(Group const* group, Player* member, Battleground const* bgTemplate, uint32 MinPlayerCount, bool isRated, uint32 arenaSlot);
    void OnCreate(Group* group, Player* leader);

public: /* GlobalScript */
    void OnGlobalItemDelFromDB(CharacterDatabaseTransaction trans, ObjectGuid::LowType itemGuid);
    void OnGlobalMirrorImageDisplayItem(Item const* item, uint32& display);
    void OnBeforeUpdateArenaPoints(ArenaTeam* at, std::map<ObjectGuid, uint32>& ap);
    void OnAfterRefCount(Player const* player, Loot& loot, bool canRate, uint16 lootMode, LootStoreItem* LootStoreItem, uint32& maxcount, LootStore const& store);
    void OnBeforeDropAddItem(Player const* player, Loot& loot, bool canRate, uint16 lootMode, LootStoreItem* LootStoreItem, LootStore const& store);
    bool OnItemRoll(Player const* player, LootStoreItem const* LootStoreItem, float& chance, Loot& loot, LootStore const& store);
    bool OnBeforeLootEqualChanced(Player const* player, std::list<LootStoreItem*> const* EqualChanced, Loot& loot, LootStore const& store);
    void OnInitializeLockedDungeons(Player* player, uint8& level, uint32& lockData, lfg::LFGDungeonData const* dungeon);
    void OnAfterInitializeLockedDungeons(Player* player);
    void OnAfterUpdateEncounterState(Map* map, EncounterCreditType type, uint32 creditEntry, Unit* source, Difficulty difficulty_fixed, std::list<DungeonEncounter const*> const* encounters, uint32 dungeonCompleted, bool updated);
    void OnBeforeWorldObjectSetPhaseMask(WorldObject const* worldObject, uint32& oldPhaseMask, uint32& newPhaseMask, bool& useCombinedPhases, bool& update);
    bool OnIsAffectedBySpellModCheck(SpellInfo const* affectSpell, SpellInfo const* checkSpell, SpellModifier const* mod);
    bool OnSpellHealingBonusTakenNegativeModifiers(Unit const* target, Unit const* caster, SpellInfo const* spellInfo, float& val);
    void OnLoadSpellCustomAttr(SpellInfo* spell);
    bool OnAllowedForPlayerLootCheck(Player const* player, ObjectGuid source);

public: /* UnitScript */
    void OnHeal(Unit* healer, Unit* reciever, uint32& gain);
    void OnDamage(Unit* attacker, Unit* victim, uint32& damage);
    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage);
    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage);
    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage);
    void ModifyHealRecieved(Unit* target, Unit* attacker, uint32& addHealth);
    uint32 DealDamage(Unit* AttackerUnit, Unit* pVictim, uint32 damage, DamageEffectType damagetype);
    void OnBeforeRollMeleeOutcomeAgainst(Unit const* attacker, Unit const* victim, WeaponAttackType attType, int32& attackerMaxSkillValueForLevel, int32& victimMaxSkillValueForLevel, int32& attackerWeaponSkill, int32& victimDefenseSkill, int32& crit_chance, int32& miss_chance, int32& dodge_chance, int32& parry_chance, int32& block_chance);
    void OnAuraRemove(Unit* unit, AuraApplication* aurApp, AuraRemoveMode mode);
    bool IfNormalReaction(Unit const* unit, Unit const* target, ReputationRank& repRank);
    bool IsNeedModSpellDamagePercent(Unit const* unit, AuraEffect* auraEff, float& doneTotalMod, SpellInfo const* spellProto);
    bool IsNeedModMeleeDamagePercent(Unit const* unit, AuraEffect* auraEff, float& doneTotalMod, SpellInfo const* spellProto);
    bool IsNeedModHealPercent(Unit const* unit, AuraEffect* auraEff, float& doneTotalMod, SpellInfo const* spellProto);
    bool CanSetPhaseMask(Unit const* unit, uint32 newPhaseMask, bool update);
    bool IsCustomBuildValuesUpdate(Unit const* unit, uint8 updateType, ByteBuffer* fieldBuffer, Player const* target, uint16 index);
    bool OnBuildValuesUpdate(Unit const* unit, uint8 updateType, ByteBuffer* fieldBuffer, Player* target, uint16 index);
    void OnUnitUpdate(Unit* unit, uint32 diff);
    void OnDisplayIdChange(Unit* unit, uint32 displayId);
    void OnUnitEnterEvadeMode(Unit* unit, uint8 why);
    void OnUnitEnterCombat(Unit* unit, Unit* victim);
    void OnUnitDeath(Unit* unit, Unit* killer);

public: /* MovementHandlerScript */
    void OnPlayerMove(Player* player, MovementInfo* movementInfo, uint32 opcode);

public: /* AllCreatureScript */
    //listener function (OnAllCreatureUpdate) is called by OnCreatureUpdate
    //void OnAllCreatureUpdate(Creature* creature, uint32 diff);
    void Creature_SelectLevel(const CreatureTemplate* cinfo, Creature* creature);
    void OnCreatureSaveToDB(Creature* creature);
    bool CanCreatureSendListInventory(Player* player, Creature* creature, uint32 vendorEntry);

public: /* AllGameobjectScript */
    void OnGameObjectSaveToDB(GameObject* go);

public: /* AllMapScript */
    void OnBeforeCreateInstanceScript(InstanceMap* instanceMap, InstanceScript* instanceData, bool load, std::string data, uint32 completedEncounterMask);
    void OnDestroyInstance(MapInstanced* mapInstanced, Map* map);

public: /* BGScript */
    void OnBattlegroundStart(Battleground* bg);
    void OnBattlegroundEndReward(Battleground* bg, Player* player, TeamId winnerTeamId);
    void OnBattlegroundUpdate(Battleground* bg, uint32 diff);
    void OnBattlegroundAddPlayer(Battleground* bg, Player* player);
    void OnBattlegroundBeforeAddPlayer(Battleground* bg, Player* player);
    void OnBattlegroundRemovePlayerAtLeave(Battleground* bg, Player* player);
    void OnQueueUpdate(BattlegroundQueue* queue, uint32 diff, BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket_id, uint8 arenaType, bool isRated, uint32 arenaRating);
    void OnAddGroup(BattlegroundQueue* queue, GroupQueueInfo* ginfo, uint32& index, Player* leader, Group* group, BattlegroundTypeId bgTypeId, PvPDifficultyEntry const* bracketEntry,
        uint8 arenaType, bool isRated, bool isPremade, uint32 arenaRating, uint32 matchmakerRating, uint32 arenaTeamId, uint32 opponentsArenaTeamId);
    bool CanFillPlayersToBG(BattlegroundQueue* queue, Battleground* bg, BattlegroundBracketId bracket_id);
    bool IsCheckNormalMatch(BattlegroundQueue* queue, Battleground* bgTemplate, BattlegroundBracketId bracket_id, uint32 minPlayers, uint32 maxPlayers);
    bool CanSendMessageBGQueue(BattlegroundQueue* queue, Player* leader, Battleground* bg, PvPDifficultyEntry const* bracketEntry);
    bool OnBeforeSendJoinMessageArenaQueue(BattlegroundQueue* queue, Player* leader, GroupQueueInfo* ginfo, PvPDifficultyEntry const* bracketEntry, bool isRated);
    bool OnBeforeSendExitMessageArenaQueue(BattlegroundQueue* queue, GroupQueueInfo* ginfo);
    void OnBattlegroundEnd(Battleground* bg, TeamId winnerTeamId);
    void OnBattlegroundDestroy(Battleground* bg);
    void OnBattlegroundCreate(Battleground* bg);

public: /* Arena Team Script */
    void OnGetSlotByType(const uint32 type, uint8& slot);
    void OnGetArenaPoints(ArenaTeam* at, float& points);
    void OnArenaTypeIDToQueueID(const BattlegroundTypeId bgTypeId, const uint8 arenaType, uint32& queueTypeID);
    void OnArenaQueueIdToArenaType(const BattlegroundQueueTypeId bgQueueTypeId, uint8& ArenaType);
    void OnSetArenaMaxPlayersPerTeam(const uint8 arenaType, uint32& maxPlayerPerTeam);

public: /* SpellSC */
    void OnCalcMaxDuration(Aura const* aura, int32& maxDuration);
    bool CanModAuraEffectDamageDone(AuraEffect const* auraEff, Unit* target, AuraApplication const* aurApp, uint8 mode, bool apply);
    bool CanModAuraEffectModDamagePercentDone(AuraEffect const* auraEff, Unit* target, AuraApplication const* aurApp, uint8 mode, bool apply);
    void OnSpellCheckCast(Spell* spell, bool strict, SpellCastResult& res);
    bool CanPrepare(Spell* spell, SpellCastTargets const* targets, AuraEffect const* triggeredByAura);
    bool CanScalingEverything(Spell* spell);
    bool CanSelectSpecTalent(Spell* spell);
    void OnScaleAuraUnitAdd(Spell* spell, Unit* target, uint32 effectMask, bool checkIfValid, bool implicit, uint8 auraScaleMask, TargetInfo& targetInfo);
    void OnRemoveAuraScaleTargets(Spell* spell, TargetInfo& targetInfo, uint8 auraScaleMask, bool& needErase);
    void OnBeforeAuraRankForLevel(SpellInfo const* spellInfo, SpellInfo const* latestSpellInfo, uint8 level);
    void OnDummyEffect(WorldObject* caster, uint32 spellID, SpellEffIndex effIndex, GameObject* gameObjTarget);
    void OnDummyEffect(WorldObject* caster, uint32 spellID, SpellEffIndex effIndex, Creature* creatureTarget);
    void OnDummyEffect(WorldObject* caster, uint32 spellID, SpellEffIndex effIndex, Item* itemTarget);

public: /* GameEventScript */
    void OnGameEventStart(uint16 EventID);
    void OnGameEventStop(uint16 EventID);
    void OnGameEventCheck(uint16 EventID);

public: /* MailScript */
    void OnBeforeMailDraftSendMailTo(MailDraft* mailDraft, MailReceiver const& receiver, MailSender const& sender, MailCheckMask& checked, uint32& deliver_delay, uint32& custom_expiration, bool& deleteMailItemsFromDB, bool& sendMail);

public: /* AchievementScript */

    void SetRealmCompleted(AchievementEntry const* achievement);
    bool IsCompletedCriteria(AchievementMgr* mgr, AchievementCriteriaEntry const* achievementCriteria, AchievementEntry const* achievement, CriteriaProgress const* progress);
    bool IsRealmCompleted(AchievementGlobalMgr const* globalmgr, AchievementEntry const* achievement, SystemTimePoint completionTime);
    void OnBeforeCheckCriteria(AchievementMgr* mgr, std::list<AchievementCriteriaEntry const*> const* achievementCriteriaList);
    bool CanCheckCriteria(AchievementMgr* mgr, AchievementCriteriaEntry const* achievementCriteria);

public: /* PetScript */

    void OnInitStatsForLevel(Guardian* guardian, uint8 petlevel);
    void OnCalculateMaxTalentPointsForLevel(Pet* pet, uint8 level, uint8& points);
    bool CanUnlearnSpellSet(Pet* pet, uint32 level, uint32 spell);
    bool CanUnlearnSpellDefault(Pet* pet, SpellInfo const* spellEntry);
    bool CanResetTalents(Pet* pet);

public: /* ArenaScript */

    bool CanAddMember(ArenaTeam* team, ObjectGuid PlayerGuid);
    void OnGetPoints(ArenaTeam* team, uint32 memberRating, float& points);
    bool CanSaveToDB(ArenaTeam* team);

public: /* MiscScript */

    void OnConstructObject(Object* origin);
    void OnDestructObject(Object* origin);
    void OnConstructPlayer(Player* origin);
    void OnDestructPlayer(Player* origin);
    void OnConstructGroup(Group* origin);
    void OnDestructGroup(Group* origin);
    void OnConstructInstanceSave(InstanceSave* origin);
    void OnDestructInstanceSave(InstanceSave* origin);
    void OnItemCreate(Item* item, ItemTemplate const* itemProto, Player const* owner);
    bool CanApplySoulboundFlag(Item* item, ItemTemplate const* proto);
    bool CanItemApplyEquipSpell(Player* player, Item* item);
    bool CanSendAuctionHello(WorldSession const* session, ObjectGuid guid, Creature* creature);
    void ValidateSpellAtCastSpell(Player* player, uint32& oldSpellId, uint32& spellId, uint8& castCount, uint8& castFlags);
    void OnPlayerSetPhase(const AuraEffect* auraEff, AuraApplication const* aurApp, uint8 mode, bool apply, uint32& newPhase);
    void ValidateSpellAtCastSpellResult(Player* player, Unit* mover, Spell* spell, uint32 oldSpellId, uint32 spellId);
    void OnAfterLootTemplateProcess(Loot* loot, LootTemplate const* tab, LootStore const& store, Player* lootOwner, bool personal, bool noEmptyError, uint16 lootMode);
    void OnInstanceSave(InstanceSave* instanceSave);
    void GetDialogStatus(Player* player, Object* questgiver);

public: /* CommandSC */

    void OnHandleDevCommand(Player* player, bool& enable);
    bool CanExecuteCommand(ChatHandler& handler, std::string_view cmdStr);

public: /* DatabaseScript */

    void OnAfterDatabasesLoaded(uint32 updateFlags);

public: /* WorldObjectScript */

    void OnWorldObjectDestroy(WorldObject* object);
    void OnWorldObjectCreate(WorldObject* object);
    void OnWorldObjectSetMap(WorldObject* object, Map* map);
    void OnWorldObjectResetMap(WorldObject* object);
    void OnWorldObjectUpdate(WorldObject* object, uint32 diff);

public: /* PetScript */

    void OnPetAddToWorld(Pet* pet);

public: /* LootScript */

    void OnLootMoney(Player* player, uint32 gold);

private:
    uint32 _scriptCount{ 0 };

    ScriptLoaderCallbackType _script_loader_callback{ nullptr };
    ModulesLoaderCallbackType _modules_loader_callback{ nullptr };

    std::string _currentContext;
};

#define sScriptMgr ScriptMgr::instance()

#endif
