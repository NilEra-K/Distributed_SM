// 客户机
// 实现连接池类
#include "conn.h"
#include "pool.h"

// 构造函数
// -destaddr: 目的地址
pool_c::pool_c(const char* destaddr, int count, size_t index /* = 0 */) : connect_pool(destaddr, count, index), m_ctimeout(30), m_rtimeout(60), m_itimeout(90) { }

// 设置超时
void pool_c::timeout(int ctimeout /* = 30 */, int rtimeout /* = 60 */, int itimeout /* = 90 */) {
    m_ctimeout = ctimeout;
    m_rtimeout = rtimeout;
    m_itimeout = itimeout;
}

// 获取连接
acl::connect_client* pool_c::peek(void) {
    connect_pool::check_idle(m_itimeout);   // 先清扫掉超过 m_itimeout 的连接
    return connect_pool::peek();
}

// 创建连接
acl::connect_client* pool_c::create_connect(void) {
    return new conn_c(addr_, m_ctimeout, m_rtimeout);   // addr_ 就是构造时传入的 destaddr
}

