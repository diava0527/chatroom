#pragma once
#include "auth_service.h"
#include "storage/user_memory_store.h"
#include<memory>
#include <unordered_map>
#include <mutex>
#include<optional>

namespace chatroom::user {

class AuthServiceImpl : public AuthService {

	private:
		std::shared_ptr<storage::UserMemoryStore> user_store;         //共享指针使用UserMemoryStore的接口
		std::unordered_map<std::string, std::string> session_map;     //sessionId->昵称映射，可通过sessionId找昵称（创建新sessionId时校验）
		std::unordered_map<std::string, std::string> nickname_map;    //昵称->sessionId映射，可通过昵称查找sessionId（用于判断某昵称用户是否在线）

		mutable std::mutex session_mtx;                               //读写两张映射表时上锁，防止并发读写竞争

	public:

		explicit AuthServiceImpl(std::shared_ptr<storage::UserMemoryStore> store);    

		// 1)代码逻辑：校验昵称是否重复并完成注册，将新用户写入内存用户存储。
		// 2)返回值类型：bool，原因是注册阶段只需要表达成功或失败，供注册 HTTP 接口调用。
		// 3)参数类型：const std::string& nickname 与 const std::string& password，原因是注册业务最小输入就是唯一昵称和用户自设密码，参数直接对应注册要求。
		bool Register(const std::string& nickname, const std::string& password) override;

		// 1)代码逻辑：校验昵称和密码，登录成功后生成 sessionId 并建立登录态映射。
		// 2)返回值类型：std::optional<std::string>，原因是登录可能成功并返回 sessionId，也可能失败无结果，供登录 HTTP 接口和 WebSocket 建连流程调用。
		// 3)参数类型：const std::string& nickname 与 const std::string& password，原因是登录凭证就是昵称和密码，参数直接对应登录要求。
		std::optional<std::string> Login(const std::string& nickname, const std::string& password) override;


		// 1)代码逻辑：根据 sessionId 注销当前登录态，并触发相关在线状态和私聊会话清理。
		// 2)返回值类型：bool，原因是登出只需要表达该 sessionId 是否有效并是否成功清理，供登出 HTTP 接口调用。
		// 3)参数类型：const std::string& sessionId，原因是客户端登录后只持有 sessionId，参数直接对应登录态设计。
		bool Logout(const std::string& sessionId) override;

		// 1)代码逻辑：根据 sessionId 解析当前登录用户昵称，用于受保护 HTTP 接口和 WebSocket 连接鉴权。
		// 2)返回值类型：std::optional<std::string>，原因是 sessionId 可能有效也可能失效，供聊天模块和 WebSocket 模块调用。
		// 3)参数类型：const std::string& sessionId，原因是该值是系统唯一登录态凭证，参数直接对应鉴权逻辑。
		std::optional<std::string> ValidateSession(const std::string& sessionId) const override;
};

}