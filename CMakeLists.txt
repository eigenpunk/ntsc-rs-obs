cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

set(NTSCRS_DIR "${CMAKE_SOURCE_DIR}/ntscrs-cbind")
set(RUST_BUILD_MODE "release")
set(RUST_SO_NAME "libntscrs_cbind.a")
set(RUST_SO "${NTSCRS_DIR}/target/${RUST_BUILD_MODE}/${RUST_SO_NAME}")
set(RUST_SO_COMMON "${CMAKE_BINARY_DIR}/${RUST_SO_NAME}")

add_custom_command(
    OUTPUT ${RUST_SO_COMMON}
    COMMAND ${CMAKE_COMMAND} -E env cargo build --${RUST_BUILD_MODE}
             --manifest-path ${NTSCRS_DIR}/Cargo.toml
    COMMAND ${CMAKE_COMMAND} -E copy ${RUST_SO} ${RUST_SO_COMMON}
    WORKING_DIRECTORY ${NTSCRS_DIR}
    COMMENT "Building ntsc-rs and C binding libraries"
    VERBATIM
)
add_custom_target(rust-build ALL DEPENDS ${RUST_SO_COMMON})

add_library(ntscrs STATIC IMPORTED)
set_target_properties(ntscrs PROPERTIES IMPORTED_LOCATION ${RUST_SO_COMMON})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ntscrs)

add_dependencies(${CMAKE_PROJECT_NAME} rust-build)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-main.c)
target_include_directories(
    ${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/ntscrs-cbind)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
