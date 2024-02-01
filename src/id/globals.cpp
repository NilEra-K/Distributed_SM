// ID服务器
// 定义全局变量

#include "globals.h"

// 配置信息
char* cfg_maddrs;                 // MySQL 地址列表
acl::master_str_tbl cfg_str[] = { // 字符串配置表, master_str_tbl 实际为一个结构体, 传入值以及为何传入需要查看 master_str_tbl 结构的具体实现
    {"mysql_addrs", "127.0.0.1",      &cfg_maddrs},
    {0, 0, 0}
};

int cfg_mtimeout;                // MySQL 读写超时, 超过这个时间未完成读写, 就放弃读写, 防止死等
int cfg_maxoffset;               // 最大偏移
acl::master_int_tbl cfg_int[]{   // 整型配置表, master_int_tbl 实际为结构体, 传入值以及为何传入需要查看 master_int_tbl 具体实现
                                 // 传入的 {X, X, X, 0, 0}, 0 是表示没有使用该结构体里的变量
    {"mysql_rw_timeout",       30, &cfg_mtimeout, 0, 0},
    {"idinc_max_step",        100, &cfg_maxoffset, 0, 0},    // ID增量的最大步长: 具体含义可以查看 `Distributed_SM/README.md` 文档中对 ID服务器的说明
    {0, 0, 0, 0, 0}
};

std::vector<std::string> g_maddrs;   // 默认无参构造, MySQL 地址表
std::string g_hostname;              // 默认无参构造, 主机名
std::vector<id_pair_t> g_ids;        // ID 表
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;         // 互斥锁初始化为无锁状态
