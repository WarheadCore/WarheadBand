--

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (32860, 32858, 31975, 32861, 32863);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(32860, 32860, 38378),
(32858, 32858, 38377),
(31975, 31975, 35511),
(32861, 32861, 38379),
(32863, 32863, 38252);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (32329, 34163, 34171, 31410, 31407, 22887);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(32329, 32329, 37965),
(34163, 34163, 37967),
(34171, 34171, 37956),
(31410, 31410, 37973),
(31407, 31407, 39413),
(22887, 22887, 40317);

DELETE FROM `spelldifficulty_dbc` WHERE `ID`=34171;
INSERT INTO `spelldifficulty_dbc` VALUES
(34171,34171,37956,0,0);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (33617, 33783);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(33617, 33617, 39363),
(33783, 33783, 39364);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (19130,30925,12739,30500,30495);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(19130, 19130, 40392),
(30925, 30925, 40059),
(12739, 12739, 15472),
(30500, 30500, 35954),
(30495, 30495, 35953);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (32197, 40062, 30846);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(32197, 32197, 37113),
(40062, 40062, 40064),
(30846, 30846, 32784);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (34798, 34797, 34644, 34359, 34358, 27637, 34254, 34642, 34641);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(34798, 34798, 39121),
(34797, 34797, 39120),
(34644, 34644, 39122),
(34359, 34359, 39128),
(34358, 34358, 39127),
(27637, 27637, 39125),
(34254, 34254, 39126),
(34642, 34642, 39347),
(34641, 34641, 39129);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (8374,15039,33534,12548,33620,15659,38795);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(8374, 8374, 38761),
(15039, 15039, 15616),
(33534, 33534, 38135),
(12548, 12548, 21401),
(33620, 33620, 38137),
(15659, 15659, 15305),
(38795, 38795, 33666);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (36275, 36279, 36278, 36277);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(36275, 36275, 38533),
(36279, 38534, 38533),
(36278, 38536, 38533),
(36277, 38535, 38533);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (32191, 32193, 15234, 34944, 17139, 34945);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(32191, 32191, 37666),
(32193, 32193, 37665),
(15234, 15234, 37664),
(34944, 34944, 37669),
(17139, 17139, 36052),
(34945, 34945, 39378);

UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64 WHERE `ID` = 31692;

DROP TABLE IF EXISTS `namesreserved_dbc`;
CREATE TABLE `namesreserved_dbc` (
	`ID` INT UNSIGNED NOT NULL,
	`Pattern` TINYTEXT NOT NULL,
    `LanguagueID` TINYINT NOT NULL,
	PRIMARY KEY (`ID`)
);

DROP TABLE IF EXISTS `namesprofanity_dbc`;
CREATE TABLE `namesprofanity_dbc` (
	`ID` INT UNSIGNED NOT NULL,
	`Pattern` TINYTEXT NOT NULL,
    `LanguagueID` TINYINT NOT NULL,
	PRIMARY KEY (`ID`)
);


UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64 WHERE `ID` IN (33495,33514,33515,33516,33517,33518,33519,33520);
UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = 0 WHERE `ID` = 35937;
UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = 0 WHERE `ID` IN (37947, 37948, 37949);
UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64 WHERE `ID` IN (34810, 34817, 34818, 34819);
UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = 0 WHERE `ID` = 31374;
UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = 0 WHERE `ID` = 39111;
UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = 1 WHERE `ID` IN (38888, 38889, 38890);
UPDATE `spell_dbc` SET `ProcTypeMask` = 20, `ProcChance` = 100, `BaseLevel` = 70, `SpellLevel` = 70, `Effect_1` = 6, `ImplicitTargetA_1` = 1, `EffectAura_1` = 42, `EffectTriggerSpell_1` = 45195 WHERE `Id` = 45196;

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (36719, 36716);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(36719, 36719, 38830),
(36716, 36716, 38828);

UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = 0 WHERE `ID`=37394;

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (35012,35243,35261,36340,36348,35265,35267,36345,35056,35057);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(35012, 35012, 38941),
(35243, 35243, 38935),
(35261, 35261, 38936),
(36340, 36340, 38921),
(36348, 36348, 38919),
(35265, 35265, 38933),
(35267, 35267, 38930),
(36345, 36345, 39196),
(35056, 35056, 38923),
(35057, 35057, 38925);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (36617,36619,36664,36655,36657,36660,36654,36738,36786,36787,36891,36887,36868,36742,36741,36743,36740,36736,36863,36864,36838,36840,37480,37479,36829,36832,36827,35964,35932,36984,36835,38855,36837);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(36617, 36617, 38810),
(36619, 36619, 38811),
(36664, 36664, 38816),
(36655, 36655, 38817),
(36657, 36657, 38818),
(36660, 36660, 38820),
(36654, 36654, 38813),
(36738, 36738, 38835),
(36786, 36786, 38843),
(36787, 36787, 38846),
(36891, 36891, 38849),
(36887, 36887, 38850),
(36868, 36868, 38892),
(36742, 36742, 38836), -- Fireball Volley
(36741, 36741, 38837), -- Frostbolt Volley
(36743, 36743, 38838), -- Holy Bolt Volley
(36740, 36740, 38839), -- Lightning Bolt Volley
(36736, 36736, 38840), -- Spell Shadow Bolt Volley
(36863, 36863, 38851),
(36864, 36864, 38852),
(36838, 36838, 38894),
(36840, 36840, 38896),
(37480, 37480, 38900),
(37479, 37479, 38899),
(36829, 36829, 38917),
(36832, 36832, 38918),
(36827, 36827, 38912),
(35964, 35964, 38942),
(35932, 35932, 38943),
(36984, 36984, 38914),
(36835, 36835, 38911),
(38855, 38855, 38901),
(36837, 36837, 38903);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (3262,3263,3264,3265,3266,3267,5000,5001,5002,5003,5004,5005,5006,5007,5008);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (57570,57579,57581,39529,57757,57728,56908,56910,57598);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`, `DifficultySpellID_3`, `DifficultySpellID_4`) VALUES
(57570, 57570, 59126, 0, 0),
(57579, 57579, 59127, 0, 0),
(57581, 57581, 59128, 0, 0),
(39529, 39529, 58940, 0, 0),
(57757, 57757, 58936, 0, 0),
(57728, 57728, 58947, 0, 0),
(56908, 56908, 58956, 0, 0),
(56910, 56910, 58957, 0, 0),
(57598, 57598, 58964, 0, 0);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (40331,35106,34449,37272,37252);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(40331, 40331, 40332),
(35106, 35106, 37856),
(34449, 34449, 37924),
(37272, 37272, 37862),
(37252, 37252, 39412);

UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = 0 WHERE `ID` IN (35153, 35904, 35905, 35906);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (33480, 32863);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(33480, 33480, 38226),
(32863, 32863, 38252);

UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = -1 WHERE `ID` IN (
14802 -- Idol Room Spawn B
);

UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = 0 WHERE `ID` IN (
12694, -- Idol Room Spawn A
12949, -- Idol Room Spawn End Boss
14801, -- Idol Room Spawn C
19826, -- Summon Blackwing Legionnaire
19827, -- Summon Blackwing Mage
19828, -- Summon Death Talon Dragonspawn
20172, -- Summon Onyxian Whelp
21287, -- Conjure Lokholar the Usurper DND
23118, -- Conjure Scourge Footsoldier DND
23209, -- Terrordale Haunting Spirit #2
23253, -- Terrordale Haunting Spirit #3
23361, -- Raise Undead Drakonid
24215, -- Create Heart of Hakkar Explosion
24250, -- Summon Zulian Stalker
24349, -- Summon Bloodlord's Raptor
25151, -- Summon Vekniss Drone
26140, -- Summon Hook Tentacle
26144, -- Summon Eye Tentacle
26145, -- Summon Eye Tentacle
26146, -- Summon Eye Tentacle
26147, -- Summon Eye Tentacle
26148, -- Summon Eye Tentacle
26149, -- Summon Eye Tentacle
26150, -- Summon Eye Tentacle
26151, -- Summon Eye Tentacle
26191, -- Teleport Giant Hook Tentacle
26216, -- Summon Giant Hook Tentacles
26396, -- Summon Portal Ground State
26477, -- Summon Giant Portal Ground State
26564, -- Summon Viscidus Trigger
26617, -- Summon Ouro Mound
26768, -- Summon Giant Eye Tentacles
26837, -- Summon InCombat Trigger
27178, -- Defile
27643, -- Summon Spirit of Jarien
27644, -- Summon Spirit of Sothos
27884, -- Summon Trainee
27921, -- Summon Spectral Trainee
27932, -- Summon Spectral Knight
27939, -- Summon Spectral Rivendare
28008, -- Summon Knight
28010, -- Summon Mounted Knight
28175, -- (DND) Summon Crystal Minion, Ghost
28177, -- (DND) Summon Crystal Minion, Skeleton
28179, -- (DND) Summon Crystal Minion, Ghoul
28217, -- Summon Zombie Chow
28218, -- Summon Fallout Slime
28227, -- (DND) Summon Crystal Minion, finder
28289, -- (DND) Summon Crystal Minion, Ghoul Uncommon
28290, -- (DND) Summon Crystal Minion, Ghost Uncommon
28291, -- (DND) Summon Crystal Minion, Skeleton Uncommon
28421, -- Summon Type A
28422, -- Summon Type B
28423, -- Summon Type C
28454, -- Summon Type D
28561, -- Summon Blizzard
28627, -- Summon Web Wrap
29141, -- Marauding Crust Borer
29218, -- Summon Flame Ring
29329, -- Summon Sapphiron's Wing Buffet
29508, -- Summon Crypt Guard
29869, -- Fished Up Murloc
30083, -- Summon Root Thresher
30445, -- Stillpine Ancestor Yor
30630, -- Debris
30737, -- Summon Heathen
30785, -- Summon Reaver
30786, -- Summon Sharpshooter
30954, -- Free Webbed Creature
30955, -- Free Webbed Creature
30956, -- Free Webbed Creature
30957, -- Free Webbed Creature
30958, -- Free Webbed Creature
30959, -- Free Webbed Creature
30960, -- Free Webbed Creature
30961, -- Free Webbed Creature
30962, -- Free Webbed Creature
30963, -- Free Webbed Creature
31010, -- Free Webbed Creature
31318, -- Summon Infinite Assassin
31321, -- Summon Black Morass Rift Lord
31391, -- Summon Black Morass Chrono Lord Deja
31392, -- Summon Black Morass Temporus
31393, -- Summon Black Morass Rift End Boss
31421, -- Summon Infinite Chronomancer
31528, -- Summon Gnome
31529, -- Summon Gnome
31530, -- Summon Gnome
31544, -- Summon Distiller
31545, -- Summon Distiller
31593, -- Summon Greater Manawraith
32114, -- Summon Wisp
32151, -- Infernal
32283, -- Focus Fire
32360, -- Summon Stolen Soul
32579, -- Portal Beam
32632, -- Summon Overrun Target
33121, -- A Vision of the Forgotten
33229, -- Wrath of the Astromancer
33242, -- Infernal
33363, -- Summon Infinite Executioner
33364, -- Summon Infinite Vanquisher
33367, -- Summon Astromancer Priest
33567, -- Summon Void Portal D
33677, -- Incite Chaos
33680, -- Incite Chaos
33681, -- Incite Chaos
33682, -- Incite Chaos
33683, -- Incite Chaos
33901, -- Summon Crystalhide Crumbler
33927, -- Summon Void Summoner
34064, -- Soul Split
34125, -- Spotlight
34175, -- Arcane Orb Primer
35127, -- Summon Boom Bot Target
35136, -- Summon Captured Critter
35142, -- Drijya Summon Imp
35145, -- Drijya Summon Doomguard
35146, -- Drijya Summon Terrorguard
35256, -- Summon Unstable Mushroom
35430, -- Infernal
35861, -- Summon Nether Vapor
35862, -- Summon Nether Vapor
35863, -- Summon Nether Vapor
35864, -- Summon Nether Vapor
36026, -- Conjure Elemental Soul: Earth
36036, -- Summon Netherstorm Target
36042, -- Summon Farahlon Crumbler
36043, -- Summon Farahlon Crumbler
36044, -- Summon Farahlon Crumbler
36045, -- Summon Farahlon Shardling
36046, -- Summon Farahlon Shardling
36047, -- Summon Farahlon Shardling
36048, -- Summon Motherlode Shardling
36049, -- Summon Motherlode Shardling
36050, -- Summon Motherlode Shardling
36112, -- Conjure Elemental Soul: Fire
36168, -- Conjure Elemental Soul: Water
36180, -- Conjure Elemental Soul: Air
36221, -- Summon  Eye of the Citadel
36229, -- Summon Infinite Assassin
36231, -- Summon Infinite Chronomancer
36232, -- Summon Infinite Executioner
36233, -- Summon Infinite Vanquisher
36234, -- Summon Black Morass Rift Lord Alt
36235, -- Summon Black Morass Rift Keeper
36236, -- Summon Black Morass Rift Keeper
36521, -- Summon Arcane Explosion
36579, -- Summon Netherock Crumbler
36584, -- Summon Netherock Crumbler
36585, -- Summon Netherock Crumbler
36595, -- Summon Apex Crumbler
36596, -- Summon Apex Crumbler
36597, -- Summon Apex Crumbler
36724, -- Summon Phoenix Egg
36818, -- Attacking Infernal
36865, -- Summon Gnome Cannon Channel Target (DND)
37177, -- Summon Black Morass Infinite Chrono-Lord
37178, -- Summon Black Morass Infinite Timereaver
37457, -- Windsor Dismisses Horse DND
37606, -- Summon Infinite Assassin
37758, -- Bone Wastes - Summon Auchenai Spirit
37766, -- Summon Murloc A1
37772, -- Summon Murloc B1
37773, -- Summon Elemental A1
37774, -- Summon Elemental B1
37911, -- Summon Elemental A2
37912, -- Summon Elemental A3
37914, -- Summon Elemental B2
37916, -- Summon Elemental B3
37923, -- Summon Murloc A2
37925, -- Summon Murloc A3
37926, -- Summon Murloc A4
37927, -- Summon Murloc A5
37928, -- Summon Murloc B2
37929, -- Summon Murloc B3
37931, -- Summon Murloc B4
37932, -- Summon Murloc B5
38111, -- Summon Horde Bat Rider Guard
38114, -- Summon Horde Rooftop Alarm Sensor
38118, -- Summon Area 52 Death Machine Guard
38124, -- Summon Horde Ground Alarm Sensor
38137, -- Summon Sky Marker
38179, -- Summon Alliance Ground Alarm Sensor
38180, -- Summon Alliance Rooftop Alarm Sensor
38181, -- Summon Alliance Gryphon Guard
38261, -- Summon Area 52 Rooftop Alarm Sensor
38266, -- Summon Stormspire Ethereal Guard
38268, -- Summon Scryer Dragonhawk Guard
38270, -- Summon Stormspire Rooftop Alarm Sensor
38271, -- Summon Scryer Rooftop Alarm Sensor
38278, -- Summon Aldor Gryphon Guard
38283, -- Summon Aldor Rooftop Alarm Sensor
38286, -- Summon Sporeggar Sporebat Guard
38287, -- Summon Sporeggar Rooftop Alarm Sensor
38288, -- Summon Toshley Guard
38291, -- Summon Toshley Rooftop Alarm Sensor
38402, -- Summon Cenarion Storm Crow Guard
38403, -- Summon Cenarion Expedition Rooftop Alarm Sensor
38512, -- Fiery Boulder
38587, -- Summon Spirit of Redemption
38854, -- Hatch Arakkoa
38865, -- Hatch Bad Arakkoa
39080, -- Summon Mountain Shardling
39081, -- Summon Vortex Shardling
39186, -- Summon Random Tractor
39191, -- Sha'tari Flames
39302, -- Quest - The Exorcism, Summon Foul Purge
39305  -- Summon Flying Skull
);

UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = 1 WHERE `ID` IN (
20734, -- Black Arrow
30792, -- Summon Ravager Ambusher
30825, -- Summon Siltfin Ambusher
30826, -- Summon Wildkin Ambusher
30976, -- Summon Gauntlet Guards
31995, -- Shattered Rumbler
39110  -- Summon Phoenix Adds
);

UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = 2 WHERE `ID` IN (
30076, -- Summon Maexxna Spiderling
30827, -- Summon Bristlelimb Ambusher
36379  -- Call Skitterers
);

UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = 3 WHERE `ID` IN (
26630, -- Spawn Vekniss Hatchlings
26631, -- Spawn Vekniss Hatchlings
26632, -- Spawn Vekniss Hatchlings
30828  -- Summon Sunhawk Ambushers
);

UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = 4 WHERE `ID` IN (
33362 -- Summon Astromancer Adds
);

UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = 5 WHERE `ID` IN (
23119, -- Conjure Peasant DND
23121  -- Conjure Peasant DND
);

UPDATE `spell_dbc` SET `Effect_1` = 28, `EffectMiscValueB_1` = 64, `EffectBasePoints_1` = 9 WHERE `ID` IN (
25789, -- Summon Yauj Brood
29434  -- Summon Maexxna Spiderling
);

UPDATE `spell_dbc` SET `Effect_2` = 28, `EffectMiscValueB_2` = 64, `EffectBasePoints_2` = 1 WHERE `ID` IN (
21883  -- Summon Healed Celebrian Vine
);

UPDATE `spell_dbc` SET `Effect_2` = 28, `EffectMiscValueB_2` = 64, `EffectBasePoints_2` = 0 WHERE `ID` IN (
23201, -- Hunter Epic Anti-Cheat DND
27939, -- Summon Spectral Rivendare
29110, -- Summon Enraged Mounts
30774, -- Summon Elekk
33614, -- Summon Void Portal B
33616, -- Summon Void Portal E
36616, -- Veneratus Spawn
39074, -- [DND]Rexxars Bird Effect
69868  -- Carrying Beer Barrels [TEST]
);

UPDATE `spell_dbc` SET `Effect_2` = 28, `EffectMiscValueB_2` = 496, `EffectBasePoints_2` = 0 WHERE `ID` IN (
74125  -- Summon Creator Spell Test
);

UPDATE `spell_dbc` SET `Effect_3` = 28, `EffectMiscValueB_3` = 496, `EffectBasePoints_3` = 0 WHERE `ID` IN (
33615  -- Summon Void Portal C
);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` IN (22911,36023,30481,30636,34100,30932);
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(22911, 22911, 37511),
(36023, 36023, 36054),
(30481, 30481, 35945),
(30636, 30636, 35942),
(34100, 34100, 35950),
(30932, 30932, 40248);

DELETE FROM `spelldifficulty_dbc` WHERE `ID` = 31532;
INSERT INTO `spelldifficulty_dbc` (`ID`, `DifficultySpellID_1`, `DifficultySpellID_2`) VALUES
(31532, 31532, 37936);