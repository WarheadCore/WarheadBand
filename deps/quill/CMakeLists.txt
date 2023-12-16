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
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  PRIVATE_SOURCES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/src/bundled)

# Group sources
GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_library(quill STATIC ${PRIVATE_SOURCES})

target_compile_definitions(quill
  PUBLIC
    QUILL_FMT_EXTERNAL
    QUILL_DISABLE_NON_PREFIXED_MACROS)

if (MINGW)
  # strftime requires this when using MinGw to correctly format the time ..
  target_link_libraries(quill PUBLIC ucrtbase)
endif ()

# Compiler options
target_compile_options(quill PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
    -Wall -Wextra -Wconversion -pedantic -Wfatal-errors -Wno-unused-private-field -Wno-gnu-zero-variadic-macro-arguments -Wno-unused-parameter>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /wd4324 /wd4189 /wd4996 /wd4100 /wd4127 /wd4702>)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(quill PUBLIC /EHsc)
  target_compile_options(quill} PRIVATE /WX)
  target_compile_definitions(quill PRIVATE _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
endif ()

target_include_directories(quill
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # Exclude
    ${CMAKE_CURRENT_SOURCE_DIR}/include/bundled)

target_link_libraries(quill
  PRIVATE
    warhead-dependency-interface
    fmt
    threads)
