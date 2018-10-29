cmake_minimum_required(VERSION 2.8)

set(CBC_VERSION_MAJOR 1)
set(CBC_VERSION_MINOR 0)
set(CBC_VERSION_REVISION 0)

set(PROJECT_NAME cbc_logging_service)
set(PROJECT_NAMESPACE vehicle_bus)
project(${PROJECT_NAME})

include ( GNUInstallDirs )

set(EXECUTABLE_OUTPUT_PATH usr/${CMAKE_INSTALL_BINDIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(PkgConfig REQUIRED)
pkg_check_modules(DLTCPP REQUIRED automotive-dlt-c++)

set(PROJECT_SOURCE_LIST
  ${CMAKE_CURRENT_SOURCE_DIR}/private/src/cbc_logging_service/cbc_logging_service_options.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/private/src/cbc_logging_service/cbc_logging_service_main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/private/src/cbc_logging_service/cbc_logging_service.cpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_LIST})

target_compile_options(${PROJECT_NAME} PRIVATE)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)

link_directories(${CMAKE_INSTALL_LIBDIR})

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  dl
  ${DLTCPP_LIBRARIES}
)
