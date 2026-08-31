#include "storage/mysql/mysql_statement.h"

#include <mysql.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {

[[noreturn]] void ThrowStatementError(MYSQL_STMT* statement,
                                      const std::string& context) {
    throw chatroom::storage::mysql::MySqlError(
        statement == nullptr ? 0 : mysql_stmt_errno(statement),
        context + (statement == nullptr || *mysql_stmt_error(statement) == '\0'
                       ? std::string{}
                       : std::string(": ") + mysql_stmt_error(statement)));
}

}  // namespace

namespace chatroom::storage::mysql {

MySqlStatement::MySqlStatement(MySqlConnection& connection,
                               const std::string& sql) {
    auto* nativeConnection = static_cast<MYSQL*>(connection.NativeHandle());
    if (nativeConnection == nullptr) {
        throw MySqlError(0, "cannot prepare a statement on a closed connection");
    }

    MYSQL_STMT* rawStatement = mysql_stmt_init(nativeConnection);
    if (rawStatement == nullptr) {
        throw MySqlError(mysql_errno(nativeConnection), "mysql_stmt_init failed");
    }
    std::unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> statement(
        rawStatement, &mysql_stmt_close);

    if (mysql_stmt_prepare(statement.get(), sql.c_str(),
                           static_cast<unsigned long>(sql.size())) != 0) {
        ThrowStatementError(
            statement.get(), "failed to prepare MySQL statement");
    }
    statement_ = statement.release();
}

MySqlStatement::~MySqlStatement() {
    if (statement_ != nullptr) {
        mysql_stmt_close(static_cast<MYSQL_STMT*>(statement_));
    }
}

void MySqlStatement::BindParameters(
    const std::vector<std::string>& parameters) {
    auto* statement = static_cast<MYSQL_STMT*>(statement_);
    const unsigned long expected = mysql_stmt_param_count(statement);
    if (expected != parameters.size()) {
        throw std::invalid_argument(
            "MySQL statement parameter count does not match SQL placeholders");
    }
    if (parameters.empty()) {
        return;
    }

    std::vector<MYSQL_BIND> bindings(parameters.size());
    parameterLengths_.resize(parameters.size());
    std::memset(bindings.data(), 0, bindings.size() * sizeof(MYSQL_BIND));

    for (std::size_t index = 0; index < parameters.size(); ++index) {
        if (parameters[index].size() >
            static_cast<std::size_t>((std::numeric_limits<unsigned long>::max)())) {
            throw std::length_error("MySQL string parameter is too large");
        }
        parameterLengths_[index] =
            static_cast<unsigned long>(parameters[index].size());
        bindings[index].buffer_type = MYSQL_TYPE_STRING;
        bindings[index].buffer =
            const_cast<char*>(parameters[index].data());
        bindings[index].buffer_length = parameterLengths_[index];
        bindings[index].length = &parameterLengths_[index];
    }

    if (mysql_stmt_bind_param(statement, bindings.data()) != 0) {
        ThrowStatementError(statement, "failed to bind MySQL statement parameters");
    }
}

void MySqlStatement::ExecutePrepared(
    const std::vector<std::string>& parameters) {
    auto* statement = static_cast<MYSQL_STMT*>(statement_);
    if (mysql_stmt_reset(statement) != 0) {
        ThrowStatementError(statement, "failed to reset MySQL statement");
    }
    BindParameters(parameters);
    if (mysql_stmt_execute(statement) != 0) {
        ThrowStatementError(statement, "failed to execute MySQL statement");
    }
}

std::uint64_t MySqlStatement::Execute(
    const std::vector<std::string>& parameters) {
    ExecutePrepared(parameters);
    return static_cast<std::uint64_t>(
        mysql_stmt_affected_rows(static_cast<MYSQL_STMT*>(statement_)));
}

MySqlRows MySqlStatement::Query(const std::vector<std::string>& parameters) {
    auto* statement = static_cast<MYSQL_STMT*>(statement_);
    bool updateMaxLength = true;
    if (mysql_stmt_attr_set(statement, STMT_ATTR_UPDATE_MAX_LENGTH,
                            &updateMaxLength) != 0) {
        ThrowStatementError(statement, "failed to configure MySQL result metadata");
    }

    ExecutePrepared(parameters);
    if (mysql_stmt_store_result(statement) != 0) {
        ThrowStatementError(statement, "failed to buffer MySQL query result");
    }

    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> metadata(
        mysql_stmt_result_metadata(statement), &mysql_free_result);
    if (!metadata) {
        if (mysql_stmt_field_count(statement) == 0) {
            return {};
        }
        ThrowStatementError(statement, "failed to read MySQL result metadata");
    }

    const unsigned int columnCount = mysql_num_fields(metadata.get());
    MYSQL_FIELD* fields = mysql_fetch_fields(metadata.get());
    std::vector<std::vector<char>> buffers(columnCount);
    std::vector<unsigned long> lengths(columnCount, 0);
    auto nullValues = std::make_unique<bool[]>(columnCount);
    auto errors = std::make_unique<bool[]>(columnCount);
    std::vector<MYSQL_BIND> bindings(columnCount);
    std::memset(bindings.data(), 0, bindings.size() * sizeof(MYSQL_BIND));

    for (unsigned int index = 0; index < columnCount; ++index) {
        const auto capacity = static_cast<std::size_t>(fields[index].max_length) + 1;
        buffers[index].resize(std::max<std::size_t>(capacity, 1));
        bindings[index].buffer_type = MYSQL_TYPE_STRING;
        bindings[index].buffer = buffers[index].data();
        bindings[index].buffer_length =
            static_cast<unsigned long>(buffers[index].size());
        bindings[index].length = &lengths[index];
        nullValues[index] = false;
        errors[index] = false;
        bindings[index].is_null = &nullValues[index];
        bindings[index].error = &errors[index];
    }

    if (columnCount > 0 && mysql_stmt_bind_result(statement, bindings.data()) != 0) {
        ThrowStatementError(statement, "failed to bind MySQL query result");
    }

    MySqlRows rows;
    while (true) {
        const int fetchResult = mysql_stmt_fetch(statement);
        if (fetchResult == MYSQL_NO_DATA) {
            break;
        }
        if (fetchResult != 0 && fetchResult != MYSQL_DATA_TRUNCATED) {
            ThrowStatementError(statement, "failed to fetch MySQL query row");
        }

        MySqlRow row;
        row.reserve(columnCount);
        for (unsigned int index = 0; index < columnCount; ++index) {
            if (nullValues[index]) {
                row.emplace_back(std::nullopt);
                continue;
            }
            if (errors[index] || lengths[index] >= buffers[index].size()) {
                buffers[index].resize(static_cast<std::size_t>(lengths[index]) + 1);
                MYSQL_BIND column{};
                column.buffer_type = MYSQL_TYPE_STRING;
                column.buffer = buffers[index].data();
                column.buffer_length =
                    static_cast<unsigned long>(buffers[index].size());
                column.length = &lengths[index];
                if (mysql_stmt_fetch_column(statement, &column, index, 0) != 0) {
                    ThrowStatementError(statement, "failed to fetch full MySQL column");
                }
            }
            row.emplace_back(std::string(
                buffers[index].data(), static_cast<std::size_t>(lengths[index])));
        }
        rows.push_back(std::move(row));
    }

    mysql_stmt_free_result(statement);
    return rows;
}

}  // namespace chatroom::storage::mysql
