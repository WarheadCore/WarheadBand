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

if ((USE_COREPCH OR USE_SCRIPTPCH) AND (CMAKE_C_COMPILER_LAUNCHER STREQUAL "ccache" OR CMAKE_CXX_COMPILER_LAUNCHER STREQUAL "ccache"))
  message(STATUS "Clang: disable pch timestamp when ccache and pch enabled")
  # TODO: for ccache https://github.com/ccache/ccache/issues/539
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fno-pch-timestamp")
endif()

# Set build-directive (used in core to tell which buildtype we used)
target_compile_definitions(warhead-compile-option-interface
  INTERFACE
    -D_BUILD_DIRECTIVE="${CMAKE_BUILD_TYPE}")

set(CLANG_EXPECTED_VERSION 10.0.0)

if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS CLANG_EXPECTED_VERSION)
  message(FATAL_ERROR "Clang: WarheadCore requires version ${CLANG_EXPECTED_VERSION} to build but found ${CMAKE_CXX_COMPILER_VERSION}")
else()
  message(STATUS "Clang: Minimum version required is ${CLANG_EXPECTED_VERSION}, found ${CMAKE_CXX_COMPILER_VERSION} - ok!")
endif()

if(WITH_WARNINGS)
  target_compile_options(warhead-warning-interface
    INTERFACE
      -W
      -Wall
      -Wextra
      -Winit-self
      -Wfatal-errors
      -Wno-mismatched-tags
      -Woverloaded-virtual)
  message(STATUS "Clang: All warnings enabled")
endif()

if(WITH_COREDEBUG)
  target_compile_options(warhead-compile-option-interface
    INTERFACE
      -g3)
  message(STATUS "Clang: Debug-flags set (-g3)")
endif()

# -Wno-narrowing needed to suppress a warning in g3d
# -Wno-deprecated-register is needed to suppress gsoap warnings on Unix systems.
target_compile_options(warhead-compile-option-interface
  INTERFACE
    -Wno-narrowing
    -Wno-deprecated-register)

if(BUILD_SHARED_LIBS)
    # -fPIC is needed to allow static linking in shared libs.
    # -fvisibility=hidden sets the default visibility to hidden to prevent exporting of all symbols.
    target_compile_options(warhead-compile-option-interface
      INTERFACE
        -fPIC)

    target_compile_options(warhead-hidden-symbols-interface
      INTERFACE
        -fvisibility=hidden)

    # --no-undefined to throw errors when there are undefined symbols
    # (caused through missing WARHEAD_*_API macros).
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --no-undefined")

    message(STATUS "Clang: Disallow undefined symbols")
  endif()
