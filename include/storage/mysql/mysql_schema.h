#pragma once

#include "storage/mysql/mysql_database.h"

namespace chatroom::storage::mysql {

void InitializeChatroomSchema(const MySqlDatabase& database);

}  // namespace chatroom::storage::mysql
