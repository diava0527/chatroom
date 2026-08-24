#pragma once

#include "auth_controller.h"
#include "auth_service.h"
#include "crow_all.h"
#include <memory>
#include<optional>

namespace chatroom::user {

	class AuthControllerImpl : public  AuthController{

	private:
		std::shared_ptr<AuthService> auth_service_;         //共享指针使用AuthService的接口(末尾加下划线避免与文件重名)

	public:

		explicit AuthControllerImpl(std::shared_ptr<AuthService> service);

		// 1)代码逻辑：接收注册 HTTP 请求、解析昵称和密码、调用注册服务并返回统一响应。
		// 2)返回值类型：crow::response，原因是注册接口直接对外提供 HTTP 服务，供 /api/v1/auth/register 路由调用。
		// 3)参数类型：const crow::request&，原因是注册参数来自原始 HTTP 请求体，参数直接对应 Crow 路由签名。
		crow::response Register(const crow::request& request)override;

		// 1)代码逻辑：接收登录 HTTP 请求、校验输入、调用登录服务并在成功时返回 sessionId。
		// 2)返回值类型：crow::response，原因是登录接口最终要输出标准 JSON 响应，供 /api/v1/auth/login 路由调用。
		// 3)参数类型：const crow::request&，原因是登录参数和请求头都从 HTTP 请求对象中解析，参数直接对应 Crow 路由签名。
		crow::response Login(const crow::request& request)override;

		// 1)代码逻辑：接收登出 HTTP 请求、提取 sessionId、调用登出服务并返回处理结果。
		// 2)返回值类型：crow::response，原因是登出接口也是标准 HTTP 对外接口，供 /api/v1/auth/logout 路由调用。
		// 3)参数类型：const crow::request&，原因是 sessionId 通过请求头传递，参数直接对应 Crow 路由签名。
		crow::response Logout(const crow::request& request)override;

	};






}