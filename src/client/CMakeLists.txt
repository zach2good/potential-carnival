add_executable(client main.cpp)
target_link_libraries(client PUBLIC ${BearLibTerminal_LIBRARY} asio)
target_include_directories(client PUBLIC ${BearLibTerminal_INCLUDE_DIR})
set_target_output_directory(client)

# Copy DLL
if (WINDOWS)
    add_custom_command(TARGET client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BearLibTerminal_RUNTIME_LIBRARY} ${CMAKE_SOURCE_DIR}/bin
    )
endif()

# Copy Font
add_custom_command(TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/res/PressStart2P-Regular.ttf ${CMAKE_SOURCE_DIR}/bin
)
