#------------------------------------------------------------------------------
# External dependencies
#------------------------------------------------------------------------------
# None for now

#------------------------------------------------------------------------------
# Build benchmark Library
#------------------------------------------------------------------------------
add_executable(fabric_server
        fabric_server.cc)
target_link_libraries(fabric_server thallium
        ${libfabric_LIBRARIES} ${HermesShm_LIBRARIES} yaml-cpp -ldl -lrt -lc)

add_executable(fabric_client
        fabric_client.cc)
target_link_libraries(fabric_client thallium
        ${libfabric_LIBRARIES} ${HermesShm_LIBRARIES} yaml-cpp -ldl -lrt -lc)

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
install(
  FILES
    ${FABRIC_HEADERS}
  DESTINATION
    ${FABRIC_INSTALL_INCLUDE_DIR}
  COMPONENT
    headers
)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install
#-----------------------------------------------------------------------------
install(
  TARGETS
        fabric_client
        fabric_server
  LIBRARY DESTINATION ${FABRIC_INSTALL_LIB_DIR}
  ARCHIVE DESTINATION ${FABRIC_INSTALL_LIB_DIR}
  RUNTIME DESTINATION ${FABRIC_INSTALL_BIN_DIR}
)

#------------------------------------------------------------------------------
# Coverage
#------------------------------------------------------------------------------
if(FABRIC_ENABLE_COVERAGE)
  set_coverage_flags(FABRIC_client)
  set_coverage_flags(FABRIC_runtime)
  set_coverage_flags(FABRIC_start_runtime)
  set_coverage_flags(FABRIC_stop_runtime)
endif()