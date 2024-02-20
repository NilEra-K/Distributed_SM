// 存储服务器
// 定义全局变量

#include "globals.h"

// 配置信息
char* cfg_gpname;                 // 隶属组名
char* cfg_spaths;                 // 存储路径表
char* cfg_taddrs;                 // 跟踪服务器地址表
char* cfg_iaddrs;                 // ID服务器地址表
char* cfg_maddrs;                 // MySQL 地址列表
char* cfg_raddrs;                 // Redis 地址列表
acl::master_str_tbl cfg_str[] = { // 字符串配置表, master_str_tbl 实际为一个结构体, 传入值以及为何传入需要查看 master_str_tbl 结构的具体实现
    {"tnv_group_name",    "group001",           &cfg_gpname},
    {"tnv_store_paths",   "../data",            &cfg_spaths},
    {"tnv_tracker_addrs", "127.0.0.1:21000",    &cfg_taddrs},
    {"tnv_ids_addrs",     "127.0.0.1:22000",    &cfg_iaddrs},
    {"mysql_addrs",       "127.0.0.1",          &cfg_maddrs},
    {"redis_addrs",       "127.0.0.1:6379",     &cfg_raddrs},
    {0, 0, 0}
};

int cfg_bindport;                // 绑定端口号
int cfg_interval;                // 心跳间隔秒数
int cfg_mtimeout;                // MySQL 读写超时, 超过这个时间未完成读写, 就放弃读写, 防止死等
int cfg_maxconns;                // Redis 连接池最大连接数
int cfg_ctimeout;                // Redis 连接超时
int cfg_rtimmout;                // Redis 读写超时
int cfg_ktimeout;                // Redis 键超时
acl::master_int_tbl cfg_int[]{   // 整型配置表, master_int_tbl 实际为结构体, 传入值以及为何传入需要查看 master_int_tbl 具体实现
                                 // 传入的 {X, X, X, 0, 0}, 0 是表示没有使用该结构体里的变量
    {"tnv_storage_port",        23000, &cfg_bindport, 0, 0},
    {"tnv_heart_beat_interval",    10, &cfg_interval, 0, 0},
    {"mysql_rw_timeout",           30, &cfg_mtimeout, 0, 0},
    {"redis_max_conn_num",        600, &cfg_maxconns, 0, 0},
    {"redis_conn_timeout",         10, &cfg_ctimeout, 0, 0},
    {"redis_rw_timeout",           10, &cfg_ctimeout, 0, 0},
    {"redis_key_timeout",          60, &cfg_ktimeout, 0, 0},
    {0, 0, 0, 0, 0}
};

std::vector<std::string> g_spaths;   // 存储路径表
std::vector<std::string> g_taddrs;   // 跟踪服务器地址表
std::vector<std::string> g_iaddrs;   // ID服务器地址表
std::vector<std::string> g_maddrs;   // MySQL地址表
std::vector<std::string> g_raddrs;   // Redis地址表
acl::redis_client_pool* g_rconns;    // Redis链接池
std::string g_hostname;              // 主机名
const char* g_version = "1.0";       // 版本
time_t g_stime;                      // 启动时间
