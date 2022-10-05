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
# User has manually chosen to ignore the git-tests, so throw them a warning.
# This is done EACH compile so they can be alerted about the consequences.
#

#
# Use it like:
# GenerateNodeApplication(master)
#

macro(GenerateNodeApplication dir nodeName)
  CollectSourceFiles(
    ${dir}
    PRIVATE_SOURCES
    # Exclude
    ${dir}/PrecompiledHeaders)

  if (WIN32)
    list(APPEND PRIVATE_SOURCES ${winDebugging} ${winService})
  endif()

  if (MSVC)
    list(APPEND PRIVATE_SOURCES ${dir}/${nodeName}.rc)
  endif()

  if (USE_COREPCH)
    set(PRIVATE_PCH_HEADER ${dir}/PrecompiledHeaders/${nodeName}PCH.h)
  endif()

  GroupSources(${dir})

  add_executable(${nodeName}
    ${PRIVATE_SOURCES})

  add_dependencies(${nodeName} revision.h)

  if (UNIX AND NOT NOJEM)
    set(${nodeName}_LINK_FLAGS "-pthread -lncurses ${${nodeName}_LINK_FLAGS}")
  endif()

  set_target_properties(${nodeName} PROPERTIES LINK_FLAGS "${${nodeName}_LINK_FLAGS}")

  target_link_libraries(${nodeName}
    PRIVATE
      warhead-core-interface
    PUBLIC
      modules
      scripts
      game
      gsoap
      readline
      gperftools)

  CollectIncludeDirectories(
    ${dir}
    PUBLIC_INCLUDES
    # Exclude
    ${dir}/PrecompiledHeaders)

  target_include_directories(${nodeName}
    PUBLIC
      ${PUBLIC_INCLUDES}
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR})

  set_target_properties(${nodeName}
    PROPERTIES
      FOLDER
        "server")

  # Add all dynamic projects as dependency to the ${nodeName}
  if (WORLDSERVER_DYNAMIC_SCRIPT_MODULES_DEPENDENCIES)
    add_dependencies(${nodeName} ${WORLDSERVER_DYNAMIC_SCRIPT_MODULES_DEPENDENCIES})
  endif()

  # Install config
  CopyDefaultConfig(${nodeName})

  if (UNIX)
    install(TARGETS ${nodeName} DESTINATION bin)
  elseif (WIN32)
    install(TARGETS ${nodeName} DESTINATION "${CMAKE_INSTALL_PREFIX}")
  endif()

  # Generate precompiled header
  if (USE_COREPCH)
    add_cxx_pch(${nodeName} ${PRIVATE_PCH_HEADER})
  endif()
endmacro()