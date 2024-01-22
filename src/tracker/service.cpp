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

    // 解析包体 -> 得到
    storage_beat_body_t* sbb = (storage_beat_body_t*)body;
    strcpy(sj.sj_groupname, sjb->sjb_groupname);    // 组名
    if (valid(sj.sj_groupname) != OK) {
        error(conn, -1, "Invalid Groupname: %s", sj.sj_groupname);
        return false;
    }

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
        std::string(ctime(&sj.sj_stime)).c_str(),
        std::string(ctime(&sj.sj_jtime)).c_str()
    );  // 打印日志

    // 将存储服务器加入组表
    if (join(&sj, conn->get_peer()) != OK) {
        error(conn, -1, "Join Into Groups Fail...");
        return false;
    }
    return ok(conn);
}

// 处理来自客户机的获取存储服务器地址列表请求
bool service_c::saddrs(acl::socket_stream* conn, long long bodylen) const {

}

// 处理来自客户机的获取组列表请求
bool service_c::groups(acl::socket_stream* conn) const {

}

/* 三级方法 */
// 将存储服务器加入组表
int service_c::join(const storage_join_t* sj, const char* saddr) const {

}

// 将存储服务器标记为活动
int service_c::beat(const char* groupname, const char* hostname, const char* saddr) const {

}

// 响应客户机存储服务器地址列表
int service_c::saddrs(acl::socket_stream* conn, const char* appid, const char* userid) const {

}

// 根据用户ID获取其对应组名
int service_c::group_of_user(const char* appid, const char* userid, std::string& groupname) const {

}

// 根据组名获取存储服务器地址列表
int service_c::saddr_of_group(const char* groupname, std::string& saddrs) const {

}

// 应答成功
bool service_c::ok(acl::socket_stream* conn) const {

}

// 应答错误
bool service_c::error(acl::socket_stream* conn, short errnumb, const char* format, ...) const {

}