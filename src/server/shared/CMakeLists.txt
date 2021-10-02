#
<<<<<<< HEAD
# This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
=======
# This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
>>>>>>> master
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
<<<<<<< HEAD
#
# User has manually chosen to ignore the git-tests, so throw them a warning.
# This is done EACH compile so they can be alerted about the consequences.
=======
>>>>>>> master
#

CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

if(USE_COREPCH)
  set(PRIVATE_PCH_HEADER PrecompiledHeaders/sharedPCH.h)
endif()

# Group sources
GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-DWARHEAD_API_EXPORT_SHARED)

add_library(shared
  ${PRIVATE_SOURCES})

CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

target_include_directories(shared
  PUBLIC
    ${PUBLIC_INCLUDES}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(shared
  PRIVATE
    warhead-core-interface
  PUBLIC
    database)

set_target_properties(shared
  PROPERTIES
    FOLDER
      "server")

# Install dynamic lib
InstallDynamicLibrary(shared)

# Generate precompiled header
if(USE_COREPCH)
  add_cxx_pch(shared ${PRIVATE_PCH_HEADER})
endif()