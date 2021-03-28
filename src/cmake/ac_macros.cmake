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
# AC_ADD_SCRIPT
#

MACRO(AC_ADD_SCRIPT path)
    CU_ADD_GLOBAL("AC_SCRIPTS_SOURCES" "${path}")
ENDMACRO()

#
# AC_ADD_SCRIPT_LOADER
#

MACRO(AC_ADD_SCRIPT_LOADER script_dec include)
    set (lower_prio_scripts ${ARGN})
    list(LENGTH lower_prio_scripts num_lower_prio_scripts)
    if (${num_lower_prio_scripts} GREATER 0)
        CU_GET_GLOBAL("AC_ADD_SCRIPTS_LIST")
        foreach(lower_prio_script ${lower_prio_scripts})
            if ("${AC_ADD_SCRIPTS_LIST}" MATCHES "Add${lower_prio_script}Scripts()")
                message("-- ${script_dec} demands lower priority: ${lower_prio_script} --")
                list(REMOVE_ITEM AC_ADD_SCRIPTS_LIST "Add${lower_prio_script}Scripts()")
                CU_SET_GLOBAL("AC_ADD_SCRIPTS_LIST" "${AC_ADD_SCRIPTS_LIST}")
                list(APPEND removed_lower_prio_scripts ${lower_prio_script})
            endif()
        endforeach()
        CU_ADD_GLOBAL("AC_ADD_SCRIPTS_LIST" "Add${script_dec}Scripts()\;")
        foreach(lower_prio_script ${removed_lower_prio_scripts})
            CU_ADD_GLOBAL("AC_ADD_SCRIPTS_LIST" "Add${lower_prio_script}Scripts()\;")
        endforeach()
    else()
        CU_ADD_GLOBAL("AC_ADD_SCRIPTS_LIST" "Add${script_dec}Scripts()\;")
    endif()

    if (NOT ${include} STREQUAL "")
        CU_GET_GLOBAL("AC_ADD_SCRIPTS_INCLUDE")
        if (NOT ";${AC_ADD_SCRIPTS_INCLUDE};" MATCHES ";${include};")
            CU_ADD_GLOBAL("AC_ADD_SCRIPTS_INCLUDE" "${include}\;")
        endif()
    endif()
ENDMACRO()

#
# AC_ADD_CONFIG_FILE
#
MACRO(AC_ADD_CONFIG_FILE configFilePath)
    CU_GET_GLOBAL("MODULE_CONFIG_FILE_LIST")
    CU_ADD_GLOBAL("MODULE_CONFIG_FILE_LIST" "${configFilePath}")
ENDMACRO()

#
#   WH_ADD_DEF_SCRIPT_LOADER
#

macro(WH_ADD_DEF_SCRIPT_LOADER script_dec)
    CU_GET_GLOBAL("WH_DEF_SCRIPTS_LIST")
    CU_ADD_GLOBAL("WH_DEF_SCRIPTS_LIST" "void Add${script_dec}Scripts();")
endmacro()

#
#   WH_ADD_MODULES_SOURCE
#

macro(WH_ADD_MODULES_SOURCE scriptName)
    CU_SUBDIRLIST(sub_DIRS ${CMAKE_CURRENT_LIST_DIR}/src/ TRUE TRUE)

    WH_ADD_DEF_SCRIPT_LOADER("${scriptName}")
    WH_ADD_SCRIPT_LOADER("${scriptName}" "")
    message(STATUS "  -> Prepared module script: ${scriptName}")
ENDMACRO()

#
#   WH_ADD_MODULE
#

macro(WH_ADD_MODULE modulename)
    CU_GET_GLOBAL("WH_MODULE_LIST")
    CU_ADD_GLOBAL("WH_MODULE_LIST" "${modulename}")
endmacro()
