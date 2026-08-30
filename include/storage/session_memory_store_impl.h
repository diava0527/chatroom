#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "storage/session_memory_store.h"

namespace chatroom::storage {

/// 登录态存储的内存实现：保存 sessionId 到用户昵称的映射。
class SessionMemoryStoreImpl final : public SessionMemoryStore {
public:
    /// 新增或覆盖指定 sessionId 的登录用户映射。
    void SaveSession(const std::string& sessionId,
                     const std::string& nickname) override;

    /// 查找 sessionId 对应的用户昵称；无效 sessionId 返回 std::nullopt。
    std::optional<std::string> FindNicknameBySessionId(
        const std::string& sessionId) const override;

    /// 删除指定 sessionId，使当前登录态立即失效。
    void RemoveSession(const std::string& sessionId) override;

private:
    /// 保护 sessions_，支持并发登录、鉴权和登出操作。
    mutable std::mutex mutex_;

    /// sessionId 到昵称的一对一映射。
    std::unordered_map<std::string, std::string> sessions_;
};

}  // namespace chatroom::storage
