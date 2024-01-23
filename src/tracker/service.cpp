// 跟踪服务器
// 实现业务服务类
#include <algorithm>
#include "proto.h"
#include "utils.h"
#include "globals.h"
#include "db.h"
#include "service.h"

/* 一级方法 */
// 业务处理类
bool service_c::business(acl::socket_stream* conn, const char* head) const {
    // | 包体长度 | 命令 | 状态 |  包体   |
    // |    8    |  1  |  1  | 包体长度 |
    // 解析包头 -> 得到包体长度、命令、状态字段
    long long bodylen = ntoll(head);    // 包体长度
    if (bodylen < 0) {
        error(conn, -1, "Invalid Body Length: %lld < 0", bodylen);
        return false;
    }
    int command = head[BODYLEN_SIZE];                   // 其实就是 head[8] 也就是命令字段
    int status = head[BODYLEN_SIZE + COMMAND_SIZE];     // 其实就是 head[9] 也就是状态字段
    logger("Bodylen: %lld, Command: %d, Status: %d", bodylen, command, status);

    // 根据命令执行具体的业务处理
    bool result;
    switch (command) {
    case CMD_TRACKER_JOIN:      // 处理来自存储服务器的加入包
        result = join(conn, bodylen);
        break;

    case CMD_TRACKER_BEAT:      // 处理来自存储服务器的心跳包
        result = beat(conn, bodylen);
        break;

    case CMD_TRACKER_SADDRS:    // 处理来自客户机的获取存储服务器地址列表请求
        result = saddrs(conn, bodylen);
        break;

    case CMD_TRACKER_GROUPS:    // 处理来自客户机的获取组列表的请求
        result = groups(conn);
        break;
    default:
        error(conn, -1, "Unknown Command: %d", command);
        return false;
    }
}

/* 二级方法 */
// 处理来自存储服务器的加入包
bool service_c::join(acl::socket_stream* conn, long long bodylen) const {
    // | 包体长度 | 命令 | 状态 |  storage_join_body_t  |
    // |    8    |  1  |  1  |       包 体 长 度       |
    // 检查包体长度
    long long expected = sizeof(storage_join_body_t);
    if (bodylen != expected) {
        error(conn, -1, "Invalid Body Length: %lld != %lld", bodylen, expected);
        return false;
    }

    // 接收包体
    char body[bodylen];
    if (conn->read(body, bodylen) < 0) {
        logger_error("Read Fail: %s, Bodylen: %lld, From: %s", acl::last_serror(), bodylen, conn->get_peer());
        return false;
    }

    // 解析包体 -> 得到版本、组名、主机名、端口号、启动时间、加入时间
    storage_join_t sj;
    storage_join_body_t* sjb = (storage_join_body_t*)body;
    strcpy(sj.sj_version, sjb->sjb_version);        // 版本
    strcpy(sj.sj_groupname, sjb->sjb_groupname);    // 组名
    if (valid(sj.sj_groupname) != OK) {
        error(conn, -1, "Invalid Groupname: %s", sj.sj_groupname);
        return false;
    }

    strcpy(sj.sj_hostname, sjb->sjb_hostname);      // 主机名
    sj.sj_port = ntos(sjb->sjb_port);               // 端口号
    if (!sj.sj_port) {
        error(conn, -1, "Invalid Port: %u", sj.sj_port);
        return false;
    }

    sj.sj_stime = ntol(sjb->sjb_stime);             // 启动时间
    sj.sj_jtime = ntol(sjb->sjb_jtime);             // 加入时间

    logger(
        "Storage Join, Version: %s, Groupname: %s, Hostname: %s, Port: %u, Start Time: %s, Join Time: %s",
        sj.sj_version,
        sj.sj_groupname,
        sj.sj_hostname,
        sj.sj_port, 
        std::string(ctime(&sj.sj_stime)).c_str(),   // 这种写法是因为 ctime() 会使用一个静态局部变量来保存格式化后时间的字符串
        std::string(ctime(&sj.sj_jtime)).c_str()    // 因为是静态的, 所以两次调用会返回同一块地址, 即后面会将前面覆盖
                                                    // std::string()用于构造一个匿名的 string, 各自构造一个独立的堆空间
                                                    // 再给函数传参时, 会把所有参数计算完后再压入函数
    );  // 打印日志

    // 将存储服务器加入组表
    if (join(&sj, conn->get_peer()) != OK) {
        error(conn, -1, "Join Into Groups Fail...");
        return false;
    }
    return ok(conn);
}

