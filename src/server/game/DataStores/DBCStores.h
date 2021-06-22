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

#ifndef WARHEAD_DBCSTORES_H
#define WARHEAD_DBCSTORES_H

#include "Common.h"
#include "DBCStore.h"
#include "DBCStructure.h"
#include <list>
#include <unordered_map>

typedef std::list<uint32> SimpleFactionsList;
typedef std::vector<FlyByCamera> FlyByCameraCollection;

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
WH_GAME_API uint32 GetDefaultMapLight(uint32 mapId);

typedef std::unordered_multimap<uint32, SkillRaceClassInfoEntry const*> SkillRaceClassInfoMap;
typedef std::pair<SkillRaceClassInfoMap::iterator, SkillRaceClassInfoMap::iterator> SkillRaceClassInfoBounds;
SkillRaceClassInfoEntry const* GetSkillRaceClassInfo(uint32 skill, uint8 race, uint8 class_);
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
WH_GAME_API extern MapDifficultyMap                          sMapDifficultyMap;
WH_GAME_API extern DBCStorage <MovieEntry>                   sMovieStore;
WH_GAME_API extern DBCStorage <OverrideSpellDataEntry>       sOverrideSpellDataStore;
WH_GAME_API extern DBCStorage <PowerDisplayEntry>            sPowerDisplayStore;
WH_GAME_API extern DBCStorage <QuestSortEntry>               sQuestSortStore;
WH_GAME_API extern DBCStorage <QuestXPEntry>                 sQuestXPStore;
WH_GAME_API extern DBCStorage <QuestFactionRewEntry>         sQuestFactionRewardStore;
WH_GAME_API extern DBCStorage <RandomPropertiesPointsEntry>  sRandomPropertiesPointsStore;
WH_GAME_API extern DBCStorage <ScalingStatDistributionEntry> sScalingStatDistributionStore;
WH_GAME_API extern DBCStorage <ScalingStatValuesEntry>       sScalingStatValuesStore;
WH_GAME_API extern DBCStorage <SkillLineEntry>               sSkillLineStore;
WH_GAME_API extern DBCStorage <SkillLineAbilityEntry>        sSkillLineAbilityStore;
WH_GAME_API extern DBCStorage <SoundEntriesEntry>            sSoundEntriesStore;
WH_GAME_API extern DBCStorage <SpellCastTimesEntry>          sSpellCastTimesStore;
WH_GAME_API extern DBCStorage <SpellCategoryEntry>           sSpellCategoryStore;
WH_GAME_API extern DBCStorage <SpellDifficultyEntry>         sSpellDifficultyStore;
WH_GAME_API extern DBCStorage <SpellDurationEntry>           sSpellDurationStore;
WH_GAME_API extern DBCStorage <SpellFocusObjectEntry>        sSpellFocusObjectStore;
WH_GAME_API extern DBCStorage <SpellItemEnchantmentEntry>    sSpellItemEnchantmentStore;
WH_GAME_API extern DBCStorage <SpellItemEnchantmentConditionEntry> sSpellItemEnchantmentConditionStore;
WH_GAME_API extern SpellCategoryStore                        sSpellsByCategoryStore;
WH_GAME_API extern PetFamilySpellsStore                      sPetFamilySpellsStore;
WH_GAME_API extern DBCStorage <SpellRadiusEntry>             sSpellRadiusStore;
WH_GAME_API extern DBCStorage <SpellRangeEntry>              sSpellRangeStore;
WH_GAME_API extern DBCStorage <SpellRuneCostEntry>           sSpellRuneCostStore;
WH_GAME_API extern DBCStorage <SpellShapeshiftEntry>         sSpellShapeshiftStore;
WH_GAME_API extern DBCStorage <SpellEntry>                   sSpellStore;
WH_GAME_API extern DBCStorage <StableSlotPricesEntry>        sStableSlotPricesStore;
WH_GAME_API extern DBCStorage <SummonPropertiesEntry>        sSummonPropertiesStore;
WH_GAME_API extern DBCStorage <TalentEntry>                  sTalentStore;
WH_GAME_API extern DBCStorage <TalentTabEntry>               sTalentTabStore;
WH_GAME_API extern DBCStorage <TaxiNodesEntry>               sTaxiNodesStore;
WH_GAME_API extern DBCStorage <TaxiPathEntry>                sTaxiPathStore;
WH_GAME_API extern TaxiMask                                  sTaxiNodesMask;
WH_GAME_API extern TaxiMask                                  sOldContinentsNodesMask;
WH_GAME_API extern TaxiMask                                  sHordeTaxiNodesMask;
WH_GAME_API extern TaxiMask                                  sAllianceTaxiNodesMask;
WH_GAME_API extern TaxiMask                                  sDeathKnightTaxiNodesMask;
WH_GAME_API extern TaxiPathSetBySource                       sTaxiPathSetBySource;
WH_GAME_API extern TaxiPathNodesByPath                       sTaxiPathNodesByPath;
WH_GAME_API extern DBCStorage <TeamContributionPointsEntry>  sTeamContributionPointsStore;
WH_GAME_API extern DBCStorage <TotemCategoryEntry>           sTotemCategoryStore;
WH_GAME_API extern DBCStorage <VehicleEntry>                 sVehicleStore;
WH_GAME_API extern DBCStorage <VehicleSeatEntry>             sVehicleSeatStore;
WH_GAME_API extern DBCStorage <WMOAreaTableEntry>            sWMOAreaTableStore;
// WH_GAME_API extern DBCStorage <WorldMapAreaEntry>           sWorldMapAreaStore; -- use Zone2MapCoordinates and Map2ZoneCoordinates
WH_GAME_API extern DBCStorage <WorldMapOverlayEntry>         sWorldMapOverlayStore;
WH_GAME_API extern std::unordered_map<uint32, FlyByCameraCollection> sFlyByCameraStore;

WH_GAME_API void LoadDBCStores(const std::string& dataPath);
WH_GAME_API void LoadM2Cameras(const std::string& dataPath);

#endif
