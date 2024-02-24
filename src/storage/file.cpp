// 存储服务器
// 实现文件操作类
#include <unistd.h>
#include <fcntl.h>
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include "types.h"
#include "file.h"

// 构造函数
file_c::file_c(void) : m_fd(-1) { }

// 打开文件
int file_c::open(const char* path, char flag) {
    // 检查路径
    if (!path || !strlen(path)) {
        logger_error("Path is NULL");
        return ERROR;
    }

    // 打开文件
    if (flag == O_READ) {
        m_fd = ::open(path, O_RDONLY);  // ::open() 表示全局域的 open() 函数, 与类内的 open() 作区分
    } else if (flag == O_WRITE) {
        m_fd = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } else {
        logger_error("Unknown Open Flag: %c", flag);
        return ERROR;
    }
    
    // 打开失败
    if (m_fd == -1) {
        logger_error("Call `open()` Fail: %s, Path: %s, Flag: %c", strerror(errno), path, flag);
        return ERROR;
    }

    return OK;
}

// 关闭文件
int file_c::close(void) {
    if (m_fd != -1) {
        if (::close(m_fd) == -1) {
            logger_error("Call `close()` Fail: %s");
            return ERROR;
        }
        m_fd = -1;
    }
    return OK;
}

// 读取文件
int file_c::read(void* buf, size_t count) const {
    // 检查文件描述符
    if (m_fd == -1) {
        logger_error("Invalid File Handle...");
        return ERROR;
    }

    // 检查文件缓冲区
    if (!buf) {
        logger_error("Invalid File Buffer...");
        return ERROR;
    }
    
    // 读取指定字节数
    ssize_t bytes = ::read(m_fd, buf, count);

    // 只有读到 count 时才会返回 OK, 其他一律返回 ERROR
    // ::read -> 第三个参数, 想读多少就读多少, 此时最理想
    //        -> 大于 0 但小于第三个参数, 读出来的数据少于期望读取的数据, 所剩多少
    //        -> 0, 读写指针已经在文件尾处
    //        -> -1, 出错, 具体消息在 errno 中
    if (bytes == -1) {
        logger_error("Call `read` Fail: %s", strerror(errno));
        return ERROR;
    }
    
    if ((size_t)bytes != count) {
        logger_error("Unable to Read Expected Bytes: %ld != %lu", bytes, count);
        return ERROR;
    }

    return OK;
}

// 写入文件
int file_c::write(const void* buf, size_t count) const {
    // 检查文件描述符

    // 检查文件缓冲区

    // 写入给定字节数
    
    return OK;
}

// 设置偏移
int file_c::seek(off_t offset) const {
    return OK;
}

// 删除文件
static int del(const char* path);

// 析构函数
file_c::~file_c(void) {
    close();
}

