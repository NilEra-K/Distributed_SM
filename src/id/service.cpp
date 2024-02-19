// ID服务器
// 实现业务服务类
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
    case CMD_ID_GET:            // 处理来自存储服务器的获取ID请求
        result = get(conn, bodylen);
        break;

    default:
        error(conn, -1, "Unknown Command: %d", command);
        return false;
    }
    return result;
}

/* 二级方法 */
// 处理来自存储服务器的获取ID请求
bool service_c::get(acl::socket_stream* conn, long long bodylen) const {
    // | 包体长度 | 命令 | 状态 | ID键 |
    // |    8    |  1  |  1  | 64+1 |   // ID键定义最大为64, 加一个空字符为 65
    // 检查包体长度
    long long expected = ID_KEY_MAX + 1;
    if (bodylen > expected) {
        error(conn, -1, "Invalid Body Length: %lld > %lld", bodylen, expected);
        return false;
    }

    // 接收包体
    char body[bodylen];
    if (conn->read(body, bodylen) < 0) {
        logger_error("Read Fail: %s, Bodylen: %lld, From: %s", acl::last_serror(), bodylen, conn->get_peer());
        return false;
    }

    // 根据ID的键获取其值
    long value = get(body);     // body 中只有 ID键
    if (value < 0) {
        error(conn, -1, "GET ID Fail, Key: %s", body);
        return false;
    }
    
    // 应答ID
    logger("GET ID OK, Key: %s, Value: %ld", body, value);
    return id(conn, value);
}

/* 三级方法 */
// 根据ID的键获取其值
long service_c::get(const char* key) const {
    // 互斥锁加锁
    if ((errno = pthread_mutex_lock(&g_mutex))) {
        logger_error("Call `pthread_mutex_lock` Fail: %s", strerror(errno));
        return -1;
    }

    long value = -1;

    // 在ID表中查找ID
    std::vector<id_pair_t>::iterator id;
    for (id = g_ids.begin(); id != g_ids.end(); ++id) {
        if (!strcmp(id->id_key, key)) {
            break;
        }
    }
    
    if (id != g_ids.end()) { // 找到该ID
        if (id->id_offset < cfg_maxoffset) { // 该ID的偏移未达到上限
            // 内存累加
            value = id->id_value + id->id_offset;
            ++id->id_offset;
        } else if ((value = fromdb(key)) >= 0) { // 该ID的偏移达到上限, 从数据库中获取ID值
            // 更新ID表中的ID
            id->id_value = value;
            id->id_offset = 1;
        }
    } else if ((value = fromdb(key)) >= 0) { // 未找到该ID, 从数据库中获取ID值
        // 在ID表中添加ID
        id_pair_t id;
        strcpy(id.id_key, key);
        id.id_value = value;
        id.id_offset = 1;
        g_ids.push_back(id);
    }

    // 互斥锁解锁
    if ((errno = pthread_mutex_unlock(&g_mutex))) {
        logger_error("Call `pthread_mutex_unlock` Fail: %s", strerror(errno));
        return -1;
    }
    return value;
}

// 从数据库中获取ID值
long service_c::fromdb(const char* key) const {
    // 创建数据库访问对象
    db_c db;

    // 连接数据库
    if (db.connect() != OK) {
        return -1;
    }

    long value = -1;

    // 获取ID当前值, 同时产生下一个值
    if (db.get_id(key, cfg_maxoffset, &value) != OK) {
        return -1;
    }
    return value;
}

// 应答成功->应答ID
bool service_c::id(acl::socket_stream* conn, long value) const {
    // | 包体长度 | 命令 | 状态 | ID值 |
    // |    8    |  1  |  1   |  8  |
    // 构造响应
    long long bodylen = BODYLEN_SIZE;
    long long resplen = HEADLEN + bodylen;
    char resp[resplen] = {};
    llton(bodylen, resp);
    resp[BODYLEN_SIZE] = CMD_ID_REPLY;      // 命令
    resp[BODYLEN_SIZE + COMMAND_SIZE] = 0;  // 状态
    llton(value, resp + HEADLEN);           // ID值

    // 发送响应
    if (conn->write(resp, resplen) < 0) {
        logger_error("Write Fail: %s, Resplen: %lld, To: %s", acl::last_serror(), resplen, conn->get_peer());
        return false;
    }
    return true;
}

// 应答错误
bool service_c::error(acl::socket_stream* conn, short errnumb, const char* format, ...) const {
    // 错误描述
    char errdesc[ERROR_DESC_MAX];

    // va -> va_start -> va_end 是使用 `va_list` 必须经历的几个步骤
    va_list ap;
    va_start(ap, format);   // 表示从 `format` 参数后面开始获取变长参数表, 并放到 `ap` 中
                            // 因为 `...` 没有名字, 是无法表示的

    vsnprintf(errdesc, ERROR_DESC_MAX, format, ap); // 使用 `vsnprintf()` 函数
                                                    // errdesc:         表明输出函数的错误描述
                                                    // ERROR_DESC_MAX:  表明输出的最大长度, 这个形参表明其输出最多不能超过 ERROR_DESC_MAX 个字节
    va_end(ap);             // 释放缓冲区

    logger_error("%s", errdesc);
    acl::string desc;
    desc.format("[%s] %s", g_hostname.c_str(), errdesc);
    memset(errdesc, 0, sizeof(errdesc));                // 将已开辟内存空间`errdesc`的首 `sizeof(errdesc)` 个字节的值设置为 `0`
    strncpy(errdesc, desc.c_str(), ERROR_DESC_MAX - 1); // 将 `desc.c_str()` 复制到 `errdesc` 中, 复制长度为 `ERROR_DESC_MAX - 1`
                                                        // 使用该方法复制不包含 `\0` 字符, 为了保证有位置添加'\0', 需要保证有一个位置的空余
    size_t desclen = strlen(errdesc);
    desclen += (desclen != 0);  // 当错误描述为空的时候 `+0`, 表示直接舍弃该部分, 不需要这一部分
                                // 当错误描述非空的时候 `+1`, 表示需要在其后添加一个 `\0` 字符

    // |包体长度|命令|状态|错误号|错误描述|
    // |   8   | 1 | 1 |  2  | <=1024|
    // 构造响应
    long long bodylen = ERROR_NUMB_SIZE + desclen;
    long long resplen = HEADLEN + bodylen;
    char resp[resplen] = {};
    llton(bodylen, resp);
    resp[BODYLEN_SIZE] = CMD_ID_REPLY;
    resp[BODYLEN_SIZE + COMMAND_SIZE] = STATUS_ERROR;
    ston(errnumb, resp + HEADLEN);
    if (desclen) {
        strcpy(resp + HEADLEN + ERROR_NUMB_SIZE, errdesc);
    }

    // 发送响应
    if (conn->write(resp, resplen) < 0) {
        logger_error("Write Fail: %s, Resplen: %lld, To: %s", acl::last_serror(), resplen, conn->get_peer());
        return false;
    }

    return OK;
}
