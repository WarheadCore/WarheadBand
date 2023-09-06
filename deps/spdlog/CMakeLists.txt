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

CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES)

# Group sources
GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_library(spdlog STATIC ${PRIVATE_SOURCES})

target_compile_definitions(spdlog
  PUBLIC
    SPDLOG_COMPILED_LIB
    SPDLOG_FMT_EXTERNAL)

CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC_INCLUDES)

target_include_directories(spdlog
  PUBLIC
    ${PUBLIC_INCLUDES}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(spdlog
  PRIVATE
    warhead-dependency-interface
  PUBLIC
    fmt
    threads)

set_target_properties(spdlog
  PROPERTIES
    FOLDER
      "deps")