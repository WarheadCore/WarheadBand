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

add_library(libdpp STATIC
  ${PRIVATE_SOURCES})

target_compile_definitions(libdpp
  PUBLIC
    -DDPP_BUILD
    -DWIN32_LEAN_AND_MEAN
    -D_WINSOCK_DEPRECATED_NO_WARNINGS
    -D_CRT_NONSTDC_NO_DEPRECATE)

if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /Od /DEBUG /Zi /sdl /MP /DFD_SETSIZE=1024")
endif()

target_include_directories(libdpp
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(libdpp
  PRIVATE
    warhead-dependency-interface
    zlib
    fmt
    openssl
    threads)

set_target_properties(libdpp
  PROPERTIES
    FOLDER
    "deps")
