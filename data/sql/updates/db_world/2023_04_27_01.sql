-- DB update 2023_04_27_00 -> 2023_04_27_01

-- Corrects subclass error messages
UPDATE `item_template` SET `subclass`=4 WHERE  `entry`=17;
UPDATE `item_template` SET `subclass`=6 WHERE  `entry`=2556;
UPDATE `item_template` SET `subclass`=0 WHERE  `entry`=20221;
UPDATE `item_template` SET `subclass`=13 WHERE  `entry`=31802;
UPDATE `item_template` SET `subclass`=3 WHERE  `entry`=33080;
UPDATE `item_template` SET `subclass`=3 WHERE  `entry`=33604;
UPDATE `item_template` SET `subclass`=8 WHERE  `entry`=37445;
UPDATE `item_template` SET `subclass`=12 WHERE  `entry`=37677;
UPDATE `item_template` SET `subclass`=7 WHERE  `entry`=41749;
UPDATE `item_template` SET `subclass`=1 WHERE  `entry`=53048;
