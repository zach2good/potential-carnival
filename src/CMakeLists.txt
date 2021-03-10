add_executable(main main.cpp)
target_link_libraries(main PRIVATE ${BearLibTerminal_LIBRARY})
target_include_directories(main PRIVATE ${BearLibTerminal_INCLUDE_DIR})
set_target_output_directory(main)

# Copy DLL
add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BearLibTerminal_RUNTIME_LIBRARY} ${CMAKE_SOURCE_DIR}
)
