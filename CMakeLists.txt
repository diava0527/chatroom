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

option(CHATROOM_ENABLE_MYSQL "Build MySQL-backed storage support" ON)

if(CHATROOM_ENABLE_MYSQL)
    add_library(chatroom_mysql STATIC
        include/storage/mysql/mysql_database.cpp
    )

    target_include_directories(chatroom_mysql
        PUBLIC ${CMAKE_SOURCE_DIR}/include
    )

    if(WIN32)
        set(
            MYSQL_ROOT_DIR
            "C:/Program Files/MySQL/MySQL Server 8.0"
            CACHE PATH
            "MySQL Server or Connector/C installation directory"
        )

        find_path(
            MYSQL_INCLUDE_DIR
            NAMES mysql.h
            HINTS "${MYSQL_ROOT_DIR}/include"
        )
        find_library(
            MYSQL_CLIENT_LIBRARY
            NAMES libmysql
            HINTS "${MYSQL_ROOT_DIR}/lib"
        )
        find_file(
            MYSQL_CLIENT_RUNTIME
            NAMES libmysql.dll
            HINTS "${MYSQL_ROOT_DIR}/lib" "${MYSQL_ROOT_DIR}/bin"
        )
        find_file(
            MYSQL_SSL_RUNTIME
            NAMES libssl-3-x64.dll
            HINTS "${MYSQL_ROOT_DIR}/bin"
        )
        find_file(
            MYSQL_CRYPTO_RUNTIME
            NAMES libcrypto-3-x64.dll
            HINTS "${MYSQL_ROOT_DIR}/bin"
        )

        if(NOT MYSQL_INCLUDE_DIR OR NOT MYSQL_CLIENT_LIBRARY OR
           NOT MYSQL_CLIENT_RUNTIME OR NOT MYSQL_SSL_RUNTIME OR
           NOT MYSQL_CRYPTO_RUNTIME)
            message(FATAL_ERROR
                "MySQL client headers, library, or runtime DLLs were not found. "
                "Set MYSQL_ROOT_DIR to the MySQL installation directory."
            )
        endif()

        set(MYSQL_RUNTIME_FILES
            "${MYSQL_CLIENT_RUNTIME}"
            "${MYSQL_SSL_RUNTIME}"
            "${MYSQL_CRYPTO_RUNTIME}"
        )

        target_include_directories(chatroom_mysql PRIVATE "${MYSQL_INCLUDE_DIR}")
        target_link_libraries(chatroom_mysql PUBLIC "${MYSQL_CLIENT_LIBRARY}")
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(MYSQLCLIENT REQUIRED IMPORTED_TARGET mysqlclient)
        target_link_libraries(chatroom_mysql PUBLIC PkgConfig::MYSQLCLIENT)
    endif()

    add_library(chatroom::mysql ALIAS chatroom_mysql)

    add_executable(mysql_connection_check
        src/mysql_connection_check.cpp
    )
    target_link_libraries(mysql_connection_check PRIVATE chatroom::mysql)

    if(WIN32)
        add_custom_command(
            TARGET mysql_connection_check
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${MYSQL_RUNTIME_FILES}
                "$<TARGET_FILE_DIR:mysql_connection_check>"
            VERBATIM
        )
    endif()
endif()

add_executable(browser_chatroom_architecture
    src/main.cpp
    "include/storage/user_memory_store_impl.h"
    "include/storage/user_memory_store_impl.cpp"
    "include/storage/lobby_memory_store_impl.cpp"
    "include/storage/session_memory_store_impl.cpp"
    "include/storage/private_chat_memory_store_impl.cpp"
    "include/user/user_service_impl.h"
    "include/user/user_service_impl.cpp"
    "include/user/auth_service_impl.cpp" 
    "include/user/auth_service_impl.h"
    "include/chat/online_user_service.cpp"
    "include/chat/lobby_service.cpp"
    "include/chat/private_chat_service.cpp"
    "include/chat/chat_controller.cpp"
    "include/user/auth_controller_impl.h" 
    "include/user/auth_controller_impl.cpp"
    "include/websocket/ws_chat_controller_impl.cpp" 
    "include/websocket/ws_chat_controller_impl.h" 
    "include/websocket/ws_connection_manager_impl.cpp" 
    "include/websocket/ws_connection_manager_impl.h" 
    "include/websocket/ws_message_dispatcher_impl.cpp" 
    "include/websocket/ws_message_dispatcher_impl.h"
    "include/framework/crow_server_app.h"
    "include/framework/crow_server_app.cpp"
    "include/framework/crow_router_registry.h"
    "include/framework/crow_router_registry.cpp"
)

target_compile_definitions(browser_chatroom_architecture PRIVATE CROW_USE_BOOST=1)

if(WIN32)
    target_link_libraries(browser_chatroom_architecture PRIVATE ws2_32)
endif()

if(CHATROOM_ENABLE_MYSQL)
    target_link_libraries(browser_chatroom_architecture PRIVATE chatroom::mysql)
    target_compile_definitions(
        browser_chatroom_architecture
        PRIVATE CHATROOM_ENABLE_MYSQL=1
    )

    if(WIN32)
        add_custom_command(
            TARGET browser_chatroom_architecture
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${MYSQL_RUNTIME_FILES}
                "$<TARGET_FILE_DIR:browser_chatroom_architecture>"
            VERBATIM
        )
    endif()
endif()
