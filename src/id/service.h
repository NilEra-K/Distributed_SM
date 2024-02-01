// ID服务器
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
    // 处理来自存储服务器的获取ID请求
    bool get(acl::socket_stream* conn, long long bodylen) const;

    // 根据 ID 的键获取其值
    long get(const char* key) const;

    // 从数据库中获取 ID 值
    long fromdb(const char* key) const;

    // 应答成功
    bool id(acl::socket_stream* conn, long value) const;
    
    // 应答错误
    bool error(acl::socket_stream* conn, short errnumb, const char* format, ...) const;
}; 