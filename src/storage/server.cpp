// 存储服务器
// 实现服务器类
#include <unistd.h>
#include "proto.h"
#include "utils.h"
#include "globals.h"
#include "service.h"
#include "server.h"

// 在进程启动时被调用
void server_c::proc_on_init(void) {
    // 隶属组名
    // <STORAGE_GROUPNAME_MAX
    if (strlen(cfg_gpname) > STORAGE_GROUPNAME_MAX) {
        logger_fatal("Groupname Too Big %lu > %d", strlen(cfg_gpname), STORAGE_GROUPNAME_MAX);
    }

    // 绑定端口号
    // >0
    if (cfg_bindport <= 0) {
        logger_fatal("Innvalid Bind Port %d <= 0", cfg_bindport);
    }

    // 存储路径表
    if (!cfg_spaths || !strlen(cfg_spaths)) {
        logger_fatal("Storage Paths is NULL");
    }
    split(cfg_spaths, g_spaths);
    if (g_spaths.empty()) {
        logger_fatal("Storage Paths is Empty");
    }

    // 跟踪服务器地址表
    if (!cfg_taddrs || !strlen(cfg_taddrs)) {
        logger_fatal("Tracker Addresses is NULL");
    }
    split(cfg_taddrs, g_taddrs);
    if (g_taddrs.empty()) {
        logger_fatal("Tracker Addresses is Empty");
    }

    // ID服务器地址表
    if (!cfg_iaddrs || !strlen(cfg_iaddrs)) {
        logger_fatal("ID Addresses is NULL");
    }
    split(cfg_iaddrs, g_iaddrs);
    if (g_iaddrs.empty()) {
        logger_fatal("ID Addresses is Empty");
    }

    // MySQL地址表
    if (!cfg_maddrs || !strlen(cfg_maddrs)) {
        logger_fatal("MySQL Addresses Is NULL!");
    }

    split(cfg_maddrs, g_maddrs);
    if (g_maddrs.empty()) {
        logger_fatal("MySQL Addresses IS EMPTY!");
    }

    // Redis地址表
    if (!cfg_raddrs || !strlen(cfg_raddrs)) {
        logger_error("Redis Addresses Is NULL...");
    } else {
        split(cfg_raddrs, g_raddrs);
        if (g_maddrs.empty()) {
            logger_error("Redis Addresses IS EMPTY...");
        } else {
            // 遍历 Redis 地址表, 尝试创建链接池
            for (std::vector<std::string>::const_iterator raddr = g_raddrs.begin(); raddr != g_raddrs.end(); ++raddr) {
                if ((g_rconns = new acl::redis_client_pool(raddr->c_str(), cfg_maxconns))) {
                    // 设置 Redis 连接超时和读写超时
                    g_rconns->set_timeout(cfg_ctimeout, cfg_rtimeout);
                    break;
                }
            }
            if (!g_rconns) {
                logger_error("Create Redis Connection Pool Fail, `cfg_raddrs: %s`", cfg_raddrs);
            }
        }
    }

    // 主机名
    char hostname[256 + 1] = {};
    if (gethostname(hostname, sizeof(hostname) - 1)) {
        logger_error("Call `gethostname` Fail: %s", strerror(errno));
    }
    g_hostname = hostname;

    // 启动时间
    g_stime = time(NULL);

    // 创建并开启连接每台跟踪服务器的客户机线程
    for (std::vector<std::string>::const_iterator taddr = g_taddrs.begin(); taddr != g_taddrs.end(); ++taddr) {
        tracker_c* tracker = new tracker_c(taddr->c_str());
        tracker->set_detachable(false);     // 设置为可汇合的线程, 方便后续回收
        tracker->start();
        m_trackers.push_back(tracker);
    }

    
    // 打印配置信息
    logger("cfg_gpname: %s, "
           "cfg_spaths: %s, "
           "cfg_taddrs: %s, "
           "cfg_iaddrs: %s, "
           "cfg_maddrs: %s, "
           "cfg_raddrs: %s, "
           "cfg_bindport: %d, "
           "cfg_interval: %d, "
           "cfg_mtimeout: %d, "
           "cfg_maxconns: %d, "
           "cfg_ctimeout: %d, "
           "cfg_rtimeout: %d, "
           "cfg_ktimeout: %d",
           cfg_gpname,
           cfg_spaths,
           cfg_taddrs,
           cfg_iaddrs,
           cfg_maddrs, 
           cfg_raddrs, 
           cfg_bindport,
           cfg_interval, 
           cfg_mtimeout, 
           cfg_maxconns, 
           cfg_ctimeout, 
           cfg_rtimeout,
           cfg_ktimeout);
}

// 在进程意图退出时被调用
// 如果返回 `true`, 进程立即退出
// 否则分两种情况: 
// 检查配置项 `ioctl_quick_abort` 非 0, 程序立即退出
// 检查配置项 `ioctl_quick_abort` 为 0, 等待所有客户机连接都关闭后, 进程再退出
bool server_c::proc_exit_timer(size_t nclients, size_t nthreads) {
    // 终止跟踪客户机线程
    for (std::list<tracker_c*>::iterator tracker = m_trackers.begin(); tracker != m_trackers.end(); ++tracker) {
        (*tracker)->stop();
    }

    if (!nclients || !nthreads) { // 当 `nclient` 为0时 或 `nthreads` 为0时
        logger("nclients: %lu, nthreads: %lu", nclients, nthreads);
        return true;
    }    
    return false;
}

// 在进程即将退出时被调用
void server_c::proc_on_exit(void) {
    for (std::list<tracker_c*>::iterator tracker = m_trackers.begin(); tracker != m_trackers.end(); ++tracker) {
        // 回收跟踪客户机线程
        if (!(*tracker)->wait(NULL)) {
            logger_error("Wait Thread #%lu Fail", (*tracker)->thread_id());
        }
        // 销毁跟踪客户机线程
        delete *tracker;
    }

    m_trackers.clear();

    // 销毁 Redis 连接池
    if (g_rconns) {
        delete g_rconns;
        g_rconns = NULL;
    }
}

/* 以下三个函数返回 `true` 连接会继续保持, 否则连接将被关闭 */
// 在线程获得连接时被调用
// * 传统的线程开辟方法无法控制线程数, 又可能会导致一个进程有过多的线程, 以及一个操作系统中存在过多的线程
// 因此 ACL 框架使用 线程池 的方式来解决这一问题
// * 在之前的学习中, 当建立连接后进行读操作, 此时会阻塞在读操作上, 因此我们希望有一种操作, 可以让我们知道是否有数据可读
// 因此使用 多路IO 操作, 使得我们可以能够在该读的时候读
bool server_c::thread_on_accept(acl::socket_stream* conn) {
    logger("[Storage] Connect, From: %s", conn->get_peer());
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
        }
        return false;
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
    logger("[Storage] Client Disconnect, From: %s", conn->get_peer());
}

