// 存储服务器
// 实现跟踪客户机线程类
#include <unistd.h>
#include "proto.h"
#include "utils.h"
#include "globals.h"
#include "tracker.h"

// 构造函数
tracker_c::tracker_c(const char* taddr) : m_stop(false), m_taddr(taddr) {

}

// 线程终止
void tracker_c::stop(void) {
    m_stop = true;
}

// 线程执行过程
void* tracker_c::run(void) {
    acl::socket_stream conn;
    while (!m_stop) { // 试错循环
        // 连接跟踪服务器
        if (!conn.open(m_taddr, 10, 30)) { // open(const char* addr, int conn_timeout, int rw_timeout)
                                           // -addr: 跟踪服务器地址 -conn_timeout: 连接超时 -rw_timeout: 读写超时
            logger_error("Connect Tracker Fail, taddr: %s", m_taddr.c_str());

            // 失败重连
            sleep(2);
            continue;
        }

        // 向跟踪服务器发送加入包
        if (join(&conn) != OK) {
            conn.close();

            // 失败重连
            sleep(2);
            continue;
        }

        time_t last = time(NULL);       // 上次心跳
        while (!m_stop) { // 心跳循环
            time_t now = time(NULL);    // 现在

            // 现在离上次心跳已足够久, 再跳一次
            if (now - last >= cfg_interval) {
                // 向跟踪服务器发送心跳包
                if (beat(&conn) != OK) {// 失败重连
                    break;
                }
            }
        }
    }
    
    return NULL;
}

// 向跟踪服务器发送加入包
int tracker_c::join(acl::socket_stream* conn) const {
    return OK;
}

// 向跟踪服务器发送心跳包
int tracker_c::beat(acl::socket_stream* conn) const {
    return OK;
}


