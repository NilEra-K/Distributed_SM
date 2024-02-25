// 存储服务器
// 声明 ID 客户机类
#pragma once

// ID 客户机类
class id_c {
public:
    // 从 ID服务器获取与 ID键相对应的值
    long get(const char* key) const;

private:
    // 向 ID服务器发送请求, 接收并解析响应, 从中获取ID值
    long client(const char* requ, long long requlen) const;
};
