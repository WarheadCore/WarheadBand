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

#ifndef WARHEAD_DBCSTORES_H
#define WARHEAD_DBCSTORES_H

#include "Common.h"
#include "DBCStore.h"
#include "DBCStructure.h"
#include <list>
#include <unordered_map>
#include <unordered_set>

typedef std::list<uint32> SimpleFactionsList;

WH_GAME_API SimpleFactionsList const* GetFactionTeamList(uint32 faction);

WH_GAME_API char const* GetPetName(uint32 petfamily, uint32 dbclang);
WH_GAME_API uint32 GetTalentSpellCost(uint32 spellId);
WH_GAME_API TalentSpellPos const* GetTalentSpellPos(uint32 spellId);

WH_GAME_API WMOAreaTableEntry const* GetWMOAreaTableEntryByTripple(int32 rootid, int32 adtid, int32 groupid);

WH_GAME_API uint32 GetVirtualMapForMapAndZone(uint32 mapid, uint32 zoneId);

enum ContentLevels
{
    CONTENT_1_60 = 0,
    CONTENT_61_70,
    CONTENT_71_80
};

WH_GAME_API ContentLevels GetContentLevelsForMapAndZone(uint32 mapid, uint32 zoneId);

WH_GAME_API void Zone2MapCoordinates(float& x, float& y, uint32 zone);
WH_GAME_API void Map2ZoneCoordinates(float& x, float& y, uint32 zone);

typedef std::map<uint32/*pair32(map, diff)*/, MapDifficulty> MapDifficultyMap;
WH_GAME_API MapDifficulty const* GetMapDifficultyData(uint32 mapId, Difficulty difficulty);
WH_GAME_API MapDifficulty const* GetDownscaledMapDifficultyData(uint32 mapId, Difficulty& difficulty);

WH_GAME_API bool IsSharedDifficultyMap(uint32 mapid);

WH_GAME_API uint32 const* /*[MAX_TALENT_TABS]*/ GetTalentTabPages(uint8 cls);

WH_GAME_API uint32 GetLiquidFlags(uint32 liquidType);

WH_GAME_API PvPDifficultyEntry const* GetBattlegroundBracketByLevel(uint32 mapid, uint32 level);
WH_GAME_API PvPDifficultyEntry const* GetBattlegroundBracketById(uint32 mapid, BattlegroundBracketId id);

WH_GAME_API CharStartOutfitEntry const* GetCharStartOutfitEntry(uint8 race, uint8 class_, uint8 gender);

WH_GAME_API LFGDungeonEntry const* GetLFGDungeon(uint32 mapId, Difficulty difficulty);
WH_GAME_API LFGDungeonEntry const* GetZoneLFGDungeonEntry(std::string const& zoneName, LocaleConstant locale);

WH_GAME_API uint32 GetDefaultMapLight(uint32 mapId);

typedef std::unordered_multimap<uint32, SkillRaceClassInfoEntry const*> SkillRaceClassInfoMap;
typedef std::pair<SkillRaceClassInfoMap::iterator, SkillRaceClassInfoMap::iterator> SkillRaceClassInfoBounds;
WH_GAME_API SkillRaceClassInfoEntry const* GetSkillRaceClassInfo(uint32 skill, uint8 race, uint8 class_);

