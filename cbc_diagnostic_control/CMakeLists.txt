cmake_minimum_required(VERSION 2.8)

set(CBC_VERSION_MAJOR 1)
set(CBC_VERSION_MINOR 0)
set(CBC_VERSION_REVISION 0)

set(PROJECT_NAME cbc_diagnostic_control)
set(PROJECT_NAMESPACE vehicle_bus)
project(${PROJECT_NAME})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -z noexecstack -z relro -z now -pie")

include ( GNUInstallDirs )

# generate file version.h based on version.h.in
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/private/inc/cbc_diagnostic_control/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/private/inc/cbc_diagnostic_control/generated/version.h
    )

set(EXECUTABLE_OUTPUT_PATH usr/${CMAKE_INSTALL_BINDIR})

set(PROJECT_SOURCE_LIST
  ${CMAKE_CURRENT_SOURCE_DIR}/private/src/cbc_diagnostic_control/cbc_diagnostic_control_frame_handler.c
  ${CMAKE_CURRENT_SOURCE_DIR}/private/src/cbc_diagnostic_control/cbc_diagnostic_control_main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/private/src/cbc_diagnostic_control/cbc_diagnostic_control_options.c
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_LIST})

target_compile_options(${PROJECT_NAME} PRIVATE -fstack-protector-strong -fPIC -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)

link_directories(${CMAKE_INSTALL_LIBDIR})

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
