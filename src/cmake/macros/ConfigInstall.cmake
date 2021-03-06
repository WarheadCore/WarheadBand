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

#
# Use it like:
# CopyDefaultConfig(worldserver)
#

function(CopyDefaultConfig servertype)
  if(WIN32)
    if("${CMAKE_MAKE_PROGRAM}" MATCHES "MSBuild")
      add_custom_command(TARGET ${servertype}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/configs")
      add_custom_command(TARGET ${servertype}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${servertype}.conf.dist" "${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/configs")
    elseif(MINGW)
      add_custom_command(TARGET ${servertype}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/configs")
      add_custom_command(TARGET ${servertype}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${servertype}.conf.dist ${CMAKE_BINARY_DIR}/bin/configs")
    endif()
  endif()

  if(UNIX)
    install(FILES "${servertype}.conf.dist" DESTINATION "${CONF_DIR}")
  elseif(WIN32)
    install(FILES "${servertype}.conf.dist" DESTINATION "${CMAKE_INSTALL_PREFIX}/configs")
  endif()
endfunction()

#
# Use it like:
# CopyModuleConfig("warhead.conf.dist")
#

function(CopyModuleConfig configDir)
  set(postPath "configs/modules")

  if(WIN32)
    if("${CMAKE_MAKE_PROGRAM}" MATCHES "MSBuild")
      add_custom_command(TARGET worldserver
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/${postPath}")
      add_custom_command(TARGET worldserver
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${configDir}" "${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/${postPath}")
    elseif(MINGW)
      add_custom_command(TARGET worldserver
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/${postPath}")
      add_custom_command(TARGET worldserver
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${configDir} ${CMAKE_BINARY_DIR}/bin/${postPath}")
    endif()
  endif()

  if(UNIX)
    install(FILES "${configDir}" DESTINATION "${CONF_DIR}/modules")
  elseif(WIN32)
    install(FILES "${configDir}" DESTINATION "${CMAKE_INSTALL_PREFIX}/${postPath}")
  endif()
  unset(postPath)
endfunction()

# Stores the absolut path of the given config module in the variable
function(GetPathToModuleConfig module variable)
  set(${variable} "${CMAKE_SOURCE_DIR}/modules/${module}/conf" PARENT_SCOPE)
endfunction()

# Creates a list of all configs modules
# and stores it in the given variable.
function(CollectModulesConfig)
  file(GLOB LOCALE_MODULE_LIST RELATIVE
    ${CMAKE_SOURCE_DIR}/modules
    ${CMAKE_SOURCE_DIR}/modules/*)

  message(STATUS "* Modules config list:")

  foreach(CONFIG_MODULE ${LOCALE_MODULE_LIST})
    GetPathToModuleConfig(${CONFIG_MODULE} MODULE_CONFIG_PATH)

    file(GLOB MODULE_CONFIG_LIST RELATIVE
      ${MODULE_CONFIG_PATH}
      ${MODULE_CONFIG_PATH}/*.conf.dist)

    foreach(configFileName ${MODULE_CONFIG_LIST})
      CopyModuleConfig("${MODULE_CONFIG_PATH}/${configFileName}")
      set(CONFIG_LIST ${CONFIG_LIST}${configFileName},)
      message(STATUS "  |- ${configFileName}")
    endforeach()

  endforeach()
  message("")
  add_definitions(-DWH_MODULE_CONFIG_LIST=$<1:"${CONFIG_LIST}">)
endfunction()
