#pragma once

#include <optional>
#include <string>
#include <vector>

#include "models/user.h"

namespace chatroom::user {

class UserService {
public:
    virtual ~UserService() = default;

    // 1)代码逻辑：根据昵称查询用户信息，用于登录前校验、在线身份核验和其他业务对用户基础信息的读取。
    // 2)返回值类型：std::optional<chatroom::models::User>，原因是用户查询可能成功也可能不存在，供登录接口和聊天权限校验接口调用。
    // 3)参数类型：const std::string& nickname，原因是昵称在本系统中唯一且代替账号，是最自然的用户查询主键。
    virtual std::optional<chatroom::models::User> FindUserByNickname(const std::string& nickname) const = 0;

    // 1)代码逻辑：返回当前内存中已注册的全部用户，用于管理展示、调试或后续扩展。
    // 2)返回值类型：std::vector<chatroom::models::User>，原因是用户集合天然是列表结构，供用户系统模块内部接口调用。
    // 3)参数类型：无参数，原因是这里读取的是当前全量用户视图，不依赖外部条件。
    virtual std::vector<chatroom::models::User> ListUsers() const = 0;
};

}  // namespace chatroom::user
