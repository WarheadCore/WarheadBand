DROP USER IF EXISTS 'warhead'@'localhost';
CREATE USER 'warhead'@'localhost' IDENTIFIED BY 'warhead' WITH MAX_QUERIES_PER_HOUR 0 MAX_CONNECTIONS_PER_HOUR 0 MAX_UPDATES_PER_HOUR 0;

GRANT ALL PRIVILEGES ON * . * TO 'warhead'@'localhost' WITH GRANT OPTION;

CREATE DATABASE `warhead_world` DEFAULT CHARACTER SET UTF8MB4 COLLATE utf8mb4_general_ci;
CREATE DATABASE `warhead_characters` DEFAULT CHARACTER SET UTF8MB4 COLLATE utf8mb4_general_ci;
CREATE DATABASE `warhead_auth` DEFAULT CHARACTER SET UTF8MB4 COLLATE utf8mb4_general_ci;

GRANT ALL PRIVILEGES ON `warhead_world` . * TO 'warhead'@'localhost' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON `warhead_characters` . * TO 'warhead'@'localhost' WITH GRANT OPTION;
GRANT ALL PRIVILEGES ON `warhead_auth` . * TO 'warhead'@'localhost' WITH GRANT OPTION;
