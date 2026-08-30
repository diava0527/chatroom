#pragma once

#include <functional>
#include <vector>

#include "framework/router_registry.h"

namespace chatroom::framework {

class CrowRouterRegistry final : public RouterRegistry {
public:
    using RouteRegistrar = std::function<void(ChatroomApp&)>;

    void AddRegistrar(RouteRegistrar registrar);
    void RegisterAll(ChatroomApp& app) override;

private:
    std::vector<RouteRegistrar> registrars_;
};

}  // namespace chatroom::framework
