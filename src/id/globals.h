// ID服务器
// 声明全局变量

#pragma once
#include <vector>
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include "types.h"

// 配置信息
extern char* cfg_maddrs;                // MySQL 地址列表
extern acl::master_str_tbl cfg_str[];   // 字符串配置表, master_str_tbl 本质是一个结构体

extern int cfg_mtimeout;                // MySQL 读写超时, 超过这个时间未完成读写, 就放弃读写, 防止死等
extern int cfg_maxoffset;               // 最大偏移
extern acl::master_int_tbl cfg_int[];   // 整型配置表, master_int_tbl 本质是一个结构体

extern std::vector<std::string> g_maddrs;   // 使用 split 拆分后的 MySQL 地址列表, 通过 cfg_maddrs 拆分, 方便使用
extern std::string g_hostname;              // 主机名
extern std::vector<id_pair_t> g_ids;        // ID表
extern pthread_mutex_t g_mutex;             // vector 容器是非线程安全的, 添加互斥锁

 
