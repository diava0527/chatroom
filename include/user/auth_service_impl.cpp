#include "auth_service_impl.h"
#include <random>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>

namespace {   //仅当前文件使用的的辅助函数，定义在匿名命名空间防止重名
	std::string GenerateSessionId() {                                       //生成固定32位十六进制的SessionId会话凭证

		thread_local std::random_device rd;                                 // thread_local：每个线程单独实例，避免多线程竞争同一个随机发生器

		thread_local std::mt19937 gen(rd());

		thread_local std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF); // 均匀分布，生成完整32位无符号整数 [0, 0xFFFFFFFF]
		std::ostringstream oss;
		oss << std::hex;                                                    // 十六进制输入到缓冲区
		oss << std::setfill('0');                                           // 不足宽度时使用0补齐

		for (int i = 0; i < 4; ++i) {
			oss << std::setw(8) << dist(gen);                              // 循环4次：每次生成4字节(uint32_t)，合计 4*4 = 16字节随机数据
		}
		return oss.str();                                                  //取出缓冲区全部字符，打包成 std::string 返回
	}
}


namespace chatroom::user {

	//显式构造函数
	AuthServiceImpl::AuthServiceImpl(std::shared_ptr<storage::UserMemoryStore> store) :user_store(std::move(store)) {}

	// 1)代码逻辑：校验昵称是否重复并完成注册，将新用户写入内存用户存储。
	// 2)返回值类型：bool，原因是注册阶段只需要表达成功或失败，供注册 HTTP 接口调用。
	// 3)参数类型：const std::string& nickname 与 const std::string& password，原因是注册业务最小输入就是唯一昵称和用户自设密码，参数直接对应注册要求
	bool AuthServiceImpl::Register(const std::string& nickname, const std::string& password) {
		chatroom::models::User user;
		user.nickname = nickname, user.password = password;                            //构造用户对象，保存昵称与密码
		return user_store->SaveUser(user);                                             //调用存储接口保存用户(接口内部已经实现查重)
	}

	// 1)代码逻辑：校验昵称和密码，登录成功后生成 sessionId 并建立登录态映射。
	//   注意已经登录的用户再次登录（多端登录）会将之前的登录态登出
	// 2)返回值类型：std::optional<std::string>，原因是登录可能成功并返回 sessionId，也可能失败无结果，供登录 HTTP 接口和 WebSocket 建连流程调用。
	// 3)参数类型：const std::string& nickname 与 const std::string& password，原因是登录凭证就是昵称和密码，参数直接对应登录要求。
	std::optional<std::string> AuthServiceImpl::Login(const std::string& nickname, const std::string& password) {
		auto opt = user_store->FindUserByNickname(nickname);
		if (!opt.has_value()) {                                                       //不存在该昵称用户
			return std::nullopt;
		}
		const auto& user = opt.value();
		if (user.password != password){                                             //密码错误
			return std::nullopt;
		}

		std::lock_guard<std::mutex> lock(session_mtx);                               //读写前对两个映射表上锁

		auto it = nickname_map.find(nickname);
		if (it != nickname_map.end()) {                                              //该用户已经登录
			std::string old_sid = it->second;                                        //获取上一次登录的sessionId

			nickname_map.erase(it);
			it = session_map.find(old_sid);
			if (it != session_map.end()) session_map.erase(it);                      //删除上次登录的两个映射记录
		}

		std::string sid;
		do {                                                                         // 循环生成新sessionId，避免与已有sessionId重复
			sid = GenerateSessionId();
		} while (session_map.find(sid)!=session_map.end());

		session_map.emplace( sid ,nickname );
		nickname_map.emplace( nickname, sid );                                      //向两个映射表中加入新登录用户

		return sid;                                                                  //返回sessionId
	}

	// 1)代码逻辑：根据 sessionId 注销当前登录态，并触发相关在线状态和私聊会话清理。
	// 2)返回值类型：bool，原因是登出只需要表达该 sessionId 是否有效并是否成功清理，供登出 HTTP 接口调用。
	// 3)参数类型：const std::string& sessionId，原因是客户端登录后只持有 sessionId，参数直接对应登录态设计。
	bool AuthServiceImpl::Logout(const std::string& sessionId) {

		std::lock_guard<std::mutex> lock(session_mtx);                               //读取之前对映射表上锁

		auto it = session_map.find(sessionId);
		if (it == session_map.end()) {                                               //无效的sessionId
			return false;
		}

		std::string nickname = it->second;

		session_map.erase(it);                                                      //删除sessionId->nickname映射记录                           

		it = nickname_map.find(nickname);
		if (it != nickname_map.end()) {
			nickname_map.erase(it);                                                 //删除nickname->sessionId映射记录
		}

		return true;
	}


	// 1)代码逻辑：根据 sessionId 解析当前登录用户昵称，用于受保护 HTTP 接口和 WebSocket 连接鉴权。
	// 2)返回值类型：std::optional<std::string>，原因是 sessionId 可能有效也可能失效，供聊天模块和 WebSocket 模块调用。
	// 3)参数类型：const std::string& sessionId，原因是该值是系统唯一登录态凭证，参数直接对应鉴权逻辑。
	std::optional<std::string> AuthServiceImpl::ValidateSession(const std::string& sessionId) const {

		std::lock_guard<std::mutex> lock(session_mtx);                               //读取之前对映射表上锁

		auto it = session_map.find(sessionId);
		if (it == session_map.end()) {                                               //无效的sessionId
			return std::nullopt;
		}

		return  it->second;
	}
}