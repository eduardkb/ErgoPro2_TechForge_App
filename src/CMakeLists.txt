set(ERGOPRO2_RESOURCES_DIR ${PROJECT_SOURCE_DIR}/resources)

# ---------------------------------------------------------------------------
# core: platform-independent logic (QtCore only, no widgets, no OS headers)
# ---------------------------------------------------------------------------
add_library(ergopro2_core STATIC
    core/Clock.h
    core/CrashHandler.cpp core/CrashHandler.h
    core/autoclick/AutoClickEngine.cpp core/autoclick/AutoClickEngine.h
    core/FeatureAvailability.cpp core/FeatureAvailability.h
    core/input/HookWatchdog.cpp core/input/HookWatchdog.h
    core/keycontrol/KeyAction.cpp core/keycontrol/KeyAction.h
    core/keycontrol/KeyBinding.cpp core/keycontrol/KeyBinding.h
    core/keycontrol/KeyComboText.cpp core/keycontrol/KeyComboText.h
    core/keycontrol/KeyComboValidation.cpp core/keycontrol/KeyComboValidation.h
    core/keycontrol/KeyControlEngine.cpp core/keycontrol/KeyControlEngine.h
    core/input/InputDispatcher.cpp core/input/InputDispatcher.h
    core/input/InputEvent.cpp core/input/InputEvent.h
    core/input/KeyCombo.cpp core/input/KeyCombo.h
    core/LocationClassifier.cpp core/LocationClassifier.h
    core/StartupMode.cpp core/StartupMode.h
    core/LogCategories.cpp core/LogCategories.h
    core/LogLevel.cpp core/LogLevel.h
    core/Logger.cpp core/Logger.h
    core/settings/AutoClickSettings.cpp core/settings/AutoClickSettings.h
    core/settings/GeneralSettings.cpp core/settings/GeneralSettings.h
    core/settings/KeyControlSettings.cpp core/settings/KeyControlSettings.h
    core/settings/SettingsRegistry.cpp core/settings/SettingsRegistry.h
    core/settings/SettingsSchema.cpp core/settings/SettingsSchema.h
    core/settings/SettingsStore.cpp core/settings/SettingsStore.h
)
target_include_directories(ergopro2_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/generated)
target_link_libraries(ergopro2_core PUBLIC Qt6::Core PRIVATE ergopro2_warnings)

