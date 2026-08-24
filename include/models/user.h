#pragma once

#include <string>

namespace chatroom::models {

struct User {
    std::string nickname;
    std::string password;
};

}  // namespace chatroom::models
