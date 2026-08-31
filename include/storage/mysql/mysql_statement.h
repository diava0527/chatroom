#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "storage/mysql/mysql_database.h"

namespace chatroom::storage::mysql {

using MySqlRow = std::vector<std::optional<std::string>>;
using MySqlRows = std::vector<MySqlRow>;

class MySqlStatement {
public:
    MySqlStatement(MySqlConnection& connection, const std::string& sql);
    ~MySqlStatement();

    MySqlStatement(const MySqlStatement&) = delete;
    MySqlStatement& operator=(const MySqlStatement&) = delete;

    std::uint64_t Execute(const std::vector<std::string>& parameters = {});
    MySqlRows Query(const std::vector<std::string>& parameters = {});

private:
    void BindParameters(const std::vector<std::string>& parameters);
    void ExecutePrepared(const std::vector<std::string>& parameters);

    void* statement_ = nullptr;
    std::vector<unsigned long> parameterLengths_;
};

}  // namespace chatroom::storage::mysql
