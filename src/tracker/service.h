// 跟踪服务器
// 声明业务服务类
#pragma once

#include <acl-lib/acl_cpp/lib_acl.hpp>
#include "types.h"

// 业务服务类
class service_c {
public:
    // 业务处理类
    bool business(acl::socket_stream* conn, const char* head) const;

private:
    // 处理来自存储服务器的加入包
    // 处理来自存储服务器的心跳包
};