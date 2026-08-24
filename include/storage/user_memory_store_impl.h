#pragma once
#include "user_memory_store.h"
#include "models/user.h"
#include <vector>
#include <optional>
#include <mutex>                        //给数据上锁，防止多线程并发读写时出现竞争等

namespace chatroom::storage {

    class UserMemoryStoreImpl : public UserMemoryStore {

	private:
		std::vector<chatroom::models::User> UserLists;        //用于存储注册过的用户列表
        mutable std::mutex mtx;                               //可变的独占锁
	public:

        // 1)代码逻辑：把注册成功的用户保存到内存容器中，并保证昵称不可重复。
        // 2)返回值类型：bool，原因是保存动作只需要表达是否写入成功，供注册接口调用。
        // 3)参数类型：const chatroom::models::User&，原因是用户存储需要完整用户实体并避免不必要拷贝，参数直接对应注册持久化要求。
        bool SaveUser(const chatroom::models::User& user)override;

        // 1)代码逻辑：按昵称读取用户信息，用于登录校验和其他用户相关业务。
        // 2)返回值类型：std::optional<chatroom::models::User>，原因是用户可能存在也可能不存在，供登录接口和用户查询接口调用。
        // 3)参数类型：const std::string& nickname，原因是昵称是唯一用户标识，参数直接对应“昵称代替账号”的要求。
        std::optional<chatroom::models::User>FindUserByNickname(const std::string& nickname) const override;

        // 1)代码逻辑：读取当前全部内存用户数据，用于调试、管理或后续扩展。
        // 2)返回值类型：std::vector<chatroom::models::User>，原因是全量用户天然是列表结构，供用户模块内部接口调用。
        // 3)参数类型：无参数，原因是该接口读取的是全量用户存储状态。
        std::vector<chatroom::models::User> ListUsers() const override;
     };
};

