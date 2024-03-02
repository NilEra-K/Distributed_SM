// 存储服务器
// 声明跟踪客户机线程类
#pragma once

#include <acl-lib/acl_cpp/lib_acl.hpp>

// 跟踪客户机线程类
class tracker_c: public acl::thread {
public:
    // 构造函数
    tracker_c(const char* taddr);

    // 线程终止
    void stop(void);

protected:
    // 线程执行过程
    void* run(void);

private:
    // 向跟踪服务器发送加入包
    int join(acl::socket_stream* conn) const;

    // 向跟踪服务器发送心跳包
    int beat(acl::socket_stream* conn) const;

    bool m_stop;            // 是否终止, 体面的关闭线程
    acl::string m_taddr;    // 跟踪服务器地址
};

