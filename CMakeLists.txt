cmake_minimum_required(VERSION 3.20)

project(KeyNoteOverlayCpp LANGUAGES CXX)

add_executable(key_note_overlay_cpp WIN32
    src/key_note_overlay_cpp.cpp
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
)

if(MINGW)
    target_compile_options(key_note_overlay_cpp PRIVATE -Wall -Wextra)
    target_link_options(key_note_overlay_cpp PRIVATE -static -static-libgcc -static-libstdc++)
endif()
