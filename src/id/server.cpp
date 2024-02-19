// ID服务器
// 实现服务器类
#include <unistd.h>
#include "proto.h"
#include "utils.h"
#include "globals.h"
#include "service.h"
#include "server.h"

// 在进程启动时被调用
void server_c::proc_on_init(void) {
    // MySQL地址表
    if (!cfg_maddrs || !strlen(cfg_maddrs)) {
        logger_fatal("MySQL Addresses Is NULL!");
    }

    split(cfg_maddrs, g_maddrs);
    if (g_maddrs.empty()) {
        logger_fatal("MySQL Addresses IS EMPTY!");
    }

    // 主机名
    char hostname[256 + 1];
    if (gethostname(hostname, sizeof(hostname) - 1)) {
        logger_error("Call `gethostname` Fail: %s", strerror(errno));
    }

    // 最大偏移不能太小
    if (cfg_maxoffset < 10) {
        logger_fatal("Invalid Maximum Offset: %d < 10", cfg_maxoffset);
    }
   
    // 打印配置信息
    logger("cfg_maddrss: %s, "
           "cfg_mtimeout: %d, "
           "cfg_maxoffset: %d",
           cfg_maddrs, 
           cfg_mtimeout, 
           cfg_maxoffset);
}

// 在进程意图退出时被调用
// 如果返回 `true`, 进程立即退出
// 否则分两种情况: 
// 检查配置项 `ioctl_quick_abort` 非 0, 程序立即退出
// 检查配置项 `ioctl_quick_abort` 为 0, 等待所有客户机连接都关闭后, 进程再退出
bool server_c::proc_exit_timer(size_t nclients, size_t nthreads) {
    if (!nclients || !nthreads) { // 当 `nclient` 为0时 或 `nthreads` 为0时
        logger("nclients: %lu, nthreads: %lu", nclients, nthreads);
        return true;
    }    
    return false;
}


/* 以下三个函数返回 `true` 连接会继续保持, 否则连接将被关闭 */
// 在线程获得连接时被调用
// * 传统的线程开辟方法无法控制线程数, 又可能会导致一个进程有过多的线程, 以及一个操作系统中存在过多的线程
// 因此 ACL 框架使用 线程池 的方式来解决这一问题
// * 在之前的学习中, 当建立连接后进行读操作, 此时会阻塞在读操作上, 因此我们希望有一种操作, 可以让我们知道是否有数据可读
// 因此使用 多路IO 操作, 使得我们可以能够在该读的时候读
bool server_c::thread_on_accept(acl::socket_stream* conn) {
    logger("Connect, From: %s", conn->get_peer());
    return true;
}

// 与线程绑定的连接可读时被调用
bool server_c::thread_on_read(acl::socket_stream* conn) {
    // 接收包头
    char head[HEADLEN];
    if (conn->read(head, HEADLEN) < 0) {
        if (conn->eof()) {
            logger("Connection Has Been Closed, From %s", conn->get_peer());
        } else {
            logger_error("Read Fail: %s, From %s", acl::last_serror(), conn->get_peer());
            return false;
        }
    }

    // 业务处理
    service_c service;
    return service.business(conn, head);
}

// 当连接发生读写超时时被调用
// 一个线程长时间没有读操作, 写操作, 也没有出现错误, 长时间处于无声无息的状态
bool server_c::thread_on_timeout(acl::socket_stream* conn) {
    logger("Read Timeout, From: %s", conn->get_peer());
    return true;    // 这里返回 `true` 表示保持连接
}

// 当连接关闭时候被调用
void server_c::thread_on_close(acl::socket_stream* conn) {
    logger("Client Disconnect, From: %s", conn->get_peer());
}

