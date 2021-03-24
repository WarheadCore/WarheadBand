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

# Set build-directive (used in core to tell which buildtype we used)
target_compile_definitions(warhead-compile-option-interface
  INTERFACE
    -D_BUILD_DIRECTIVE="${CMAKE_BUILD_TYPE}")

if(PLATFORM EQUAL 32)
  target_compile_options(warhead-compile-option-interface
    INTERFACE
      -axSSE2)
else()
  target_compile_options(warhead-compile-option-interface
    INTERFACE
      -xSSE2)
endif()

if(WITH_WARNINGS)
  target_compile_options(warhead-warning-interface
    INTERFACE
      -w1)
  message(STATUS "ICC: All warnings enabled")
endif()

if(WITH_COREDEBUG)
  target_compile_options(warhead-compile-option-interface
    INTERFACE
      -g)
  message(STATUS "ICC: Debug-flag set (-g)")
endif()
