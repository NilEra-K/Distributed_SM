// 存储服务器
// 声明全局变量

#pragma once
#include <vector>
#include <string>
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include "types.h"

// 配置信息
extern char* cfg_gpname;                // [新增] 隶属组名
extern char* cfg_spaths;                // [新增] 存储路径表
extern char* cfg_taddrs;                // [新增] 跟踪服务器地址表
extern char* cfg_iaddrs;                // [新增] ID服务器地址表
extern char* cfg_maddrs;                // MySQL 地址列表
extern char* cfg_raddrs;                // Redis 地址列表
extern acl::master_str_tbl cfg_str[];   // 字符串配置表, master_str_tbl 本质是一个结构体

extern int cfg_bindport;                // [新增] 绑定端口号
extern int cfg_interval;                // [含义变化] 心跳间隔秒数
extern int cfg_mtimeout;                // MySQL 读写超时, 超过这个时间未完成读写, 就放弃读写, 防止死等
extern int cfg_maxconns;                // Redis 连接池最大连接数
extern int cfg_ctimeout;                // Redis 连接超时
extern int cfg_rtimeout;                // Redis 读写超时
extern int cfg_ktimeout;                // Redis 键超时
extern acl::master_int_tbl cfg_int[];   // 整型配置表, master_int_tbl 本质是一个结构体

extern std::vector<std::string> g_spaths;   // [新增] 存储路径表
extern std::vector<std::string> g_taddrs;   // [新增] 跟踪服务器地址表
extern std::vector<std::string> g_iaddrs;   // [新增] ID服务器地址表
extern std::vector<std::string> g_maddrs;   // 使用 split 拆分后的 MySQL 地址列表, 通过 cfg_maddrs 拆分, 方便使用
extern std::vector<std::string> g_raddrs;   // 使用 split 拆分后的 Redis 地址列表, 通过 cfg_raddrs 拆分, 方便使用
extern acl::redis_client_pool* g_rconns;    // Redis连接池
extern std::string g_hostname;              // 主机名
extern const char* g_version;               // [新增] 版本
extern time_t g_stime;                      // [新增] 启动时间
 
