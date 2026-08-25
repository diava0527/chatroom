cmake_minimum_required(VERSION 3.16)
project(browser_chatroom_architecture LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(CROW_USE_BOOST)

if(MSVC)
    add_compile_options(/utf-8)
    add_compile_definitions(_WIN32_WINNT=0x0601)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party/boost_1_78_0
)

add_executable(browser_chatroom_architecture
    src/main.cpp
    "include/storage/user_memory_store_impl.h"
    "include/storage/user_memory_store_impl.cpp"
    "include/user/user_service_impl.h"
    "include/user/user_service_impl.cpp"
 "include/user/auth_service_impl.cpp" "include/user/auth_service_impl.h" "include/websocket/ws_chat_controller_impl.cpp" "include/websocket/ws_chat_controller_impl.h" "include/websocket/ws_connection_manager_impl.cpp" "include/websocket/ws_connection_manager_impl.h" "include/websocket/ws_message_dispatcher_impl.cpp" "include/websocket/ws_message_dispatcher_impl.h")
