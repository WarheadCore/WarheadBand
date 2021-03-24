#
# This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# User has manually chosen to ignore the git-tests, so throw them a warning.
# This is done EACH compile so they can be alerted about the consequences.
#

set(scripts_STAT_SRCS
  ${scripts_STAT_SRCS}
  ${AC_SCRIPTS_DIR}/Events/brewfest.cpp
  ${AC_SCRIPTS_DIR}/Events/childrens_week.cpp
  ${AC_SCRIPTS_DIR}/Events/hallows_end.cpp
  ${AC_SCRIPTS_DIR}/Events/pilgrims_bounty.cpp
  ${AC_SCRIPTS_DIR}/Events/winter_veil.cpp
  ${AC_SCRIPTS_DIR}/Events/love_in_air.cpp
  ${AC_SCRIPTS_DIR}/Events/midsummer.cpp
)

AC_ADD_SCRIPT_LOADER("Event" "ScriptLoader.h")

message("  -> Prepared: Events")
