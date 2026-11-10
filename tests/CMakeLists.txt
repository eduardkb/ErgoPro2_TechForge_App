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
