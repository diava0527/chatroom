#pragma once

#include "framework/response_builder.h"

namespace chatroom::framework {

class JsonResponseBuilder final : public ResponseBuilder {
public:
    crow::response BuildHttpJson(
        int code,
        const std::string& message,
        const std::optional<std::string>& data) const override;
};

}  // namespace chatroom::framework
