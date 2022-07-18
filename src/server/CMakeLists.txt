#
# This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU Affero General Public License as published by the
# Free Software Foundation; either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.
#

add_subdirectory(apps)

if ((APPS_BUILD AND NOT APPS_BUILD STREQUAL "none") OR BUILD_TOOLS_DB_IMPORT)
  add_subdirectory(database)
endif()

if (BUILD_APPLICATION_AUTHSERVER OR BUILD_APPLICATION_WORLDSERVER)
  add_subdirectory(shared)
endif()

if (BUILD_APPLICATION_WORLDSERVER)
  add_subdirectory(game)
  add_subdirectory(scripts)
endif()
