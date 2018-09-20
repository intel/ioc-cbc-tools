cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME cbc_diagnostic_control_test)
set(PROJECT_NAMESPACE vehicle_bus)
project(${PROJECT_NAME})

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

include ( GNUInstallDirs )

set(EXECUTABLE_OUTPUT_PATH usr/${CMAKE_INSTALL_BINDIR})

# generate file version.h based on version.h.in
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../inc/cbc_diagnostic_control/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/../../inc/cbc_diagnostic_control/generated/version.h
    @ONLY
    )
    
set(PROJECT_SOURCE_LIST
  ${CMAKE_CURRENT_SOURCE_DIR}/src/UnitTestsMain.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/CbcDiagnosticControlFrameHandlerTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/CbcDiagnosticControlOptionsTests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/cbc_diagnostic_control/cbc_diagnostic_control_options.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/cbc_diagnostic_control/cbc_diagnostic_control_frame_handler.c
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_LIST})

target_compile_options(${PROJECT_NAME} PRIVATE)


link_directories(${CMAKE_INSTALL_LIBDIR})

target_link_libraries(${PROJECT_NAME}
  ${GTEST_LIBRARIES} pthread
)

