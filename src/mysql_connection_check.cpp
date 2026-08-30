#include "storage/mysql/mysql_database.h"

#include <exception>
#include <iostream>

int main() {
    try {
        auto config = chatroom::storage::mysql::MySqlConfig::FromEnvironment();
        chatroom::storage::mysql::MySqlDatabase database(config);

        auto connection = database.OpenConnection();
        connection.Ping();

        std::cout << "MySQL connection succeeded\n"
                  << "server=" << config.host << ':' << config.port << '\n'
                  << "database="
                  << (config.database.empty() ? "(not selected)" : config.database)
                  << '\n'
                  << "user=" << config.username << '\n'
                  << "server_version=" << connection.ServerVersion() << '\n';
        return 0;
    } catch (const chatroom::storage::mysql::MySqlError& error) {
        std::cerr << "MySQL connection failed"
                  << " (error " << error.ErrorCode() << "): "
                  << error.what() << '\n';
    } catch (const std::exception& error) {
        std::cerr << "MySQL connection configuration failed: "
                  << error.what() << '\n';
    }
    return 1;
}
