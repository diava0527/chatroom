#pragma once

#include <string_view>

namespace chatroom::common {

inline constexpr std::string_view kApiPrefix = "/api/v1";
inline constexpr std::string_view kWebSocketPath = "/ws/chat";
inline constexpr std::string_view kLobbyReceiver = "LOBBY";
inline constexpr int kSuccessCode = 0;
inline constexpr int kNicknameExistsCode = 1001;
inline constexpr int kLoginFailedCode = 1002;
inline constexpr int kSessionInvalidCode = 1003; 
inline constexpr int kInvalidParamCode = 1004;	//参数缺失或格式错误
inline constexpr int kTargetNotOnlineCode = 1005; // 目标用户不在线

}  // namespace chatroom::common
