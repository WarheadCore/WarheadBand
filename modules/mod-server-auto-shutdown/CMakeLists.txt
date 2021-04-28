#
# Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
# Copyright (C) 2021+ WarheadCore <https://github.com/WarheadCore>
#

# Add scripts to script loader
# AC_ADD_SCRIPT_LOADER("ServerAutoShutdown" "${CMAKE_CURRENT_LIST_DIR}/src/server_auto_shutdown_loader.h")

# Add config file
AC_ADD_CONFIG_FILE("${CMAKE_CURRENT_LIST_DIR}/conf/ServerAutoShutdown.conf.dist")
