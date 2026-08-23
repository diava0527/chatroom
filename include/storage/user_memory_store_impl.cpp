#include "user_memory_store_impl.h"

namespace chatroom::storage {

	// 功能：把注册成功的用户保存到内存容器中，并保证昵称不可重复。
	bool UserMemoryStoreImpl::SaveUser(const chatroom::models::User& user) {
		std::lock_guard<std::mutex> lock(mtx);                              // 写入之前自动加锁，出作用域自动解锁
		for (auto const& x : UserLists) {
			if (x.nickname == user.nickname) {                              //昵称已存在
				return false;
			}
		} 
		UserLists.push_back(user);                                         //保存新用户
		return true;
	}

	// 功能：按昵称读取用户信息，用于登录校验和其他用户相关业务。
	std::optional<chatroom::models::User>UserMemoryStoreImpl::FindUserByNickname(const std::string& nickname) const {
		std::lock_guard<std::mutex> lock(mtx);                             // 读取之前自动加锁，出作用域自动解锁
		for (auto const& x : UserLists) {
			if (x.nickname == nickname) {                                  //存在该昵称用户
				return x;
			}
		}
		return std::nullopt;                                              //不存在该昵称用户
	}

	// 功能：代码逻辑：读取当前全部内存用户数据，用于调试、管理或后续扩展。
	std::vector<chatroom::models::User> UserMemoryStoreImpl::ListUsers() const {
		std::lock_guard<std::mutex> lock(mtx);                             // 读取之前自动加锁，出作用域自动解锁
		return UserLists;
	}

}