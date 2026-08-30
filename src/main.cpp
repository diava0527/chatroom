#define CROW_USE_BOOST 1

#include "crow_all.h"

#include "chat/chat_controller.h"
#include "chat/chat_protocol.h"
#include "chat/lobby_service.h"
#include "chat/online_user_service.h"
#include "chat/private_chat_service.h"
#include "common/constants.h"
#include "common/types.h"
#include "framework/response_builder.h"
#include "framework/router_registry.h"
#include "framework/server_app.h"
#include "models/lobby_session.h"
#include "models/message.h"
#include "models/private_chat_session.h"
#include "models/user.h"
#include "storage/lobby_memory_store.h"
#include "storage/private_chat_memory_store.h"
#include "storage/session_memory_store.h"
#include "storage/user_memory_store.h"
#if defined(CHATROOM_ENABLE_MYSQL)
#include "storage/mysql/mysql_database.h"
#endif
#include "user/auth_controller.h"
#include "user/auth_service.h"
#include "user/user_service.h"
#include "websocket/ws_chat_controller.h"
#include "websocket/ws_connection_manager.h"
#include "websocket/ws_message_dispatcher.h"

#include <exception>
#include <iostream>

int main() {
    try {
#if defined(CHATROOM_ENABLE_MYSQL)
        auto mysqlConfig =
            chatroom::storage::mysql::MySqlConfig::FromEnvironment();
        chatroom::storage::mysql::MySqlDatabase mysqlDatabase(mysqlConfig);
        auto mysqlConnection = mysqlDatabase.OpenConnection();
        mysqlConnection.Ping();

        std::cout << "Connected to MySQL "
                  << mysqlConnection.ServerVersion() << " at "
                  << mysqlConfig.host << ':' << mysqlConfig.port << '\n';
#endif

        crow::SimpleApp app;

        // 当前阶段只保留启动入口骨架，不实现具体业务逻辑。
        // 这里统一纳入项目架构所需头文件，作为后续模块装配的总入口。
        (void)app;

        return 0;
#if defined(CHATROOM_ENABLE_MYSQL)
    } catch (const chatroom::storage::mysql::MySqlError& error) {
        std::cerr << "MySQL startup check failed (error "
                  << error.ErrorCode() << "): " << error.what() << '\n';
#endif
    } catch (const std::exception& error) {
        std::cerr << "Application startup failed: " << error.what() << '\n';
    }
    return 1;
}
