#pragma once
#include "user_service.h"
#include "storage/user_memory_store.h"    //此处用户服务接口的实现需要用到用户存储接口
#include <memory>

namespace chatroom::user {

	class UserServiceImpl : public UserService {     //UserService的子类，实现用户信息的读取

	private:
		std::shared_ptr<storage::UserMemoryStore> user_store;         //共享指针使用UserMemoryStore的接口（因为AuthService子类也要使用）

	public:

		// 构造注入 UserMemoryStore以访问用户列表，explicit防止隐式转换
		explicit UserServiceImpl(std::shared_ptr<storage::UserMemoryStore> store);    
		

		std::optional<chatroom::models::User> FindUserByNickname(const std::string& nickname)const override;    //通过昵称找到某用户

		std::vector<chatroom::models::User> ListUsers()const override;    //返回用户列表

	};
}
