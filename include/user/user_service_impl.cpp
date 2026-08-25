#include "user_service_impl.h"
namespace chatroom::user {

	//构造函数：需要指向UserMemoryStore对象的共享指针，move转移值传递的形参以优化性能
	UserServiceImpl::UserServiceImpl(std::shared_ptr<storage::UserMemoryStore> store) : user_store(std::move(store)){}

	//功能：调用UserMemoryStore内部接口在用户列表UserLists中查找昵称为nickname的用户并返回
	std::optional<chatroom::models::User> UserServiceImpl::FindUserByNickname(const std::string& nickname) const {
		return user_store->FindUserByNickname(nickname);
	}

	//功能：调用UserMemoryStore内部接口返回用户列表
	std::vector<chatroom::models::User> UserServiceImpl::ListUsers() const {
		return user_store->ListUsers();
	}
}