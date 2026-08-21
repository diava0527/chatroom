#pragma once

#include <optional>
#include <string>

namespace chatroom::storage {

class SessionMemoryStore {
public:
    virtual ~SessionMemoryStore() = default;

    // 1)代码逻辑：保存 sessionId 到昵称的映射关系，建立登录态。
    // 2)返回值类型：void，原因是这里定义的是存储动作本身，不需要同步返回业务对象，供登录接口调用。
    // 3)参数类型：const std::string& sessionId 与 const std::string& nickname，原因是登录态核心就是会话标识和用户昵称的对应关系，参数直接对应 session 设计。
    virtual void SaveSession(const std::string& sessionId, const std::string& nickname) = 0;

    // 1)代码逻辑：根据 sessionId 查询当前登录用户昵称，用于 HTTP 和 WebSocket 鉴权。
    // 2)返回值类型：std::optional<std::string>，原因是 sessionId 可能有效也可能无效，供鉴权相关接口调用。
    // 3)参数类型：const std::string& sessionId，原因是客户端当前唯一能提供的登录态凭证就是 sessionId，参数直接对应登录态要求。
    virtual std::optional<std::string> FindNicknameBySessionId(const std::string& sessionId) const = 0;

    // 1)代码逻辑：删除指定 sessionId 的登录态映射，在登出后使其失效。
    // 2)返回值类型：void，原因是这里定义的是会话失效动作，不需要返回附加业务结果，供登出接口调用。
    // 3)参数类型：const std::string& sessionId，原因是登出针对的是客户端当前持有的会话标识，参数直接对应登出流程。
    virtual void RemoveSession(const std::string& sessionId) = 0;
};

}  // namespace chatroom::storage
