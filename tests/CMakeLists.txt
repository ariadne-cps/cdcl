include(CTest)
foreach(TEST test_boolean_literal test_cdcl_state test_cdcl_theory test_cdcl_solver)
    add_executable(${TEST} ${TEST}.cpp)
    target_link_libraries(${TEST} PRIVATE ariadne-cdcl cdcl)
    add_test(NAME ${TEST} COMMAND $<TARGET_FILE:${TEST}>)
    if(WIN32)
        set_tests_properties(${TEST} PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:ariadne-cdcl>"
            TIMEOUT 120
        )
    endif()
endforeach()
add_custom_target(tests)
add_dependencies(tests test_boolean_literal test_cdcl_state test_cdcl_theory test_cdcl_solver)
