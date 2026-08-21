#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace chatroom::common {

using MessageId = std::string;
using Nickname = std::string;
using SessionId = std::string;
using PrivateSessionId = std::string;
using Timestamp = std::string;

enum class UserStatus : std::uint8_t {
    Offline = 0,
    Online = 1
};

enum class ChatScope : std::uint8_t {
    Lobby = 0,
    Private = 1
};

}  // namespace chatroom::common