// 处理来自存储服务器的心跳包
bool service_c::beat(acl::socket_stream* conn, long long bodylen) const {
    // | 包体长度 | 命令 | 状态 |  storage_beat_body_t  |
    // |    8    |  1  |  1  |       包 体 长 度       |
    // 检查包体长度
    long long expected = sizeof(storage_beat_body_t);
    if (bodylen != expected) {
        error(conn, -1, "Invalid Body Length: %lld != %lld", bodylen, expected);
        return false;
    }

    // 接收包体
    char body[bodylen];
    if (conn->read(body, bodylen) < 0) {
        logger_error("Read Fail: %s, Bodylen: %lld, From: %s", acl::last_serror(), bodylen, conn->get_peer());
        return false;
    }

    // 解析包体 -> 得到组名和主机名
    storage_beat_body_t* sbb = (storage_beat_body_t*)body;
    char groupname[STORAGE_GROUPNAME_MAX + 1];  // 组名
    char hostname[STORAGE_HOSTNAME_MAX + 1];    // 主机名
    strcpy(groupname, sbb->sbb_groupname);
    strcpy(hostname, sbb->sbb_hostname);

    logger("Storage Beat, Groupname: %s, Hostname: %s", groupname, hostname);  // 打印日志

    // 将存储服务器标记为活动
    if (beat(groupname, hostname, conn->get_peer()) != OK) {
        error(conn, -1, "Mark Storage As ACTIVE Fail...");
        return false;
    }
    return ok(conn);
}

// 处理来自客户机的获取存储服务器地址列表请求
bool service_c::saddrs(acl::socket_stream* conn, long long bodylen) const {
    // | 包体长度 | 命令 | 状态 | 应用ID | 用户ID | 文件ID |
    // |    8    |  1  |  1  |   16   |  256  |  128  |
    // 可以使用这三个ID来实现访问控制: 根据用户的身份、购买的应用以及要访问的文件等完成访问
    
    // 检查包体长度
    long long expected = APPID_SIZE + USERID_SIZE + FILEID_SIZE;
    if (bodylen != expected) {
        error(conn, -1, "Invalid Body Length: %lld != %lld", bodylen, expected);
        return false;
    }

    // 接收包体
    char body[bodylen];
    if (conn->read(body, bodylen) < 0) {
        logger_error("Read Fail: %s, Bodylen: %lld, From: %s", acl::last_serror(), bodylen, conn->get_peer());
        return false;
    }

    // 解析包体: 此处和结构体解析方式不同, 需要通过计算初始位置来完成解析
    char appid[APPID_SIZE];     // 应用ID
    char userid[USERID_SIZE];   // 用户ID
    char fileid[FILEID_SIZE];   // 文件ID
    strcpy(appid, body);
    strcpy(userid, body + APPID_SIZE);
    strcpy(fileid, body + APPID_SIZE + FILEID_SIZE);

    // 响应客户机存储服务器地址列表
    if (saddrs(conn, appid, userid) != OK) {
        return false;
    }
}

