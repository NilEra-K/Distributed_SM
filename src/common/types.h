// 公共模块
// 定义所有模块都需要用到的宏和数据类型

// 防止头文件被钻石包含时被重复定义
#pragma once

#include <netinet/in.h>

// 函数返回值
#define OK               0	// 成功
#define ERROR	        -1  // 本地错误
#define SOCKET_ERROR    -2  // 套接字通信错误
#define STATUS_ERROR    -3  // 服务器状态异常

// 缓存: 需要考虑到用户的使用。
// 例如: 跟踪服务器和存储服务器都使用了Redis缓存, 不能强制要求用户有两个Redis实例
// 即一个跟踪服务器的Redis实例, 同时有一个存储服务器的Redis实例, 这种要求过于苛刻
// 但是为了避免两个服务器缓存之间的键冲突情况, 即跟踪服务器存在一个 userid, 存储服务器也有一个 userid
// 这种情况下使用一个Redis实例会造成冲突, 所以使用宏定义一个 *前缀*
#define TRACKER_REDIS_PREFIX    "tracker"   // 跟踪服务器 Redis 前缀
#define STORAGE_REDIS_PREFIX    "storage"   // 存储服务器 Redis 前缀    

// 存储服务器状态: 离线、在线、活动
// 离线: 没有收到有关这个存储服务器的任何信息, 或认为这个存储服务器已经无法在通信
// 在线: 当存储服务器启动时, 需要向跟踪服务器发送注册请求, 表明自己已经启动
// 活动: 当能够发送心跳包时, 说明存储服务器已经完成了初始化等一系列工作, 可以正常执行业务
// 有限离散域, 可以使用枚举 enum 类型, 使用 enum 时, 我们往往不关心枚举中的值到底是多少
typedef enum storage_status {
    STORAGE_STATUS_OFFLINE,     // 离线
    STORAGE_STATUS_ONLINE,      // 在线
    STORAGE_STATUS_ACTIVE       // 活动
} storage_status_t;

// 存储服务器加入和信息: 例如 存储服务器的IP地址、端口号...等
// 其中的类型不一致, 使用结构体比较好
#define STORAGE_VERSION_MAX     6   // 版本最大字符数, 不超过 6 字符, 如 0.0.1
#define STORAGE_GROUPNAME_MAX   16  // 组名最大字符数
#define STORAGE_HOSTNAME_MAX    128 // 主机名最大字符数
#define STORAGE_ADDR_MAX        16  // 地址最大字符数, 点分十进制最大占用 15 字符: xxx.xxx.xxx.xxx
typedef struct storage_join {
    char        sj_version[STORAGE_VERSION_MAX + 1];        // 版本号字段, +1 是为了给 '\0' 字符留空间
    char        sj_groupname[STORAGE_GROUPNAME_MAX + 1];    // 组名
    char        sj_hostname[STORAGE_HOSTNAME_MAX + 1];      // 主机名
    in_port_t   sj_port;                                    // 端口号, in_port_t 为 uint16_t 的别名
                                                            // 即 unsigned int 类型
    time_t      sj_stime;                                   // 启动时间
    time_t      sj_jtime;                                   // 加入时间
} storage_join_t;   // 存储服务器加入

typedef struct storage_info { // 保存存储服务器的信息
    // 相对存储服务器加入结构, 删除了组名(作为键, 就无须存储了), 增加了 IP地址、心跳时间、存储服务器状态
    char                si_version[STORAGE_VERSION_MAX + 1];    // 版本号字段, +1 是为了给 '\0' 字符留空间
    char                si_hostname[STORAGE_HOSTNAME_MAX + 1];  // 主机名
    char                si_addr[STORAGE_ADDR_MAX + 1];          // IP 地址
    in_port_t           si_port;                                // 端口号
    time_t              si_stime;                               // 启动时间
    time_t              si_jtime;                               // 加入时间
    time_t              si_btime;                               // 心跳时间
    storage_status_t    si_status;                              // 存储服务器状态
} storage_info_t;   // 存储服务器信息

// ID 键值对
#define ID_KEY_MAX      64  // 键的最大字符数
typedef struct id_pair {
    char id_key[ID_KEY_MAX + 1];    // 键
    long id_value;                  // 值
    int  id_offset;                 // 偏移
} id_pair_t;    // ID 键值对

// 存储服务器读写磁盘文件缓冲区
#define STORAGE_RCVWR_SIZE  (512 * 1024)    // 接收写入缓冲区字节数  512 K
#define STORAGE_RDSND_SIZE  (512 * 1024)    // 读取发送缓冲区字节数  512 K


 
