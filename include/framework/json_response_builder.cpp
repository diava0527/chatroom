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
        responseBody["data"] = *data;
    } else {
        responseBody["data"] = nullptr;
    }

    crow::response response;
    response.code = 200;
    response.set_header("Content-Type", "application/json; charset=utf-8");
    response.body = crow::json::dump(responseBody);
    return response;
}

}  // namespace chatroom::framework