# ---------------------------------------------------------------------------
# platform: OS abstraction (interfaces + per-OS implementations)
# ---------------------------------------------------------------------------
add_library(ergopro2_platform STATIC
    platform/PlatformFactory.cpp platform/PlatformFactory.h
    platform/PlatformInterfaces.h
    platform/PlatformResult.h
    platform/stub/StubPlatform.cpp platform/stub/StubPlatform.h
    platform/common/FileOperations.cpp platform/common/FileOperations.h
    platform/common/Synthetic.h
)
if(APPLE)
    target_sources(ergopro2_platform PRIVATE
        platform/mac/MacAutostart.cpp platform/mac/MacAutostart.h
        platform/mac/MacDesktopIntegration.mm platform/mac/MacDesktopIntegration.h
        platform/mac/MacInput.mm platform/mac/MacInput.h
        platform/mac/MacInstaller.cpp platform/mac/MacInstaller.h
        platform/mac/MacPermissions.mm platform/mac/MacPermissions.h)
    # Objective-C++ files use ARC (automatic reference counting).
    target_compile_options(ergopro2_platform PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
    target_link_libraries(ergopro2_platform PRIVATE
        "-framework AppKit" "-framework CoreServices" "-framework ApplicationServices" "-framework IOKit"
        "-framework Security")
elseif(WIN32)
    target_sources(ergopro2_platform PRIVATE
        platform/win/WinInput.cpp platform/win/WinInput.h
        platform/win/WinPlatform.cpp platform/win/WinPlatform.h)
    target_link_libraries(ergopro2_platform PRIVATE user32 shcore wtsapi32)
    target_compile_definitions(ergopro2_platform PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
elseif(UNIX)
    target_sources(ergopro2_platform PRIVATE
        platform/linux/LinuxInput.h
        platform/linux/LinuxPlatform.cpp platform/linux/LinuxPlatform.h
        platform/linux/LinuxPowerEvents.cpp
        platform/linux/LinuxX11Input.cpp)

    # X11: XRecord + XTest (libXtst) and Xkb (libX11) - required.
    find_package(X11 REQUIRED)
    if(NOT TARGET X11::Xtst)
        message(FATAL_ERROR "libXtst (XTest + XRecord) development files are missing - install libxtst-dev")
    endif()
    target_link_libraries(ergopro2_platform PRIVATE X11::X11 X11::Xtst)

    # Wayland (best effort): libevdev is optional.
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBEVDEV QUIET IMPORTED_TARGET libevdev)
    endif()
    if(LIBEVDEV_FOUND)
        target_sources(ergopro2_platform PRIVATE platform/linux/LinuxEvdevInput.cpp)
        target_compile_definitions(ergopro2_platform PRIVATE ERGOPRO2_HAVE_LIBEVDEV)
        target_link_libraries(ergopro2_platform PRIVATE PkgConfig::LIBEVDEV)
        message(STATUS "libevdev found: Wayland input support enabled")
    else()
        message(STATUS "libevdev not found: Wayland input support disabled (install libevdev-dev to enable)")
    endif()

    # Sleep / lock notifications over D-Bus (optional).
    find_package(Qt6 QUIET COMPONENTS DBus)
    if(TARGET Qt6::DBus)
        target_compile_definitions(ergopro2_platform PRIVATE ERGOPRO2_HAVE_QTDBUS)
        target_link_libraries(ergopro2_platform PRIVATE Qt6::DBus)
    else()
        message(STATUS "QtDBus not found: no sleep/lock notifications on Linux")
    endif()
endif()
target_link_libraries(ergopro2_platform PUBLIC ergopro2_core PRIVATE ergopro2_warnings)

# ---------------------------------------------------------------------------
# ErgoPro2 executable (Qt Widgets UI)
# ---------------------------------------------------------------------------
qt_add_executable(ErgoPro2 WIN32 MACOSX_BUNDLE
    main.cpp
    ui/AppContext.h
    ui/AppController.cpp ui/AppController.h
    ui/ErgoApplication.cpp ui/ErgoApplication.h
    ui/FeatureAvailabilityService.cpp ui/FeatureAvailabilityService.h
    ui/InputService.cpp ui/InputService.h
    ui/autoclick/AutoClickController.cpp ui/autoclick/AutoClickController.h
    ui/autoclick/AutoClickOverlay.cpp ui/autoclick/AutoClickOverlay.h
    ui/autoclick/AutoClickWorker.cpp ui/autoclick/AutoClickWorker.h
    ui/keycontrol/HotkeyDialog.cpp ui/keycontrol/HotkeyDialog.h
    ui/keycontrol/KeyControlController.cpp ui/keycontrol/KeyControlController.h
    ui/InstallController.cpp ui/InstallController.h
    ui/MainWindow.cpp ui/MainWindow.h
    ui/PermissionActions.cpp ui/PermissionActions.h
    ui/PermissionsDialog.cpp ui/PermissionsDialog.h
    ui/SingleInstance.cpp ui/SingleInstance.h
    ui/TabIcons.cpp ui/TabIcons.h
    ui/Theme.cpp ui/Theme.h
    ui/TrayController.cpp ui/TrayController.h
    ui/tabs/AutoClickTab.cpp ui/tabs/AutoClickTab.h
    ui/tabs/DiagnosticsCard.cpp ui/tabs/DiagnosticsCard.h
    ui/tabs/GeneralTab.cpp ui/tabs/GeneralTab.h
    ui/tabs/InputDiagnosticsCard.cpp ui/tabs/InputDiagnosticsCard.h
    ui/tabs/InstallCard.cpp ui/tabs/InstallCard.h
    ui/tabs/KeyControlTab.cpp ui/tabs/KeyControlTab.h
    ui/widgets/CardWidget.cpp ui/widgets/CardWidget.h
    ui/widgets/FeatureBanner.cpp ui/widgets/FeatureBanner.h
    ui/widgets/FeaturePage.cpp ui/widgets/FeaturePage.h
    ui/widgets/ParameterRow.cpp ui/widgets/ParameterRow.h
    ui/widgets/PlaceholderPage.cpp ui/widgets/PlaceholderPage.h
    ui/widgets/StatusBadge.cpp ui/widgets/StatusBadge.h
    ui/widgets/ToggleSwitch.cpp ui/widgets/ToggleSwitch.h
    ${ERGOPRO2_RESOURCES_DIR}/resources.qrc
)
target_link_libraries(ErgoPro2 PRIVATE
    ergopro2_platform
    ergopro2_core
    Qt6::Widgets
    Qt6::Network
    Qt6::Svg
    ergopro2_warnings)

if(APPLE)
    set(ERGOPRO2_ICNS ${ERGOPRO2_RESOURCES_DIR}/icons/app/ErgoPro2.icns)
    if(EXISTS ${ERGOPRO2_ICNS})
        target_sources(ErgoPro2 PRIVATE ${ERGOPRO2_ICNS})
        set_source_files_properties(${ERGOPRO2_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    else()
        message(WARNING "ErgoPro2.icns missing - run scripts/generate-icons.sh")
    endif()
    set_target_properties(ErgoPro2 PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${ERGOPRO2_RESOURCES_DIR}/macos/Info.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER ${ERGOPRO2_BUNDLE_ID}
        MACOSX_BUNDLE_BUNDLE_NAME ErgoPro2
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_ICON_FILE ErgoPro2.icns)

    # The linker only signs the executable (identifier "ErgoPro2", no sealed
    # resources), which macOS treats as an invalid bundle signature: the
    # Accessibility permission is then never recognized (fixed in 1.1.2+).
    # Sign the whole bundle with its bundle identifier after every build.
    if(ERGOPRO2_CODESIGN_IDENTITY)
        find_program(ERGOPRO2_CODESIGN codesign REQUIRED)
        add_custom_command(TARGET ErgoPro2 POST_BUILD
            COMMAND ${ERGOPRO2_CODESIGN} --force --sign "${ERGOPRO2_CODESIGN_IDENTITY}" --timestamp=none
                    "$<TARGET_BUNDLE_DIR:ErgoPro2>"
            COMMENT "Signing ErgoPro2.app (identity: ${ERGOPRO2_CODESIGN_IDENTITY})"
            VERBATIM)
    else()
        message(WARNING "ERGOPRO2_CODESIGN_IDENTITY is empty: ErgoPro2.app is not signed; macOS will not "
                        "recognize the Accessibility permission (see documentation/BUILD.md 1.6)")
    endif()
elseif(WIN32)
    set(ERGOPRO2_ICO_PATH ${ERGOPRO2_RESOURCES_DIR}/icons/app/ErgoPro2.ico)
    configure_file(${ERGOPRO2_RESOURCES_DIR}/windows/ErgoPro2.rc.in
                   ${CMAKE_BINARY_DIR}/generated/ErgoPro2.rc @ONLY)
    target_sources(ErgoPro2 PRIVATE ${CMAKE_BINARY_DIR}/generated/ErgoPro2.rc)
endif()

# ---------------------------------------------------------------------------
# Install rules (used by packaging; full packaging arrives in stage 7)
# ---------------------------------------------------------------------------
install(TARGETS ErgoPro2
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(UNIX AND NOT APPLE)
    install(FILES ${ERGOPRO2_RESOURCES_DIR}/linux/ergopro2.desktop
            DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES ${ERGOPRO2_RESOURCES_DIR}/icons/app/ergopro2.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
endif()
