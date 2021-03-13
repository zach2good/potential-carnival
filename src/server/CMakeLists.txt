add_executable(server main.cpp server.h)
target_include_directories(server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(server PUBLIC util asio)
set_target_output_directory(server)
