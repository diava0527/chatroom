#pragma once

#include <string>

namespace chatroom::models {

struct LobbySession {
    std::string nickname;
    std::string enteredAt;
};

}  // namespace chatroom::models
