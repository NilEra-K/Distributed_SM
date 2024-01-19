// 公共模块
// 声明实用函数
#pragma once

#include <string>
#include <vector>

// long long 类型整数 主机序转网络序
void llton(long long ll, char* n);

// long long 类型整数 网络序转主机序
long long ntoll(const char* n);

// long 类型整数 主机序转网络序
void lton(long l, char* n);

// long 类型整数 网络序转主机序
long ntol(const char* n);

// short 类型整数 主机序转网络序
void ston(short s, char* n);

// short 类型整数 网络序转主机序
short ntos(const char* n);

// 字符串是否合法, 即是否只包含 26 个英文的大小写和 0-9 的阿拉伯数字
int valid(const char* str);

// 以分号为分隔符, 将一个字符串拆分为多个子字符串
int split(const char* str, std::vector<std::string>& substrs);
