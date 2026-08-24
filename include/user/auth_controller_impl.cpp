#include "auth_controller_impl.h"

namespace chatroom::user {

	//显式构造函数
	AuthControllerImpl::AuthControllerImpl(std::shared_ptr<AuthService> service):auth_service_(std::move(service)){}

	// 1)代码逻辑：接收注册 HTTP 请求、解析昵称和密码、调用注册服务并返回统一响应。
	// 2)返回值类型：crow::response，原因是注册接口直接对外提供 HTTP 服务，供 /api/v1/auth/register 路由调用。
	// 3)参数类型：const crow::request&，原因是注册参数来自原始 HTTP 请求体，参数直接对应 Crow 路由签名。
	crow::response AuthControllerImpl::Register(const crow::request& request) {

		auto json_body = crow::json::load(request.body);                               //解析请求体

		if (!json_body || !json_body.has("nickname") || !json_body.has("password")) {  //校验是否合法
			return crow::response(400, crow::json::wvalue(
				{ {"code",1004},
				{"message","invalid request"},
				{"data",crow::json::wvalue()} }                                        //构造空对象返回
			));
		}

		auto nick_val = json_body["nickname"];
		auto pwd_val = json_body["password"];
		if (nick_val.t() != crow::json::type::String || pwd_val.t() != crow::json::type::String){
			                                                                           // 参数类型错误
			return crow::response(400, crow::json::wvalue(
				{ {"code",1004},
				{"message","invalid request"},
				{"data",crow::json::wvalue()} }
			));
		}

		std::string nickname = nick_val.s();
		std::string password = pwd_val.s();                                           //提取参数

		if (nickname.empty() || password.empty()) {                                   //参数为空
			return crow::response(400, crow::json::wvalue(
				{ {"code",1004},
				{"message","invalid request"},
				{"data",crow::json::wvalue()} }
			));
		}

		bool flag = auth_service_->Register(nickname, password);                      //调用业务层注册实现

		if (!flag) {
			return crow::response(409, crow::json::wvalue(                           //返回响应
				{ {"code",1001},
				{"message","nickname already exists"},
				{"data",crow::json::wvalue()} }
			));
		}
		else {
			return crow::response(200, crow::json::wvalue(
				{ {"code",0},
				{"message","register success"},
				{"data", {{"nickname", nickname}}} }
			));
		}
	}


	// 1)代码逻辑：接收登录 HTTP 请求、校验输入、调用登录服务并在成功时返回 sessionId。
	// 2)返回值类型：crow::response，原因是登录接口最终要输出标准 JSON 响应，供 /api/v1/auth/login 路由调用。
	// 3)参数类型：const crow::request&，原因是登录参数和请求头都从 HTTP 请求对象中解析，参数直接对应 Crow 路由签名。
	crow::response AuthControllerImpl::Login(const crow::request& request) {
		auto json_body = crow::json::load(request.body);                               //解析请求体

		if (!json_body || !json_body.has("nickname") || !json_body.has("password")) {  //校验是否合法
			return crow::response(400, crow::json::wvalue(
				{ {"code",1002},
				{"message","nickname or password error"},
				{"data",crow::json::wvalue()} }
			));
		}

		auto nick_val = json_body["nickname"];
		auto pwd_val = json_body["password"];
		if (nick_val.t() != crow::json::type::String || pwd_val.t() != crow::json::type::String) {
			// 参数类型错误
			return crow::response(400, crow::json::wvalue(
				{ {"code",1002},
				{"message","nickname or password error"} ,
				{"data",crow::json::wvalue()} }
			));
		}

		std::string nickname = nick_val.s();
		std::string password = pwd_val.s();                                            //提取参数

		if (nickname.empty() || password.empty()) {                                   //参数为空
			return crow::response(400, crow::json::wvalue(
				{ {"code",1002},
				{"message","nickname or password error"} ,
				{"data",crow::json::wvalue()} }
			));
		}

		auto sid = auth_service_->Login(nickname,password);                          //调用业务层登录实现

		if (!sid.has_value()) {                                                      //返回响应
			return crow::response(401, crow::json::wvalue(
				{ {"code",1002},
				{"message","nickname or password error"} ,
				{"data",crow::json::wvalue()} }
			));
		}
		else {
			return crow::response(200, crow::json::wvalue(
				{ {"code",0},
				{"message","login success"},
				{"data", { {"nickname",nickname}, {"sessionId", sid.value()}}}}
			));
		}
	}


	// 1)代码逻辑：接收登出 HTTP 请求、提取 sessionId、调用登出服务并返回处理结果。
	// 2)返回值类型：crow::response，原因是登出接口也是标准 HTTP 对外接口，供 /api/v1/auth/logout 路由调用。
	// 3)参数类型：const crow::request&，原因是 sessionId 通过请求头传递，参数直接对应 Crow 路由签名。
	crow::response AuthControllerImpl::Logout(const crow::request& request) {

		std::string sessionId = request.get_header_value("X-Session-Id");             //从请求头获取sessionId

		if (sessionId.empty()) {                                                      //sessionId为空
			return crow::response(400, crow::json::wvalue(
				{ {"code",1003},
				{"message","sessionId invalid"} ,
				{"data",crow::json::wvalue()} }
			));
		}

		bool flag = auth_service_->Logout(sessionId);                                 //调用业务层登出实现

		if (!flag) {
			return crow::response(401, crow::json::wvalue(                           //返回响应
				{ {"code",1003},
				{"message","sessionId invalid"},
				{"data",crow::json::wvalue()} }
			));
		}
		else {
			return crow::response(200, crow::json::wvalue(
				{ {"code",0},
				{"message","logout success"},
				{"data",crow::json::wvalue()} }
			));
		}


	}
}