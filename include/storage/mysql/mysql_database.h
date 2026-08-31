#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace chatroom::storage::mysql {

struct MySqlConfig {
    std::string host = "127.0.0.1";
    std::uint16_t port = 3306;
    std::string database;
    std::string username;
    std::string password;
    unsigned int connectTimeoutSeconds = 5;

    // Reads DB_HOST, DB_PORT, DB_NAME, DB_USER, DB_PASSWORD and
    // DB_CONNECT_TIMEOUT_SECONDS. DB_NAME and DB_USER are required; the
    // password may be empty for explicitly configured local accounts.
    static MySqlConfig FromEnvironment();
};

class MySqlError : public std::runtime_error {
public:
    MySqlError(unsigned int errorCode, const std::string& message);

    unsigned int ErrorCode() const noexcept;

private:
    unsigned int errorCode_;
};

class MySqlConnection {
public:
    ~MySqlConnection();

    MySqlConnection(const MySqlConnection&) = delete;
    MySqlConnection& operator=(const MySqlConnection&) = delete;

    MySqlConnection(MySqlConnection&& other) noexcept;
    MySqlConnection& operator=(MySqlConnection&& other) noexcept;

    void Ping();
    std::string ServerVersion() const;
    void* NativeHandle() noexcept;

private:
    friend class MySqlDatabase;

    explicit MySqlConnection(void* handle) noexcept;

    void* handle_ = nullptr;
};

class MySqlDatabase {
public:
    explicit MySqlDatabase(MySqlConfig config);

    MySqlConnection OpenConnection() const;
    const MySqlConfig& Config() const noexcept;

private:
    MySqlConfig config_;
};

}  // namespace chatroom::storage::mysql
