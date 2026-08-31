cmake_minimum_required(VERSION 3.16)
project(browser_chatroom_architecture LANGUAGES CXX)

include(CTest)

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

<<<<<<< HEAD
=======
option(CHATROOM_ENABLE_MYSQL "Build MySQL-backed storage support" ON)

if(CHATROOM_ENABLE_MYSQL)
    add_library(chatroom_mysql STATIC
        include/storage/mysql/mysql_database.cpp
        include/storage/mysql/mysql_statement.cpp
        include/storage/mysql/mysql_schema.cpp
        include/storage/mysql/mysql_user_store.cpp
        include/storage/mysql/mysql_lobby_store.cpp
        include/storage/mysql/mysql_private_chat_store.cpp
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

if(BUILD_TESTING)
    add_executable(session_lifecycle_test
        tests/session_lifecycle_test.cpp
        include/user/auth_service_impl.cpp
        include/websocket/ws_chat_controller_impl.cpp
        include/websocket/ws_connection_manager_impl.cpp
        include/chat/online_user_service.cpp
    )
    if(WIN32)
        target_link_libraries(session_lifecycle_test PRIVATE ws2_32)
    endif()
    add_test(NAME session_lifecycle COMMAND session_lifecycle_test)
    find_program(CHATROOM_NODE_EXECUTABLE NAMES node node.exe)
    if(CHATROOM_NODE_EXECUTABLE)
        add_executable(session_socket_server
            tests/session_socket_server.cpp
            include/user/auth_service_impl.cpp
            include/user/auth_controller_impl.cpp
            include/storage/user_memory_store_impl.cpp
            include/websocket/ws_chat_controller_impl.cpp
            include/websocket/ws_connection_manager_impl.cpp
            include/chat/online_user_service.cpp
        )
        if(WIN32)
            target_link_libraries(session_socket_server PRIVATE ws2_32)
        endif()
        add_test(NAME session_socket_integration COMMAND "${CHATROOM_NODE_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tests/session_socket_test.cjs" "$<TARGET_FILE:session_socket_server>")
        set_tests_properties(session_socket_integration PROPERTIES TIMEOUT 30)
    endif()
endif()

>>>>>>> N-storage
add_executable(browser_chatroom_architecture
    src/main.cpp
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
    "include/framework/crow_server_app.cpp"
    "include/framework/crow_router_registry.cpp"
    "include/framework/json_response_builder.cpp"
)

target_compile_definitions(
    browser_chatroom_architecture
    PRIVATE CROW_USE_BOOST=1
)

target_compile_definitions(browser_chatroom_architecture PRIVATE CROW_USE_BOOST=1)

if(WIN32)
    target_link_libraries(browser_chatroom_architecture PRIVATE ws2_32)
endif()
