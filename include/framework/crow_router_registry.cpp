#include "framework/crow_router_registry.h"

#include <utility>

namespace chatroom::framework {

void CrowRouterRegistry::AddRegistrar(RouteRegistrar registrar) {
    registrars_.push_back(std::move(registrar));
}

void CrowRouterRegistry::RegisterAll(ChatroomApp& app) {
    for (auto& registrar : registrars_) {
        registrar(app);
    }
}

}  // namespace chatroom::framework
