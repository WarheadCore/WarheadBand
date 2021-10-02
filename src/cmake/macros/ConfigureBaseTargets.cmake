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

# An interface library to make the target com available to other targets
add_library(warhead-compile-option-interface INTERFACE)

# Use -std=c++11 instead of -std=gnu++11
set(CXX_EXTENSIONS OFF)

if (USE_CPP_20)
  # Enable support С++20
  set(CMAKE_CXX_STANDARD 20)
  message(STATUS "Enabled С++20 standard")
else()
  # Enable support С++17
  set(CMAKE_CXX_STANDARD 17)
  message(STATUS "Enabled С++17 standard")
endif()

# An interface library to make the warnings level available to other targets
# This interface taget is set-up through the platform specific script
add_library(warhead-warning-interface INTERFACE)

# An interface used for all other interfaces
add_library(warhead-default-interface INTERFACE)

target_link_libraries(warhead-default-interface
  INTERFACE
    warhead-compile-option-interface)

# An interface used for silencing all warnings
add_library(warhead-no-warning-interface INTERFACE)

if (MSVC)
  target_compile_options(warhead-no-warning-interface
    INTERFACE
      /W0)
else()
  target_compile_options(warhead-no-warning-interface
    INTERFACE
      -w)
endif()

# An interface library to change the default behaviour
# to hide symbols automatically.
add_library(warhead-hidden-symbols-interface INTERFACE)

# An interface amalgamation which provides the flags and definitions
# used by the dependency targets.
add_library(warhead-dependency-interface INTERFACE)
target_link_libraries(warhead-dependency-interface
  INTERFACE
    warhead-default-interface
    warhead-no-warning-interface
    warhead-hidden-symbols-interface)

# An interface amalgamation which provides the flags and definitions
# used by the core targets.
add_library(warhead-core-interface INTERFACE)
target_link_libraries(warhead-core-interface
  INTERFACE
    warhead-default-interface
    warhead-warning-interface)
