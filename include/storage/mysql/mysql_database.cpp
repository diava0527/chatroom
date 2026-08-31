#include "storage/mysql/mysql_database.h"

#include <mysql.h>

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <memory>
#include <optional>
#include <utility>

namespace {

std::optional<std::string> ReadEnvironment(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return std::nullopt;
    }
    return std::string(value);
}

unsigned long ParseUnsignedEnvironment(
    const char* name,
    unsigned long defaultValue,
    unsigned long minimum,
    unsigned long maximum) {
    auto rawValue = ReadEnvironment(name);
    if (!rawValue.has_value() || rawValue->empty()) {
        return defaultValue;
    }

    errno = 0;
    char* end = nullptr;
    const unsigned long value = std::strtoul(rawValue->c_str(), &end, 10);
    if (errno != 0 || end == rawValue->c_str() || *end != '\0' ||
        value < minimum || value > maximum) {
        throw std::invalid_argument(
            std::string(name) + " must be an integer between " +
            std::to_string(minimum) + " and " + std::to_string(maximum));
    }
    return value;
}

class MySqlClientRuntime {
public:
    MySqlClientRuntime() {
        if (mysql_library_init(0, nullptr, nullptr) != 0) {
            throw chatroom::storage::mysql::MySqlError(
                0, "failed to initialize the MySQL client library");
        }
    }

    ~MySqlClientRuntime() {
        mysql_library_end();
    }

    MySqlClientRuntime(const MySqlClientRuntime&) = delete;
    MySqlClientRuntime& operator=(const MySqlClientRuntime&) = delete;
};

void EnsureMySqlClientRuntime() {
    static MySqlClientRuntime runtime;
    (void)runtime;
}

[[noreturn]] void ThrowMySqlError(MYSQL* handle, const std::string& context) {
    const unsigned int code = handle == nullptr ? 0 : mysql_errno(handle);
    const char* detail = handle == nullptr ? nullptr : mysql_error(handle);
    throw chatroom::storage::mysql::MySqlError(
        code,
        context + (detail == nullptr || *detail == '\0'
                       ? std::string{}
                       : std::string(": ") + detail));
}

}  // namespace

namespace chatroom::storage::mysql {

MySqlConfig MySqlConfig::FromEnvironment() {
    MySqlConfig config;

    if (auto value = ReadEnvironment("DB_HOST"); value.has_value() && !value->empty()) {
        config.host = *value;
    }
    config.port = static_cast<std::uint16_t>(ParseUnsignedEnvironment(
        "DB_PORT", 3306, 1, 65535));

    if (auto value = ReadEnvironment("DB_NAME"); value.has_value()) {
        config.database = *value;
    }
    if (config.database.empty()) {
        throw std::invalid_argument("DB_NAME is required");
    }
    if (auto value = ReadEnvironment("DB_USER"); value.has_value()) {
        config.username = *value;
    }
    if (config.username.empty()) {
        throw std::invalid_argument("DB_USER is required");
    }
    if (auto value = ReadEnvironment("DB_PASSWORD"); value.has_value()) {
        config.password = *value;
    }

    config.connectTimeoutSeconds = static_cast<unsigned int>(
        ParseUnsignedEnvironment(
            "DB_CONNECT_TIMEOUT_SECONDS", 5, 1, UINT_MAX));

    return config;
}

MySqlError::MySqlError(unsigned int errorCode, const std::string& message)
    : std::runtime_error(message), errorCode_(errorCode) {}

unsigned int MySqlError::ErrorCode() const noexcept {
    return errorCode_;
}

MySqlConnection::MySqlConnection(void* handle) noexcept : handle_(handle) {}

MySqlConnection::~MySqlConnection() {
    if (handle_ != nullptr) {
        mysql_close(static_cast<MYSQL*>(handle_));
    }
}

MySqlConnection::MySqlConnection(MySqlConnection&& other) noexcept
    : handle_(std::exchange(other.handle_, nullptr)) {}

MySqlConnection& MySqlConnection::operator=(MySqlConnection&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    if (handle_ != nullptr) {
        mysql_close(static_cast<MYSQL*>(handle_));
    }
    handle_ = std::exchange(other.handle_, nullptr);
    return *this;
}

void MySqlConnection::Ping() {
    if (handle_ == nullptr) {
        throw MySqlError(0, "cannot ping a closed MySQL connection");
    }
    auto* handle = static_cast<MYSQL*>(handle_);
    if (mysql_ping(handle) != 0) {
        ThrowMySqlError(handle, "MySQL ping failed");
    }
}

std::string MySqlConnection::ServerVersion() const {
    if (handle_ == nullptr) {
        return {};
    }
    const char* version = mysql_get_server_info(static_cast<MYSQL*>(handle_));
    return version == nullptr ? std::string{} : std::string(version);
}

void* MySqlConnection::NativeHandle() noexcept {
    return handle_;
}

MySqlDatabase::MySqlDatabase(MySqlConfig config)
    : config_(std::move(config)) {}

MySqlConnection MySqlDatabase::OpenConnection() const {
    EnsureMySqlClientRuntime();

    MYSQL* rawHandle = mysql_init(nullptr);
    if (rawHandle == nullptr) {
        throw MySqlError(0, "mysql_init failed");
    }
    std::unique_ptr<MYSQL, decltype(&mysql_close)> handle(rawHandle, &mysql_close);

    unsigned int timeout = config_.connectTimeoutSeconds;
    if (mysql_options(handle.get(), MYSQL_OPT_CONNECT_TIMEOUT, &timeout) != 0) {
        ThrowMySqlError(handle.get(), "failed to configure MySQL connect timeout");
    }

    const char* database = config_.database.empty()
        ? nullptr
        : config_.database.c_str();

    if (mysql_real_connect(
            handle.get(),
            config_.host.c_str(),
            config_.username.c_str(),
            config_.password.c_str(),
            database,
            config_.port,
            nullptr,
            0) == nullptr) {
        ThrowMySqlError(handle.get(), "failed to connect to MySQL");
    }

    if (mysql_set_character_set(handle.get(), "utf8mb4") != 0) {
        ThrowMySqlError(handle.get(), "failed to select utf8mb4 character set");
    }

    if (mysql_query(handle.get(), "SET time_zone = '+00:00'") != 0) {
        ThrowMySqlError(handle.get(), "failed to configure the MySQL session time zone");
    }

    return MySqlConnection(handle.release());
}

const MySqlConfig& MySqlDatabase::Config() const noexcept {
    return config_;
}

}  // namespace chatroom::storage::mysql
