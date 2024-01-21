// 跟踪服务器
// 定义全局变量

#include "globals.h"

// 配置信息
char* cfg_appids;                 // 应用 ID 表, 例如视频应用、地图功能、叫车服务等...
char* cfg_maddrs;                 // MySQL 地址列表
char* cfg_raddrs;                 // Redis 地址列表
acl::master_str_tbl cfg_str[] = { // 字符串配置表, master_str_tbl 实际为一个结构体, 传入值以及为何传入需要查看 master_str_tbl 结构的具体实现
    {"tnv_apps_id", "tnvideo",        &cfg_appids}, // tnv_apps_id 为键, 默认值为 "tnvideo", 该配置存储在 cfg_appids
    {"mysql_addrs", "127.0.0.1",      &cfg_maddrs},
    {"redis_addrs", "127.0.0.1:6379", &cfg_raddrs},
    {0, 0, 0}
};

int cfg_interval;                // 存储服务器状态检查间隔秒数
int cfg_mtimeout;                // MySQL 读写超时, 超过这个时间未完成读写, 就放弃读写, 防止死等
int cfg_maxconns;                // Redis 连接池最大连接数
int cfg_ctimeout;                // Redis 连接超时
int cfg_rtimmout;                // Redis 读写超时
int cfg_ktimeout;                // Redis 键超时
acl::master_int_tbl cfg_int[]{   // 整型配置表, master_int_tbl 实际为结构体, 传入值以及为何传入需要查看 master_int_tbl 具体实现
                                 // 传入的 {X, X, X, 0, 0}, 0 是表示没有使用该结构体里的变量
    {"check_active_interval", 120, &cfg_interval, 0, 0},
    {"mysql_rw_timeout",       30, &cfg_mtimeout, 0, 0},
    {"redis_max_conn_num",    600, &cfg_maxconns, 0, 0},
    {"redis_conn_timeout",     10, &cfg_ctimeout, 0, 0},
    {"redis_rw_timeout",       10, &cfg_ctimeout, 0, 0},
    {"redis_key_timeout",      60, &cfg_ktimeout, 0, 0},
    {0, 0, 0, 0, 0}
};

std::vector<std::string> g_appids;   // 默认无参构造
std::vector<std::string> g_maddrs;   // 默认无参构造
std::vector<std::string> g_raddrs;   // 默认无参构造
acl::redis_client_pool* g_rconns;    // 指针未初始化, 在全局域默认为空指针
std::string g_hostname;              // 默认无参构造

/**
 * 跟踪服务器维护的结构类似于下面的结构——即组表
 *          map
 *        /     \
 *   string     list<storage_info_t>
 * --------     ----------------------
 * group001 --> [storage_info_t][][]
 * group002 --> [storage_info_t][]
 * group003 --> [storage_info_t][][][]
*/
std::map<std::string, std::list<storage_info_t>> g_groups;   // 组表, 保存 存储服务器信息
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;         // 互斥锁初始化为无锁状态
