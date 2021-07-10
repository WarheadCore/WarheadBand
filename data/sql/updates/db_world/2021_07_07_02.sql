-- DB update 2021_07_07_01 -> 2021_07_07_02
DROP PROCEDURE IF EXISTS `updateDb`;
DELIMITER //
CREATE PROCEDURE updateDb ()
proc:BEGIN DECLARE OK VARCHAR(100) DEFAULT 'FALSE';
SELECT COUNT(*) INTO @COLEXISTS
FROM information_schema.COLUMNS
WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'version_db_world' AND COLUMN_NAME = '2021_07_07_01';
IF @COLEXISTS = 0 THEN LEAVE proc; END IF;
START TRANSACTION;
ALTER TABLE version_db_world CHANGE COLUMN 2021_07_07_01 2021_07_07_02 bit;
SELECT sql_rev INTO OK FROM version_db_world WHERE sql_rev = '1625291253501808914'; IF OK <> 'FALSE' THEN LEAVE proc; END IF;
--
-- START UPDATING QUERIES
--

INSERT INTO `version_db_world` (`sql_rev`) VALUES ('1625291253501808914');

DELETE FROM `reference_loot_template` WHERE `Entry` = 11105 AND `Item` = 13888;
INSERT INTO `reference_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(11105, 13888, 0, 0, 0, 1, 1, 1, 1, 'Darkclaw Lobster');

--
-- END UPDATING QUERIES
--
UPDATE version_db_world SET date = '2021_07_07_02' WHERE sql_rev = '1625291253501808914';
COMMIT;
END //
DELIMITER ;
CALL updateDb();
DROP PROCEDURE IF EXISTS `updateDb`;
