if(NOT EXISTS "${BINARY}")
    message(FATAL_ERROR "Missing no-memory executable")
endif()
# Check imports and the symbol table: a disabled runtime switch or a linked
# reader with no visible UI must not satisfy the no-memory build boundary.
get_filename_component(binary_dir "${BINARY}" DIRECTORY)
get_filename_component(binary_name "${BINARY}" NAME)
# MinGW binutils may decode a non-ASCII absolute argv path incorrectly; the
# native working-directory API plus the ASCII filename avoids that limitation.
execute_process(COMMAND "${OBJDUMP}" -p -t -C "${binary_name}"
    WORKING_DIRECTORY "${binary_dir}"
    RESULT_VARIABLE result OUTPUT_VARIABLE contents ERROR_VARIABLE errors)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Cannot inspect executable: ${errors}")
endif()
if(contents MATCHES "ReadProcessMemory|VirtualQueryEx|NtReadVirtualMemory|ZwReadVirtualMemory|WriteProcessMemory|VirtualAllocEx|CreateRemoteThread|DebugActiveProcess|osu_link::")
    message(FATAL_ERROR "Game-memory access API or osu reader symbol present")
endif()
message(STATUS "No game-memory access imports or osu reader symbols")
