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
# InstallDynamicLibrary(common)
#

function(InstallDynamicLibrary libName)
  if (BUILD_SHARED_LIBS)
    if (UNIX)
      install(TARGETS ${libName} LIBRARY DESTINATION lib)
    elseif (WIN32)
      install(TARGETS ${libName} RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}")
    endif()
  endif()
endfunction()
