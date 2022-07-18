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

CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

if(USE_COREPCH)
  set(PRIVATE_PCH_HEADER PrecompiledHeaders/databasePCH.h)
endif()

# Group sources
GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-DWARHEAD_API_EXPORT_DATABASE)

add_library(database
  ${PRIVATE_SOURCES})

# Do NOT add any extra include directory unless it does not create unneeded extra dependencies,
# and specially, not add any dependency to neither of these: shared, game, scripts
# This way we ensure that if either a PR does that without modifying this file,
# a compile error will be generated, either this file will be modified so it
# is detected more easily.
# While it is OK to include files from other libs as long as they don't require
# linkage (enums, defines...) it is discouraged to do so unless necessary, as it will pullute
# include_directories leading to further unnoticed dependency aditions
# Linker Depencency requirements: common
CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

target_include_directories(database
  PUBLIC
    ${PUBLIC_INCLUDES}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(database
  PRIVATE
    warhead-core-interface
    mysql
  PUBLIC
    common)

set_target_properties(database
  PROPERTIES
    FOLDER
      "server")

# Install dynamic lib
InstallDynamicLibrary(database)

# Generate precompiled header
if(USE_COREPCH)
  add_cxx_pch(database ${PRIVATE_PCH_HEADER})
endif()
