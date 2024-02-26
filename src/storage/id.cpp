// 存储服务器
// 定义 ID 客户机类
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include "proto.h"
#include "utils.h"
#include "globals.h"
#include "id.h"

// 从 ID服务器获取与 ID键相对应的值
long id_c::get(const char* key) const {
    // 检查键
    // ID_KEY_MAX
    if (!key) {
        logger_error("Key is NULL...");
        return -1;
    }
    size_t keylen =  strlen(key);
    if (!keylen) {
        logger_error("Key is NULL...");
        return -1;
    }

    if (keylen > ID_KEY_MAX) {
        logger_error("Key Too Long: %lu > %d", keylen, ID_KEY_MAX);
        return -1;
    }

    // | 包体长度 | 命令 | 状态 | ID  键 |
    // |    8   |   1  |  1  | 包体长度|
    // 构造请求
    long long bodylen = keylen + 1;
    long long requlen = HEADLEN + bodylen;
    char requ[requlen] = {};
    llton(bodylen, requ);
    requ[BODYLEN_SIZE] = CMD_ID_GET;
    requ[BODYLEN_SIZE + COMMAND_SIZE] = 0;
    strcpy(requ + HEADLEN, key);

    // 向 ID 服务器发送请求, 接收并解析响应, 从中获取 ID 值
    return client(requ, requlen);
}

// 向 ID服务器发送请求, 接收并解析响应, 从中获取ID值
long id_c::client(const char* requ, long long requlen) const {
    acl::socket_stream conn;
    
    // 从 ID 服务器地址列表中随机抽取一台ID服务器尝试连接
    srand(time(NULL));
    int nids = g_iaddrs.size();
    int nrand = rand() % nids;
    for (int i = 0; i < nids; ++i) {
        if (conn.open(g_iaddrs[nrand].c_str(), 0, 0)) {
            logger("Connect ID SUCCESS, Addr: %s", g_iaddrs[nrand].c_str());
            break;
        } else {
            logger("Connect ID FAIL, Addr: %s", g_iaddrs[nrand].c_str());
            nrand = (nrand + 1) % nids;
        }
    }
    // 从 for 循环中退出后, 发现 conn 依旧处于无连接状态, 说明所有连接均不可用, 打印所有地址
    if (!conn.alive()) {
        logger_error("Connect ID Fail, Addrs: %s", cfg_iaddrs);
        return -1;
    }

    // 向ID服务器发送请求
    if (conn.write(requ, requlen) < 0) {
        logger_error("Write Fail: %s, Requlen: %lld, To: %s", acl::last_serror(), requlen, conn.get_peer());
        conn.close();
        return -1;
    }

    // 从ID服务器接收响应
    long long resplen = HEADLEN + BODYLEN_SIZE;
    char resp[resplen] = {};
    if (conn.read(resp, resplen) < 0) {
        logger_error("Read Fail: %s, Requlen: %lld, FROM: %s", acl::last_serror(), requlen, conn.get_peer());
        conn.close();
        return -1;
    }

    // | 包体长度 | 命令 | 状态 | ID  值 |
    // |    8   |   1  |  1  | 包体长度|
    // 从 ID服务器的响应中解析ID值
    long value = ntol(resp + HEADLEN);
    conn.close();
    return value;
}
