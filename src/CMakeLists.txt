set(ERGOPRO2_RESOURCES_DIR ${PROJECT_SOURCE_DIR}/resources)

# ---------------------------------------------------------------------------
# core: platform-independent logic (QtCore only, no widgets, no OS headers)
# ---------------------------------------------------------------------------
add_library(ergopro2_core STATIC
    core/Clock.h
    core/CrashHandler.cpp core/CrashHandler.h
    core/LogCategories.cpp core/LogCategories.h
    core/LogLevel.cpp core/LogLevel.h
    core/Logger.cpp core/Logger.h
    core/settings/GeneralSettings.cpp core/settings/GeneralSettings.h
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
)
if(APPLE)
    target_sources(ergopro2_platform PRIVATE
        platform/mac/MacDesktopIntegration.mm platform/mac/MacDesktopIntegration.h)
    target_link_libraries(ergopro2_platform PRIVATE "-framework AppKit" "-framework CoreServices")
elseif(WIN32)
    # Windows implementations are added in stages 1 and 2 (platform/win/).
elseif(UNIX)
    # Linux implementations are added in stages 1 and 2 (platform/linux/).
endif()
target_link_libraries(ergopro2_platform PUBLIC ergopro2_core PRIVATE ergopro2_warnings)

# ---------------------------------------------------------------------------
# ErgoPro2 executable (Qt Widgets UI)
# ---------------------------------------------------------------------------
qt_add_executable(ErgoPro2 WIN32 MACOSX_BUNDLE
    main.cpp
    ui/AppController.cpp ui/AppController.h
    ui/ErgoApplication.cpp ui/ErgoApplication.h
    ui/MainWindow.cpp ui/MainWindow.h
    ui/SingleInstance.cpp ui/SingleInstance.h
    ui/Theme.cpp ui/Theme.h
    ui/TrayController.cpp ui/TrayController.h
    ui/tabs/GeneralTab.cpp ui/tabs/GeneralTab.h
    ui/widgets/CardWidget.cpp ui/widgets/CardWidget.h
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
