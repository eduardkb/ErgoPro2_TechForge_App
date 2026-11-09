cmake_minimum_required(VERSION 3.21)

# ---------------------------------------------------------------------------
# The project VERSION below is the SINGLE SOURCE OF TRUTH for the app version.
#   - A new stage bumps MINOR and resets PATCH (1.N.0).
#   - A bug-fix run bumps PATCH only.
#   - MAJOR stays 1.
# ---------------------------------------------------------------------------
project(ErgoPro2
    VERSION 1.0.0
    DESCRIPTION "Ergonomic helper: auto click, key control and break reminders"
    HOMEPAGE_URL "https://techforge.com/ergopro2"
    LANGUAGES CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OBJCXX_STANDARD 20)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# All executables (app, tests, tools) land in <build>/bin.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(ERGOPRO2_BUILD_TESTS "Build the unit tests" ON)
option(ERGOPRO2_BUILD_TOOLS "Build developer tools (icon generator)" OFF)

set(ERGOPRO2_ORGANIZATION "techforge")
set(ERGOPRO2_BUNDLE_ID "com.techforge.ergopro2")

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Network Svg)
if(ERGOPRO2_BUILD_TESTS)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
endif()

qt_standard_project_setup()
# qt_standard_project_setup() enables AUTOMOC/AUTOUIC only; resources/*.qrc need AUTORCC.
set(CMAKE_AUTORCC ON)

# Shared compiler warnings, linked PRIVATE into every target of the project.
add_library(ergopro2_warnings INTERFACE)
if(MSVC)
    target_compile_options(ergopro2_warnings INTERFACE /W4 /permissive- /utf-8)
else()
    target_compile_options(ergopro2_warnings INTERFACE -Wall -Wextra)
endif()

# Generated version header (src/core/Version.h.in -> <build>/generated/ergopro2/Version.h)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/Version.h.in
    ${CMAKE_BINARY_DIR}/generated/ergopro2/Version.h
    @ONLY)

add_subdirectory(src)

if(ERGOPRO2_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(ERGOPRO2_BUILD_TOOLS)
    add_subdirectory(tools/icongen)
endif()
