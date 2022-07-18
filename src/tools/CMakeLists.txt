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

# Make the tools list available in the current scope
GetToolsList(TOOLS_BUILD_LIST)

if (TOOLS_BUILD STREQUAL "none")
  set(TOOLS_DEFAULT_BUILD "disabled")
else()
  set(TOOLS_DEFAULT_BUILD "enabled")
endif()

# Sets BUILD_TOOLS_USE_WHITELIST
# Sets BUILD_TOOLS_WHITELIST
if (TOOLS_BUILD MATCHES "-only")
  set(BUILD_TOOLS_USE_WHITELIST ON)

  if (TOOLS_BUILD STREQUAL "maps-only")
    list(APPEND BUILD_TOOLS_WHITELIST map_extractor mmaps_generator vmap4_assembler vmap4_extractor)
  endif()

  if (TOOLS_BUILD STREQUAL "db-only")
    list(APPEND BUILD_TOOLS_WHITELIST dbimport)
  endif()
endif()

# Set the TOOL_${TOOL_BUILD_NAME} variables from the
# variables set above
foreach(TOOL_BUILD_NAME ${TOOLS_BUILD_LIST})
  ToolNameToVariable(${TOOL_BUILD_NAME} TOOL_BUILD_VARIABLE)

  if(${TOOL_BUILD_VARIABLE} STREQUAL "default")
    if(BUILD_TOOLS_USE_WHITELIST)
      list(FIND BUILD_TOOLS_WHITELIST "${TOOL_BUILD_NAME}" INDEX)
      if(${INDEX} GREATER -1)
        set(${TOOL_BUILD_VARIABLE} ${TOOLS_DEFAULT_BUILD})
      else()
        set(${TOOL_BUILD_VARIABLE} "disabled")
      endif()
    else()
      set(${TOOL_BUILD_VARIABLE} ${TOOLS_DEFAULT_BUILD})
    endif()
  endif()

  # Build the Graph values
  if(${TOOL_BUILD_VARIABLE} MATCHES "enabled")
    list(APPEND TOOL_BUILD_GRAPH_KEYS tools)
    set(TOOL_BUILD_VALUE_DISPLAY_tools tools)
    list(APPEND TOOL_BUILD_VALUE_CONTAINS_tools ${TOOL_BUILD_NAME})
  else()
    list(APPEND TOOL_BUILD_GRAPH_KEYS disabled)
    set(TOOL_BUILD_VALUE_DISPLAY_disabled disabled)
    list(APPEND TOOL_BUILD_VALUE_CONTAINS_disabled ${TOOL_BUILD_NAME})
  endif()
endforeach()

list(SORT TOOL_BUILD_GRAPH_KEYS)
list(REMOVE_DUPLICATES TOOL_BUILD_GRAPH_KEYS)

# Display the graphs
message("")
message("* Tools build list (${TOOLS_BUILD}):")
message("  |")

foreach(TOOL_BUILD_GRAPH_KEY ${TOOL_BUILD_GRAPH_KEYS})
  if(NOT TOOL_BUILD_GRAPH_KEY STREQUAL "disabled")
    message("  +- ${TOOL_BUILD_VALUE_DISPLAY_${TOOL_BUILD_GRAPH_KEY}}")
  else()
    message("  |  ${TOOL_BUILD_VALUE_DISPLAY_${TOOL_BUILD_GRAPH_KEY}}")
  endif()
  foreach(TOOL_BUILD_GRAPH_ENTRY ${TOOL_BUILD_VALUE_CONTAINS_${TOOL_BUILD_GRAPH_KEY}})
    message("  |   +- ${TOOL_BUILD_GRAPH_ENTRY}")
  endforeach()
  message("  |")
endforeach()

message("")

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

# Generates the actual tools projects
foreach(TOOL_NAME ${TOOLS_BUILD_LIST})
  GetPathToTool(${TOOL_NAME} SOURCE_TOOL_PATH)
  ToolNameToVariable(${TOOL_NAME} TOOL_BUILD_VARIABLE)

  if (${TOOL_BUILD_VARIABLE} STREQUAL "disabled")
    continue()
  endif()

  unset(TOOL_PRIVATE_SOURCES)
  CollectSourceFiles(
    ${SOURCE_TOOL_PATH}
    TOOL_PRIVATE_SOURCES)

  if (WIN32)
    list(APPEND TOOL_PRIVATE_SOURCES ${winDebugging})
  endif()

  GetProjectNameOfToolName(${TOOL_NAME} TOOL_PROJECT_NAME)

  # Create the application project
  add_executable(${TOOL_PROJECT_NAME}
    ${TOOL_PRIVATE_SOURCES})

  # Need fix errors in maps tools
  # target_link_libraries(${TOOL_PROJECT_NAME}
  #   PRIVATE
  #     warhead-core-interface)

  if (${TOOL_PROJECT_NAME} MATCHES "dbimport")
    target_link_libraries(${TOOL_PROJECT_NAME}
      PUBLIC
        database
      PRIVATE
        warhead-core-interface)

    # Install config
    CopyToolConfig(${TOOL_PROJECT_NAME} ${TOOL_NAME})
  else()
    target_link_libraries(${TOOL_PROJECT_NAME}
      PRIVATE
        warhead-dependency-interface)

    target_link_libraries(${TOOL_PROJECT_NAME}
      PUBLIC
        common
        mpq
        zlib
        Recast
        g3dlib)
  endif()

  unset(TOOL_PUBLIC_INCLUDES)
  CollectIncludeDirectories(
    ${SOURCE_TOOL_PATH}
    TOOL_PUBLIC_INCLUDES)

  target_include_directories(${TOOL_PROJECT_NAME}
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR})

  target_include_directories(${TOOL_PROJECT_NAME}
    PUBLIC
      ${TOOL_PUBLIC_INCLUDES}
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}/${TOOL_NAME})

  set_target_properties(${TOOL_PROJECT_NAME}
    PROPERTIES
      FOLDER
        "tools")

  if (UNIX)
    install(TARGETS ${TOOL_PROJECT_NAME} DESTINATION bin)
  elseif (WIN32)
    install(TARGETS ${TOOL_PROJECT_NAME} DESTINATION "${CMAKE_INSTALL_PREFIX}")
  endif()
endforeach()
