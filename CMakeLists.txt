cmake_minimum_required(VERSION 3.20)

project(KeyNoteOverlayCpp LANGUAGES CXX)

add_executable(key_note_overlay_cpp WIN32
    src/key_note_overlay_cpp.cpp
    src/pattern_difficulty.cpp
    src/mania_strain.cpp
    src/dan_rating.cpp
    src/session_report.cpp
)

target_compile_features(key_note_overlay_cpp PRIVATE cxx_std_17)
target_compile_definitions(key_note_overlay_cpp PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)
target_link_libraries(key_note_overlay_cpp PRIVATE
    user32
    gdi32
    comctl32
    shell32
    comdlg32
)

if(MSVC)
    target_compile_options(key_note_overlay_cpp PRIVATE /utf-8)
endif()

if(MINGW)
    target_compile_options(key_note_overlay_cpp PRIVATE -Wall -Wextra)
    target_link_options(key_note_overlay_cpp PRIVATE -static -static-libgcc -static-libstdc++)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(game_overlay_test tests/game_overlay_test.cpp)
    target_compile_features(game_overlay_test PRIVATE cxx_std_17)
    target_include_directories(game_overlay_test PRIVATE src)
    target_compile_definitions(game_overlay_test PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(game_overlay_test PRIVATE user32)
    if(MINGW)
        target_compile_options(game_overlay_test PRIVATE -Wall -Wextra)
        target_link_options(game_overlay_test PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/selftest/game")
    add_test(NAME game_overlay_native COMMAND game_overlay_test)
    set_tests_properties(game_overlay_native PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/selftest/game" TIMEOUT 20)
    add_executable(session_report_test tests/session_report_test.cpp src/session_report.cpp src/pattern_difficulty.cpp src/mania_strain.cpp src/dan_rating.cpp)
    target_compile_features(session_report_test PRIVATE cxx_std_17)
    target_include_directories(session_report_test PRIVATE src)
    if(MSVC)
        target_compile_options(session_report_test PRIVATE /utf-8)
    endif()
    if(MINGW)
        target_compile_options(session_report_test PRIVATE -Wall -Wextra)
        target_link_options(session_report_test PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    add_test(NAME session_report COMMAND session_report_test)
    set_tests_properties(session_report PROPERTIES TIMEOUT 30)
    add_executable(pattern_features EXCLUDE_FROM_ALL tools/pattern_features.cpp src/pattern_difficulty.cpp src/mania_strain.cpp)
    target_compile_features(pattern_features PRIVATE cxx_std_17)
    target_include_directories(pattern_features PRIVATE src)
    if(MINGW)
        target_link_options(pattern_features PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    add_executable(pattern_difficulty_test tests/pattern_difficulty_test.cpp src/pattern_difficulty.cpp src/mania_strain.cpp src/dan_rating.cpp)
    target_compile_features(pattern_difficulty_test PRIVATE cxx_std_17)
    target_include_directories(pattern_difficulty_test PRIVATE src)
    if(MINGW)
        target_compile_options(pattern_difficulty_test PRIVATE -Wall -Wextra)
        target_link_options(pattern_difficulty_test PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    add_test(NAME pattern_difficulty COMMAND pattern_difficulty_test)
    set_tests_properties(pattern_difficulty PROPERTIES TIMEOUT 20)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/selftest/core" "${CMAKE_CURRENT_BINARY_DIR}/selftest/ui" "${CMAKE_CURRENT_BINARY_DIR}/selftest/lifecycle"
        "${CMAKE_CURRENT_BINARY_DIR}/selftest/report-failure" "${CMAKE_CURRENT_BINARY_DIR}/selftest/exception")
    add_test(NAME overlay_config_and_input COMMAND key_note_overlay_cpp --self-test)
    set_tests_properties(overlay_config_and_input PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/selftest/core" TIMEOUT 20)
    add_test(NAME overlay_settings_workflow COMMAND key_note_overlay_cpp --ui-self-test)
    set_tests_properties(overlay_settings_workflow PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/selftest/ui" TIMEOUT 20)
    add_test(NAME overlay_report_failure_recovery COMMAND key_note_overlay_cpp --ui-self-test --report-output-blocked)
    set_tests_properties(overlay_report_failure_recovery PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/selftest/report-failure" TIMEOUT 20)
    add_test(NAME overlay_selftest_exception_boundary COMMAND key_note_overlay_cpp --self-test-exception)
    set_tests_properties(overlay_selftest_exception_boundary PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/selftest/exception" TIMEOUT 20 WILL_FAIL TRUE)
    add_test(NAME overlay_tray_lifecycle COMMAND key_note_overlay_cpp --lifecycle-self-test)
    set_tests_properties(overlay_tray_lifecycle PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/selftest/lifecycle" TIMEOUT 20)
endif()