WH_GAME_API extern DBCStorage <AchievementEntry>             sAchievementStore;
WH_GAME_API extern DBCStorage <AchievementCriteriaEntry>     sAchievementCriteriaStore;
WH_GAME_API extern DBCStorage <AchievementCategoryEntry>     sAchievementCategoryStore;
WH_GAME_API extern DBCStorage <AreaTableEntry>               sAreaTableStore;
WH_GAME_API extern DBCStorage <AreaGroupEntry>               sAreaGroupStore;
WH_GAME_API extern DBCStorage <AreaPOIEntry>                 sAreaPOIStore;
WH_GAME_API extern DBCStorage <AuctionHouseEntry>            sAuctionHouseStore;
WH_GAME_API extern DBCStorage <BankBagSlotPricesEntry>       sBankBagSlotPricesStore;
WH_GAME_API extern DBCStorage <BarberShopStyleEntry>         sBarberShopStyleStore;
WH_GAME_API extern DBCStorage <BattlemasterListEntry>        sBattlemasterListStore;
WH_GAME_API extern DBCStorage <ChatChannelsEntry>            sChatChannelsStore;
WH_GAME_API extern DBCStorage <CharStartOutfitEntry>         sCharStartOutfitStore;
WH_GAME_API extern DBCStorage <CharTitlesEntry>              sCharTitlesStore;
WH_GAME_API extern DBCStorage <ChrClassesEntry>              sChrClassesStore;
WH_GAME_API extern DBCStorage <ChrRacesEntry>                sChrRacesStore;
WH_GAME_API extern DBCStorage <CinematicCameraEntry>         sCinematicCameraStore;
WH_GAME_API extern DBCStorage <CinematicSequencesEntry>      sCinematicSequencesStore;
WH_GAME_API extern DBCStorage <CreatureDisplayInfoEntry>     sCreatureDisplayInfoStore;
WH_GAME_API extern DBCStorage <CreatureDisplayInfoExtraEntry>sCreatureDisplayInfoExtraStore;
WH_GAME_API extern DBCStorage <CreatureFamilyEntry>          sCreatureFamilyStore;
WH_GAME_API extern DBCStorage <CreatureModelDataEntry>       sCreatureModelDataStore;
WH_GAME_API extern DBCStorage <CreatureSpellDataEntry>       sCreatureSpellDataStore;
WH_GAME_API extern DBCStorage <CreatureTypeEntry>            sCreatureTypeStore;
WH_GAME_API extern DBCStorage <CurrencyTypesEntry>           sCurrencyTypesStore;
WH_GAME_API extern DBCStorage <DestructibleModelDataEntry>   sDestructibleModelDataStore;
WH_GAME_API extern DBCStorage <DungeonEncounterEntry>        sDungeonEncounterStore;
WH_GAME_API extern DBCStorage <DurabilityCostsEntry>         sDurabilityCostsStore;
WH_GAME_API extern DBCStorage <DurabilityQualityEntry>       sDurabilityQualityStore;
WH_GAME_API extern DBCStorage <EmotesEntry>                  sEmotesStore;
WH_GAME_API extern DBCStorage <EmotesTextEntry>              sEmotesTextStore;
WH_GAME_API extern DBCStorage <FactionEntry>                 sFactionStore;
WH_GAME_API extern DBCStorage <FactionTemplateEntry>         sFactionTemplateStore;
WH_GAME_API extern DBCStorage <GameObjectDisplayInfoEntry>   sGameObjectDisplayInfoStore;
WH_GAME_API extern DBCStorage <GemPropertiesEntry>           sGemPropertiesStore;
WH_GAME_API extern DBCStorage <GlyphPropertiesEntry>         sGlyphPropertiesStore;
WH_GAME_API extern DBCStorage <GlyphSlotEntry>               sGlyphSlotStore;

