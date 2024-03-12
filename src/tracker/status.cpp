// 跟踪服务器
// 实现存储服务器状态检查线程类

#include <unistd.h>
#include "globals.h"
#include "status.h"

// 构造函数
status_c::status_c(void): m_stop(false) { }

// 线程终止
void status_c::stop(void) {
    m_stop = true;
}

// 线程执行过程
void* status_c::run(void) {
    for (time_t last = time(NULL); !m_stop; sleep(1)) { // 需要注意这里没有对 `last` 进行迭代, `last` 的更新在下面的代码中
        // 现在的时间
        time_t now = time(NULL);

        // 若现在距离最近一次检查存储服务器状态已足够久
        if (now - last >= cfg_interval) {
            // 检查存储服务器的状态
            check();

            // 更新最近一次检查时间
            last = now;
        }
    }
    return NULL;
}

// 检查存储服务器的状态
int status_c::check(void) const {
    // 获取现在的时间
    time_t now = time(NULL);

    // 互斥锁加锁
    if ((errno = pthread_mutex_lock(&g_mutex))) {
        logger_error("Call `pthread_mutex_lock` Fail: %s", strerror(errno));
        return ERROR;
    }

    // 遍历组表中的每一个组
    for (std::map<std::string, std::list<storage_info_t>>::iterator group = g_groups.begin(); group != g_groups.end(); ++group) {
        // 遍历该组每一台存储服务器
        for (std::list<storage_info_t>::iterator si = group->second.begin(); si != group->second.end(); ++si) {
            // 若该组的存储服务器心跳停止太久
            if (now - si->si_btime >= cfg_interval) {
                // 将其状体标记为离线
                si->si_status = STORAGE_STATUS_OFFLINE;
            }
        }
    }

    // 互斥锁解锁
    if ((errno = pthread_mutex_unlock(&g_mutex))) {
        logger_error("Call `pthread_mutex_lock` Fail: %s", strerror(errno));
        return ERROR;
    }
    return OK;
}


