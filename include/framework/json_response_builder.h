#pragma once

#include "framework/response_builder.h"

namespace chatroom::framework {

class JsonResponseBuilder final : public ResponseBuilder {
public:
    // 1)代码逻辑：把统一响应协议组装成 Crow 可直接返回的 JSON 响应对象，保证控制器输出格式一致。
    // 2)返回值类型：crow::response，原因是 HTTP 接口最终都要交给 Crow 返回标准响应，供各控制器与占位路由调用。
    // 3)参数类型：int、const std::string&、const std::optional<std::string>&，原因是统一协议固定需要状态码、描述信息和可选数据载荷，参数直接对应响应结构。
    crow::response BuildHttpJson(
        int code,
        const std::string& message,
        const std::optional<std::string>& data) const override;
};

}  // namespace chatroom::framework
