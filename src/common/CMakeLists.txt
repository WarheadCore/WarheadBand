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
  ${CMAKE_CURRENT_SOURCE_DIR}/Debugging
  ${CMAKE_CURRENT_SOURCE_DIR}/Platform
  ${CMAKE_CURRENT_SOURCE_DIR}/Collision
  ${CMAKE_CURRENT_SOURCE_DIR}/Navigation
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

if (BUILD_APPLICATION_WORLDSERVER OR BUILD_TOOLS_MAPS)
  unset(PRIVATE_SOURCES)
  CollectSourceFiles(
    ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE_SOURCES
    # Exclude
    ${CMAKE_CURRENT_SOURCE_DIR}/Debugging
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform
    ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)
endif()

# Manually set sources for Debugging directory as we don't want to include WheatyExceptionReport in common project
# It needs to be included both in authserver and worldserver for the static global variable to be properly initialized
# and to handle crash logs on windows
list(APPEND PRIVATE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/Debugging/Errors.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Debugging/Errors.h)

if (USE_COREPCH)
  set(PRIVATE_PCH_HEADER PrecompiledHeaders/commonPCH.h)
endif()

# Group sources
GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-DWARHEAD_API_EXPORT_COMMON)

add_library(common
  ${PRIVATE_SOURCES})

CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

target_include_directories(common
  PUBLIC
    # Provide the binary dir for all child targets
    ${CMAKE_BINARY_DIR}
    ${PUBLIC_INCLUDES}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(common
  PRIVATE
    warhead-core-interface
  PUBLIC
    argon2
    boost
    argon2
    sfmt
    utf8cpp
    openssl
    threads
    jemalloc
    stdfs
    fmt
    valgrind
    indicators::indicators
    WarheadRegex)

if (BUILD_APPLICATION_WORLDSERVER OR BUILD_TOOLS_MAPS)
  target_link_libraries(common
    PUBLIC
      g3dlib
      Detour)
endif()

add_dependencies(common revision.h)

set_target_properties(common
  PROPERTIES
    FOLDER
      "server")

# Install dynamic lib
InstallDynamicLibrary(common)

# Generate precompiled header
if (USE_COREPCH)
  add_cxx_pch(common ${PRIVATE_PCH_HEADER})
endif ()
