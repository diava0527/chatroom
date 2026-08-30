#pragma once

#include <functional>
#include <vector>

#include "framework/router_registry.h"

namespace chatroom::framework {

class CrowRouterRegistry final : public RouterRegistry {
public:
    using RouteRegistrar = std::function<void(crow::SimpleApp&)>;

    // 1)代码逻辑：向路由注册中心追加一个具体注册动作，供 main 函数或上层装配模块分批挂载 HTTP 与 WebSocket 路由。
    // 2)返回值类型：void，原因是这里只是保存注册动作，不直接产出业务结果，供启动装配流程调用。
    // 3)参数类型：RouteRegistrar&&，原因是路由注册动作本身就是可调用对象，右值传入可以减少一次不必要拷贝。
    void AddRegistrar(RouteRegistrar&& registrar);

    // 1)代码逻辑：按保存顺序执行全部路由注册动作，把上层准备好的路由统一挂载到 Crow 应用实例。
    // 2)返回值类型：void，原因是路由装配属于启动副作用，不需要返回额外结果，供 ServerApp 调用。
    // 3)参数类型：crow::SimpleApp&，原因是所有注册动作都必须绑定同一个应用实例，参数直接对应 Crow 使用方式。
    void RegisterAll(crow::SimpleApp& app) override;

private:
    std::vector<RouteRegistrar> registrars_;
};

}  // namespace chatroom::framework
