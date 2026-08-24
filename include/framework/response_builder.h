#pragma once

#include <optional>
#include <string>

#include "crow_all.h"

namespace chatroom::framework {

class ResponseBuilder {
public:
    virtual ~ResponseBuilder() = default;

    // 1)代码逻辑：把成功业务结果统一封装为 code、message、data 格式的 HTTP JSON 响应，避免不同控制器返回格式不一致。
    // 2)返回值类型：crow::response，原因是 HTTP 控制器最终必须向 Crow 返回标准响应对象，供用户模块和聊天模块 HTTP 接口调用。
    // 3)参数类型：int、const std::string&、const std::optional<std::string>&，原因是统一响应至少需要状态码、描述信息和可选数据载荷，参数直接对应接口规范要求。
    virtual crow::response BuildHttpJson(
        int code,
        const std::string& message,
        const std::optional<std::string>& data) const = 0;
};

}  // namespace chatroom::framework
