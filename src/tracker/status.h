// 跟踪服务器
// 声明存储服务器状态检查线程类
#pragma once

#include <acl-lib/acl_cpp/lib_acl.hpp>

// 存储服务器状态检查线程类
class status_c: public acl::thread {
public:
    // 构造函数
    status_c(void);

    // 线程终止
    void stop(void);

protected:
    // 线程执行过程
    void* run(void);

private:
    // 检查存储服务器的状态
    int check(void) const;

    bool m_stop;    // 是否终止, 体面的关闭线程
};

