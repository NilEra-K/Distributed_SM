// 存储服务器
// 声明文件操作类
#pragma once
#include <sys/types.h>

// 文件操作类
class file_c {
public:
    // 构造函数
    file_c(void);

    // 打开标志
    static const char O_READ  = 'r';
    static const char O_WRITE = 'w';
    
    // 打开文件
    int open(const char* path, char flag);  // flag 表示打开标志

    // 关闭文件
    int close(void);

    // 读取文件
    int read(void* buf, size_t count) const;

    // 写入文件
    int write(const void* buf, size_t count) const;

    // 设置偏移
    int seek(off_t offset) const;

    // 删除文件
    static int del(const char* path);

    // 析构函数
    ~file_c(void);

private:
    int m_fd;   // 文件描述符
};

