#pragma once

#include <string>

namespace chatroom::models {

struct Message {
    std::string messageId;
    std::string senderNickname;
    std::string receiverNickname;
    std::string content;
    std::string timestamp;
};

}  // namespace chatroom::models
