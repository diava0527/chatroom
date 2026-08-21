#pragma once

#include <string>
#include <vector>

#include "models/message.h"

namespace chatroom::models {

struct PrivateChatSession {
    std::string privateSessionId;
    std::string senderNickname;
    std::string receiverNickname;
    std::vector<Message> messages;
};

}  // namespace chatroom::models
