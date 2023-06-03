SET @entry := 999991;
SET @scriptName := 'Arena1v1_Creature';

DELETE FROM `creature_template` WHERE `entry`= @entry;
INSERT INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`, `speed_walk`, `speed_run`, `scale`, `rank`, `dmgschool`, `DamageModifier`, `BaseAttackTime`, `RangeAttackTime`, `unit_class`, `unit_flags`, `unit_flags2`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `HoverHeight`, `HealthModifier`, `ManaModifier`, `ArmorModifier`, `RacialLeader`, `movementId`, `RegenHealth`, `mechanic_immune_mask`, `flags_extra`, `ScriptName`, `VerifiedBuild`) VALUES
    (@entry, 0, 0, 0, 0, 0, 23766, 0, 0, 0, 'Arena Battlemaster 1v1', '', '', 8218, 70, 70, 2, 35, 1048577, 1.1, 1.14286, 3, 0, 0, 1, 2000, 2000, 1, 768, 2048, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, @scriptName, 12340);

DELETE FROM `battlemaster_entry` WHERE `entry`= @entry;
INSERT INTO `battlemaster_entry` (`entry`, `bg_template`) VALUES (@entry, 6);
