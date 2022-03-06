-- account_discord_link
DROP TABLE IF EXISTS `account_discord_link`;
CREATE TABLE IF NOT EXISTS `account_discord_link` (
  `AccountID` INT NOT NULL,
  `DiscordUserID` BIGINT NOT NULL,
  PRIMARY KEY (`AccountID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
