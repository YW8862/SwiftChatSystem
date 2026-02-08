#pragma once

#include <cstdint>

namespace swift {

/// 会话类型：与 proto 中 chat_type 字段取值一致
enum class ChatType : int32_t {
    PRIVATE = 1,
    GROUP = 2,
};

}  // namespace swift