// 处理来自客户机的获取组列表请求
bool service_c::groups(acl::socket_stream* conn) const {
    // 互斥锁加锁: 防止组表在访问的过程中出现冲突, 得到不可预知的结果
    if ((errno = pthread_mutex_lock(&g_mutex))) { // errno 为 0 时表示正常返回
        logger_error("Call `pthread_mutex_lock` Fail: %s", strerror(errno));
        return false;
    } 
    
    // 全组字符串
    acl::string gps;        // 定义一个全组字符串, 并获取组的大小
    gps.format("COUNT OF GROUPS: %lu\n", g_groups.size());

    // 遍历组表中的每一个组
    for (std::map<std::string, std::list<storage_info_t>>::const_iterator group = g_groups.begin(); group != g_groups.end(); ++group) {
        // 获取单组字符串: 如 `COUNT OF STORAGES`、`COUNT OF ACTIVE STORAGES`
        acl::string grp;    // 定义一个单组字符串
        grp.format("GROUPNAME: %s\n"
                   "COUNT OF STORAGES: %lu\n"
                   "COUNT OF ACTIVE STORAGES: %s\n", 
                   // COUNT ACTIVE STORAGES 是活动的存储服务器数, 但是暂时不知道具体数量, 使用 "%d" 占位
                   // 代码段中的 `first` 或 `second` 用法, 是因为 map 中的每个元素都对应一组 <key, value> 键值对(pair)
                   // 键值对中的第一个成员称为 `first`, 第二个成员称为 `second`
                   group->first.c_str(), group->second.size(), "%d");

        int act = 0;    // 活动存储服务器数
        // 遍历该组每一台存储服务器
        for (std::list<storage_info_t>::const_iterator si = group->second.begin(); si != group->second.end(); ++si) {
            // 存储服务器字符串
            acl::string stg;
            stg.format("VERSION: %s\n"      // 版本
                       "HOSTNAME: %s\n"     // 主机名
                       "ADDRESS: %s:%u\n"   // IP地址:端口号
                       "STARTUP TIME: %s"   // 服务器启动时间
                       "JOIN TIME: %s"      // 服务器加入时间
                       "BEAT TIME: %s"      // 服务器心跳时间
                       "STATUS: ",          // 服务器状态, 暂时不赋值, 等待转换完毕后进行拼接
                       si->si_version, 
                       si->si_hostname, 
                       si->si_addr, si->si_port,
                       std::string(ctime(&si->si_stime)).c_str(),
                       std::string(ctime(&si->si_jtime)).c_str(),
                       std::string(ctime(&si->si_btime)).c_str());
            switch (si->si_status) {
            case STORAGE_STATUS_OFFLINE:
                stg += "OFFLINE";
                break;
            
            case STORAGE_STATUS_ONLINE:
                stg += "ONLINE";
                break;

            case STORAGE_STATUS_ACTIVE:
                stg += "ACTIVE";
                ++act;  // 每发现一个活动服务器就将 act 计数器 +1
                break;

            default:
                stg += "UNKNOWN";
                break;
            }
            grp += stg + "\n";  // 存储服务器字符串
        }
        gps += grp.format(grp, act);    // grp.format(grp, act); 表示用 grp 来格式化 grp, 因为还有一个 "%d"占位, 此处就用 act 将 "%d" 替换
                                        // grp 的返回还是 grp, 然后将 grp 拼接到 gps 上
    }
    gps = gps.left(gps.size() - 1);     // 删除多出的一个 '\n' 字符, 防止多换行

    // 互斥锁解锁
    if ((errno = pthread_mutex_unlock(&g_mutex))) {
        logger_error("Call `pthread_mutex_unlock` Fail: %s", strerror(errno));
        return false;
    }

    // 构造响应
    long long bodylen = gps.size() + 1;
    long long resplen = HEADLEN + bodylen;
    char resp[resplen] = {};
    llton(bodylen, resp);
    resp[BODYLEN_SIZE] = CMD_TRACKER_REPLY;
    resp[BODYLEN_SIZE + COMMAND_SIZE] = 0;
    strcpy(resp + HEADLEN, gps.c_str());

    // 发送响应
    if (conn->write(resp, resplen) < 0) {
        logger_error("Write Fail: %s, Respone Length: %lld, To: %s", acl::last_serror(), resplen, conn->get_peer());
    }
}

/* 三级方法 */
// 将存储服务器加入组表
int service_c::join(const storage_join_t* sj, const char* saddr) const {
    // 互斥锁加锁
    
    // 在组表中查找待加入存储服务器所隶属的组
    return OK;
}

// 将存储服务器标记为活动
int service_c::beat(const char* groupname, const char* hostname, const char* saddr) const {

    return OK;
}

// 响应客户机存储服务器地址列表
int service_c::saddrs(acl::socket_stream* conn, const char* appid, const char* userid) const {

    return OK;
}

// 根据用户ID获取其对应组名
int service_c::group_of_user(const char* appid, const char* userid, std::string& groupname) const {

    return OK;
}

// 根据组名获取存储服务器地址列表
int service_c::saddr_of_group(const char* groupname, std::string& saddrs) const {

    return OK;
}

// 应答成功
bool service_c::ok(acl::socket_stream* conn) const {

    return OK;
}

// 应答错误
bool service_c::error(acl::socket_stream* conn, short errnumb, const char* format, ...) const {

    return OK;
}
