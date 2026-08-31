#pragma once

#include <optional>
#include <string>

namespace chatroom::user {

class AuthService {
public:
    virtual ~AuthService() = default;

    // 1)代码逻辑：校验昵称是否重复并完成注册，将新用户写入内存用户存储。
    // 2)返回值类型：bool，原因是注册阶段只需要表达成功或失败，供注册 HTTP 接口调用。
    // 3)参数类型：const std::string& nickname 与 const std::string& password，原因是注册业务最小输入就是唯一昵称和用户自设密码，参数直接对应注册要求。
    virtual bool Register(const std::string& nickname, const std::string& password) = 0;

    // 1)代码逻辑：校验昵称和密码，登录成功后生成 sessionId 并建立登录态映射。
    // 2)返回值类型：std::optional<std::string>，原因是登录可能成功并返回 sessionId，也可能失败无结果，供登录 HTTP 接口和 WebSocket 建连流程调用。
    // 3)参数类型：const std::string& nickname 与 const std::string& password，原因是登录凭证就是昵称和密码，参数直接对应登录要求。
    virtual std::optional<std::string> Login(const std::string& nickname, const std::string& password) = 0;

    // 1)代码逻辑：根据 sessionId 注销当前登录态，并触发相关在线状态和私聊会话清理。
    // 2)返回值类型：bool，原因是登出只需要表达该 sessionId 是否有效并是否成功清理，供登出 HTTP 接口调用。
    // 3)参数类型：const std::string& sessionId，原因是客户端登录后只持有 sessionId，参数直接对应登录态设计。
    virtual bool Logout(const std::string& sessionId) = 0;

    // 1)代码逻辑：根据 sessionId 解析当前登录用户昵称，用于受保护 HTTP 接口和 WebSocket 连接鉴权。
    // 2)返回值类型：std::optional<std::string>，原因是 sessionId 可能有效也可能失效，供聊天模块和 WebSocket 模块调用。
    // 3)参数类型：const std::string& sessionId，原因是该值是系统唯一登录态凭证，参数直接对应鉴权逻辑。
    virtual std::optional<std::string> ValidateSession(const std::string& sessionId) const = 0;

    // 1)代码逻辑：原子校验并登记当前连接；同会话重连替换旧连接身份。
    // 2)返回值类型：std::optional<std::string>，供 WebSocket 建连时取得昵称或拒绝失效会话。
    // 3)参数类型：sessionId 为登录凭据，connection 为仅比较身份、不解引用的连接指针。
    virtual std::optional<std::string> AttachConnection(
        const std::string& sessionId, const void* connection) = 0;

    // 1)代码逻辑：仅当前连接断开时释放登录占用，旧连接迟到的关闭事件不影响新连接。
    // 2)返回值类型：void，供 WebSocket 关闭回调更新会话生命周期。
    // 3)参数类型：sessionId 与 connection 共同定位准确的登录与连接。
    virtual void DetachConnection(const std::string& sessionId, const void* connection) = 0;

    // 1)代码逻辑：确认消息来自仍有效的当前连接，阻止被替换或已退出的连接继续发消息。
    // 2)返回值类型：bool，供 WebSocket 消息控制器判断是否放行。
    // 3)参数类型：sessionId 与 connection 用于同时验证登录态和连接身份。
    virtual bool ValidateConnection(const std::string& sessionId, const void* connection) const = 0;
};

}  // namespace chatroom::user