WH_GAME_API extern DBCStorage <GtBarberShopCostBaseEntry>    sGtBarberShopCostBaseStore;
WH_GAME_API extern DBCStorage <GtCombatRatingsEntry>         sGtCombatRatingsStore;
WH_GAME_API extern DBCStorage <GtChanceToMeleeCritBaseEntry> sGtChanceToMeleeCritBaseStore;
WH_GAME_API extern DBCStorage <GtChanceToMeleeCritEntry>     sGtChanceToMeleeCritStore;
WH_GAME_API extern DBCStorage <GtChanceToSpellCritBaseEntry> sGtChanceToSpellCritBaseStore;
WH_GAME_API extern DBCStorage <GtChanceToSpellCritEntry>     sGtChanceToSpellCritStore;
WH_GAME_API extern DBCStorage <GtNPCManaCostScalerEntry>     sGtNPCManaCostScalerStore;
WH_GAME_API extern DBCStorage <GtOCTClassCombatRatingScalarEntry> sGtOCTClassCombatRatingScalarStore;
WH_GAME_API extern DBCStorage <GtOCTRegenHPEntry>            sGtOCTRegenHPStore;
//extern DBCStorage <GtOCTRegenMPEntry>            sGtOCTRegenMPStore; -- not used currently
WH_GAME_API extern DBCStorage <GtRegenHPPerSptEntry>         sGtRegenHPPerSptStore;
WH_GAME_API extern DBCStorage <GtRegenMPPerSptEntry>         sGtRegenMPPerSptStore;
WH_GAME_API extern DBCStorage <HolidaysEntry>                sHolidaysStore;
WH_GAME_API extern DBCStorage <ItemBagFamilyEntry>           sItemBagFamilyStore;
WH_GAME_API extern DBCStorage <ItemDisplayInfoEntry>         sItemDisplayInfoStore;
WH_GAME_API extern DBCStorage <ItemExtendedCostEntry>        sItemExtendedCostStore;
WH_GAME_API extern DBCStorage <ItemLimitCategoryEntry>       sItemLimitCategoryStore;
WH_GAME_API extern DBCStorage <ItemRandomPropertiesEntry>    sItemRandomPropertiesStore;
WH_GAME_API extern DBCStorage <ItemRandomSuffixEntry>        sItemRandomSuffixStore;
WH_GAME_API extern DBCStorage <ItemSetEntry>                 sItemSetStore;
WH_GAME_API extern DBCStorage <LFGDungeonEntry>           sLFGDungeonStore;
WH_GAME_API extern DBCStorage <LiquidTypeEntry>              sLiquidTypeStore;
WH_GAME_API extern DBCStorage <LockEntry>                    sLockStore;
WH_GAME_API extern DBCStorage <MailTemplateEntry>            sMailTemplateStore;
WH_GAME_API extern DBCStorage <MapEntry>                     sMapStore;
//extern DBCStorage <MapDifficultyEntry>           sMapDifficultyStore; -- use GetMapDifficultyData insteed
extern MapDifficultyMap                          sMapDifficultyMap;
extern DBCStorage <MovieEntry>                   sMovieStore;
extern DBCStorage <OverrideSpellDataEntry>       sOverrideSpellDataStore;
extern DBCStorage <PowerDisplayEntry>            sPowerDisplayStore;
extern DBCStorage <QuestSortEntry>               sQuestSortStore;
extern DBCStorage <QuestXPEntry>                 sQuestXPStore;
extern DBCStorage <QuestFactionRewEntry>         sQuestFactionRewardStore;
extern DBCStorage <RandomPropertiesPointsEntry>  sRandomPropertiesPointsStore;
extern DBCStorage <ScalingStatDistributionEntry> sScalingStatDistributionStore;
extern DBCStorage <ScalingStatValuesEntry>       sScalingStatValuesStore;
extern DBCStorage <SkillLineEntry>               sSkillLineStore;
extern DBCStorage <SkillLineAbilityEntry>        sSkillLineAbilityStore;
extern DBCStorage <SkillTiersEntry>              sSkillTiersStore;
extern DBCStorage <SoundEntriesEntry>            sSoundEntriesStore;
extern DBCStorage <SpellCastTimesEntry>          sSpellCastTimesStore;
extern DBCStorage <SpellCategoryEntry>           sSpellCategoryStore;
extern DBCStorage <SpellDifficultyEntry>         sSpellDifficultyStore;
extern DBCStorage <SpellDurationEntry>           sSpellDurationStore;
extern DBCStorage <SpellFocusObjectEntry>        sSpellFocusObjectStore;
extern DBCStorage <SpellItemEnchantmentEntry>    sSpellItemEnchantmentStore;
extern DBCStorage <SpellItemEnchantmentConditionEntry> sSpellItemEnchantmentConditionStore;
extern SpellCategoryStore                        sSpellsByCategoryStore;
extern PetFamilySpellsStore                      sPetFamilySpellsStore;
extern std::unordered_set<uint32>                sPetTalentSpells;
extern DBCStorage <SpellRadiusEntry>             sSpellRadiusStore;
extern DBCStorage <SpellRangeEntry>              sSpellRangeStore;
extern DBCStorage <SpellRuneCostEntry>           sSpellRuneCostStore;
extern DBCStorage <SpellShapeshiftEntry>         sSpellShapeshiftStore;
extern DBCStorage <SpellEntry>                   sSpellStore;
extern DBCStorage <SpellVisualEntry>             sSpellVisualStore;
extern DBCStorage <StableSlotPricesEntry>        sStableSlotPricesStore;
extern DBCStorage <SummonPropertiesEntry>        sSummonPropertiesStore;
extern DBCStorage <TalentEntry>                  sTalentStore;
extern DBCStorage <TalentTabEntry>               sTalentTabStore;
extern DBCStorage <TaxiNodesEntry>               sTaxiNodesStore;
extern DBCStorage <TaxiPathEntry>                sTaxiPathStore;
extern TaxiMask                                  sTaxiNodesMask;
extern TaxiMask                                  sOldContinentsNodesMask;
extern TaxiMask                                  sHordeTaxiNodesMask;
extern TaxiMask                                  sAllianceTaxiNodesMask;
extern TaxiMask                                  sDeathKnightTaxiNodesMask;
extern TaxiPathSetBySource                       sTaxiPathSetBySource;
extern TaxiPathNodesByPath                       sTaxiPathNodesByPath;
extern DBCStorage <TeamContributionPointsEntry>  sTeamContributionPointsStore;
extern DBCStorage <TotemCategoryEntry>           sTotemCategoryStore;
extern DBCStorage <VehicleEntry>                 sVehicleStore;
extern DBCStorage <VehicleSeatEntry>             sVehicleSeatStore;
extern DBCStorage <WMOAreaTableEntry>            sWMOAreaTableStore;
//extern DBCStorage <WorldMapAreaEntry>           sWorldMapAreaStore; -- use Zone2MapCoordinates and Map2ZoneCoordinates
extern DBCStorage <WorldMapOverlayEntry>         sWorldMapOverlayStore;

WH_GAME_API void LoadDBCStores(const std::string& dataPath);

#endif
