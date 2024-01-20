// 跟踪服务器
// 声明缓存类

#pragma once
#include <acl-lib/acl_cpp/lib_acl.hpp>

// 缓存类
class cache_c {
public:
    // 根据键获取其值
    int get(const char* key, acl::string& value) const;

    // 设置指定键的值
    int set(const char* key, const char* value, int timeout = -1) const;

    // 删除指定键值对
    int del(const char* key) const;
};
