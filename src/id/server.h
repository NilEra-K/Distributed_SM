// ID服务器
// 声明服务器类
#pragma once

#include <acl-lib/acl_cpp/lib_acl.hpp>

// 服务器类
class server_c: public acl::master_threads {
protected:
    // 在进程启动时被调用
    void proc_on_init(void);

    // 在进程意图退出时被调用
    // 如果返回 `true`, 进程立即退出
    // 否则分两种情况: 
    // 检查配置项 `ioctl_quick_abort` 非 0, 程序立即退出
    // 检查配置项 `ioctl_quick_abort` 为 0, 等待所有客户机连接都关闭后, 进程再退出
    bool proc_exit_timer(size_t nclients, size_t nthreads);

    /* 以下三个函数返回 `true` 连接会继续保持, 否则连接将被关闭 */
    // 在线程获得连接时被调用
    bool thread_on_accept(acl::socket_stream* conn);

    // 与线程绑定的连接可读时被调用
    bool thread_on_read(acl::socket_stream* conn);

    // 当连接发生读写超时时被调用
    // 一个线程长时间没有读操作, 写操作, 也没有出现错误, 长时间处于无声无息的状态
    bool thread_on_timeout(acl::socket_stream* conn);

    // 当连接关闭时候被调用
    void thread_on_close(acl::socket_stream* conn);
};

