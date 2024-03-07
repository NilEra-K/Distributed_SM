// 客户机
// 实现连接池管理器类

#include "pool.h"
#include "mngr.h"

// 创建连接池
acl::connect_pool* mngr_c::create_pool(const char* destaddr, size_t count, size_t index) {
    return new pool_c(destaddr, count, index);
}
