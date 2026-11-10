# Qt Test unit tests for src/core. Run with: ctest --test-dir <build> --output-on-failure

function(ergopro2_add_test name)
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE ergopro2_core Qt6::Test ergopro2_warnings)
    add_test(NAME ${name} COMMAND ${name})
    # Tests never need a display.
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

ergopro2_add_test(tst_clock)
ergopro2_add_test(tst_logger)
ergopro2_add_test(tst_settings)
ergopro2_add_test(tst_startupmode)
ergopro2_add_test(tst_locationclassifier)
ergopro2_add_test(tst_featureavailability)
ergopro2_add_test(tst_inputevent)
ergopro2_add_test(tst_keycombo)
ergopro2_add_test(tst_dispatcher)
ergopro2_add_test(tst_watchdog)
ergopro2_add_test(tst_autoclick)
ergopro2_add_test(tst_keycontrol)
ergopro2_add_test(tst_breaktimer)
ergopro2_add_test(tst_draglock)
ergopro2_add_test(tst_statistics)
ergopro2_add_test(tst_statisticsstore)

# macOS platform test (Objective-C++, links the platform library): sleep/lock
# notifications after MacPowerEvents::start() returned (1.7.3).
if(APPLE)
    qt_add_executable(tst_macpowerevents tst_macpowerevents.mm)
    target_link_libraries(tst_macpowerevents PRIVATE ergopro2_platform ergopro2_core Qt6::Test ergopro2_warnings
        "-framework AppKit")
    target_compile_options(tst_macpowerevents PRIVATE -fobjc-arc)
    add_test(NAME tst_macpowerevents COMMAND tst_macpowerevents)
    set_tests_properties(tst_macpowerevents PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
