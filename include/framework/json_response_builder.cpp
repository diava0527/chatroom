#include "framework/json_response_builder.h"

namespace chatroom::framework {

crow::response JsonResponseBuilder::BuildHttpJson(
    int code,
    const std::string& message,
    const std::optional<std::string>& data) const {
    crow::json::wvalue responseBody;
    responseBody["code"] = code;
    responseBody["message"] = message;

    if (data.has_value()) {
        auto parsed = crow::json::load(*data);
        if (parsed) {
            responseBody["data"] = std::move(parsed);
        } else {
            responseBody["data"] = *data;
        }
    } else {
        responseBody["data"] = nullptr;
    }

    crow::response response;
    response.code = 200;
    response.set_header("Content-Type", "application/json; charset=utf-8");
    response.body = responseBody.dump();
    return response;
}

}  // namespace chatroom::framework
