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

# An interface library to make the target com available to other targets
add_library(warhead-compile-option-interface INTERFACE)

# Use -std=c++11 instead of -std=gnu++11
set(CXX_EXTENSIONS OFF)

# Enable C++20 support
set(CMAKE_CXX_STANDARD 20)
message(STATUS "Enabled С++20 standard")

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
