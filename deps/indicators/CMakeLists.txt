add_library(indicators INTERFACE)
add_library(indicators::indicators ALIAS indicators)

target_include_directories(indicators INTERFACE
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

target_link_libraries(indicators INTERFACE threads)

target_link_libraries(indicators
  INTERFACE
    warhead-dependency-interface)

set_target_properties(indicators
  PROPERTIES
    FOLDER
      "deps")