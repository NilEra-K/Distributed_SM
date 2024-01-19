// 公共模块
// 实现实用函数
#pragma once

#include <string.h>
#include "types.h"
#include "utils.h"

// long long 类型整数 主机序转网络序, 过程如下, lton 和 ston 同理
// X 为 1个字节, 一个字节有 8 bit
// # 表示该部分被截断
// X X X X X X X X | => 移出缓冲区
//               X | X X X X X X X     ll >> 56 表示 ll 右移 56 位
//               ^
//               n[0]

// X X X X X X X X | => 移出缓冲区
//             # X | X X X X X X       ll >> 48 表示 ll 右移 48 位
//               ^
//               n[1]

// X X X X X X X X | => 移出缓冲区
//           # # X | X X X X X         ll >> 40 表示 ll 右移 40 位
//               ^
//               n[2]
void llton(long long ll, char* n) {
    for (size_t i = 0; i < sizeof(ll); ++i) {
        n[i] = ll >> (sizeof(ll) - i - 1) * 8;
    }
}

// long long 类型整数 网络序转主机序, 过程如下
// 0 0 0 0 0 0 0 0
// X                 => X 0 0 0 0 0 0 0
// ^
// n[0]

// 0 0 0 0 0 0 0 0
//   X               => 0 X 0 0 0 0 0 0
//   ^
//   n[1]

// 0 | 0 = 0; // 0 | 1 = 1; // 1 | 0 = 1; // 1 | 1 = 1; 
// 所以 X 0 0 0 0 0 0 0 | 0 X 0 0 0 0 0 0 = X X 0 0 0 0 0 0
long long ntoll(const char* n) {
    long long ll = 0;
    for (size_t i = 0; i < sizeof(ll); ++i) {
        // ll |= X; 等价于 ll = ll | X;
        // 先将 n[i] 转化为 unsigned char, 这就表示最高位不按符号位看, 防止拓展后填充符号
        // 然后将其转化为 long long, 将 n[i] 拓展到 64 位, 填充为 0
        ll |= (long long)(unsigned char)n[i] << (sizeof(ll) - i - 1) * 8;
    }
    return ll;
}

// long 类型整数 主机序转网络序
void lton(long l, char* n) {
    for (size_t i = 0; i < sizeof(l); ++i) {
        n[i] = l >> (sizeof(l) - i - 1) * 8;
    }
}

// long 类型整数 网络序转主机序
long ntol(const char* n) {
    long l = 0;
    for (size_t i = 0; i < sizeof(l); ++i) {
        l |= (long long)(unsigned char)n[i] << (sizeof(l) - i - 1) * 8;
    }
    return l;
}

// short 类型整数 主机序转网络序
void ston(short s, char* n) {
    for (size_t i = 0; i < sizeof(s); ++i) {
        n[i] = s >> (sizeof(s) - i - 1) * 8;
    }
}

// short 类型整数 网络序转主机序
short ntos(const char* n) {
    short s = 0;
    for (size_t i = 0; i < sizeof(s); ++i) {
        s |= (long long)(unsigned char)n[i] << (sizeof(s) - i - 1) * 8;
    }
    return s;
}

// 字符串是否合法, 即是否只包含 26 个英文的大小写和 0-9 的阿拉伯数字
int valid(const char* str) {
    if (!str)   return ERROR;   // 如果字符串为 nullptr, 则返回 ERROR

    size_t len = strlen(str);
    if (!len)   return ERROR;   // 如果字符串长度为 0, 则返回 ERROR

    for (size_t i = 0; i < len; ++i) {
        if(!(('a' <= str[i] && str[i] <= 'z') || ('A' <= str[i] && str[i] <= 'Z') || ('0' <= str[i] && str[i] <= '9'))) {
            return ERROR;
        }
    }
    return OK;
}

// 以分号为分隔符, 将一个字符串拆分为多个子字符串
// 使用 strtok 来完成这个函数
int split(const char* str, std::vector<std::string>& substrs) {

    return OK;
}
