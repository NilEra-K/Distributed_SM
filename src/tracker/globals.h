// 跟踪服务器
// 声明全局变量

#pragma once
#include <vector>
#include <string>
#include <map>
#include <list>
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include "types.h"

// 配置信息
extern char* cfg_appids;                // 应用 ID 表, 例如视频应用、地图功能、叫车服务等...
extern char* cfg_maddrs;                // MySQL 地址列表
extern char* cfg_raddrs;                // Redis 地址列表
extern acl::master_str_tbl cfg_str[];   // 字符串配置表, master_str_tbl 本质是一个结构体

extern int cfg_interval;                // 存储服务器状态检查间隔秒数
extern int cfg_mtimeout;                // MySQL 读写超时, 超过这个时间未完成读写, 就放弃读写, 防止死等
extern int cfg_maxconns;                // Redis 连接池最大连接数
extern int cfg_ctimeout;                // Redis 连接超时
extern int cfg_rtimmout;                // Redis 读写超时
extern int cfg_ktimeout;                // Redis 键超时
extern acl::master_int_tbl cfg_int[];   // 整型配置表, master_int_tbl 本质是一个结构体

extern std::vector<std::string> g_appids;   // 使用 split 拆分后的 应用ID表, 通过 cfg_appids 拆分, 方便使用
extern std::vector<std::string> g_maddrs;   // 使用 split 拆分后的 MySQL 地址列表, 通过 cfg_maddrs 拆分, 方便使用
extern std::vector<std::string> g_raddrs;   // 使用 split 拆分后的 Redis 地址列表, 通过 cfg_raddrs 拆分, 方便使用
extern acl::redis_client_pool* g_rconns;    // Redis连接池
extern std::string g_hostname;              // 主机名

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
extern std::map<std::string, std::list<storage_info_t>> g_groups;   // 组表, 保存 存储服务器信息
extern pthread_mutex_t g_mutex;             // map 容器是非线程安全的, 添加互斥锁

 
