cmake_minimum_required(VERSION 3.21)

# ---------------------------------------------------------------------------
# The project VERSION below is the SINGLE SOURCE OF TRUTH for the app version.
#   - A new stage bumps MINOR and resets PATCH (1.N.0).
#   - A bug-fix run bumps PATCH only.
#   - MAJOR stays 1.
# ---------------------------------------------------------------------------
project(ErgoPro2
    VERSION 1.6.0
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
# macOS: identity used to sign ErgoPro2.app after every build. "-" = ad-hoc;
# set it to a certificate name (e.g. "ErgoPro2 Local Signing", BUILD.md 1.6.3)
# so macOS keeps the Accessibility / Input Monitoring permissions across rebuilds.
# Empty = do not sign (the linker's partial signature is then invalid for the bundle).
set(ERGOPRO2_CODESIGN_IDENTITY "-" CACHE STRING "macOS code signing identity for ErgoPro2.app (\"-\" = ad-hoc)")

set(ERGOPRO2_ORGANIZATION "techforge")
set(ERGOPRO2_BUNDLE_ID "com.techforge.ergopro2")

# macOS 26 SDK (Xcode 26) no longer contains AGL.framework, but Qt 6.8.x
# (FindWrapOpenGL.cmake) still links "-framework AGL": its find_library() finds
# AGL in the running system (/System/Library/Frameworks), not in the SDK the
# linker uses, so linking fails with "framework 'AGL' not found". When the SDK
# has no AGL, point Qt's cache variable at OpenGL.framework instead (AGL was a
# deprecated OpenGL helper; linking OpenGL twice is harmless). No effect on SDKs
# that still ship AGL or on Qt versions that no longer use it.
if(APPLE AND NOT DEFINED CACHE{WrapOpenGL_AGL})
    set(_ergopro2_sdk "${CMAKE_OSX_SYSROOT}")
    if(NOT IS_DIRECTORY "${_ergopro2_sdk}")
        execute_process(COMMAND xcrun --show-sdk-path
                        OUTPUT_VARIABLE _ergopro2_sdk OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    endif()
    if(IS_DIRECTORY "${_ergopro2_sdk}/System/Library/Frameworks/OpenGL.framework"
       AND NOT IS_DIRECTORY "${_ergopro2_sdk}/System/Library/Frameworks/AGL.framework")
        set(WrapOpenGL_AGL "${_ergopro2_sdk}/System/Library/Frameworks/OpenGL.framework"
            CACHE FILEPATH "Replacement for AGL.framework, which this macOS SDK no longer contains")
        message(STATUS "macOS SDK without AGL.framework (${_ergopro2_sdk}): Qt's AGL link replaced by OpenGL.framework")
    endif()
endif()

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
