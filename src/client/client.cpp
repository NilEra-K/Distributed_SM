// 客户机
// 实现客户机类
#include <acl-lib/acl_cpp/lib_acl.hpp>
#include "types.h"
#include "utils.h"
#include "conn.h"
#include "pool.h"
#include "mngr.h"
#include "client.h"

#define MAX_SOCKERRS    10  // 套接字通信错误最大次数

// 静态变量
acl::connect_manager* client_c::s_mngr = NULL;   // 连接池管理器
std::vector<std::string> client_c::s_taddrs;     // 跟踪服务器地址表
int client_c::s_scount = 8;                      // 存储服务器连接数上限

// 初始化
// -taddrs: 跟踪服务器地址表 -tcount: 跟踪服务器连接数上限 -scount: 存储服务器连接数上限
int client_c::init(const char* taddrs, int tcount /* = 16 */, int scount /* = 8 */) {
    // 只初始化一次: 无法阻止只调用一次, 那就保证其他次调用无用功
    if (s_mngr) { // 以 s_mngr 为标志, 当其非空时, 说明已经调用过该函数, 直接返回 OK 即可
        return OK;
    }

    // 跟踪服务器地址表
    if (!taddrs || !strlen(taddrs)) {
        logger_error("Tracker Addresses is NULL...");
        return ERROR;
    }
    split(taddrs, s_taddrs);
    if (s_taddrs.empty()) {
        logger_error("Tracker Addresses is Empty...");
        return ERROR;
    }

    // 跟踪服务器连接数上限
    if (tcount <= 0) {
        logger_error("Invalid Tracker Connection Pool Count %d <= 0", tcount);
        return ERROR;
    }

    // 存储服务器连接数上限
    if (scount <= 0) {
        logger_error("Invalid Storage Connection Pool Count %d <= 0", scount);
        return ERROR;
    }
    s_scount = scount;

    // 创建连接池管理器类
    if (!(s_mngr = new mngr_c)) {
        logger_error("Create Connection Pool Manager Fail: %s", acl::last_serror());
        return ERROR;
    }

    // 初始化跟踪服务器连接池
    s_mngr->init(NULL, taddrs, tcount);

    return OK;
}

// 终结化
void client_c::deinit(void) {
    if (s_mngr) {
        delete s_mngr;
        s_mngr = NULL;
    }
    s_taddrs.clear();
}

// 从跟踪服务器获取存储服务器地址列表
int client_c::saddrs(const char* appid, const char* userid, const char* fileid, std::string& saddrs) {
    if (s_taddrs.empty()) {
        logger_error("Tracker Addresses is Empty...");
        return ERROR;
    }
    int result = ERROR;

    // 生成有限随机数
    srand(time(NULL));
    int ntaddrs = s_taddrs.size();
    int nrand = rand() % ntaddrs;

    for (int i = 0; i < ntaddrs; ++i) {
        // 随机抽取跟踪服务器地址
        const char* taddr = s_taddrs[nrand].c_str();
        nrand = (nrand + 1) % ntaddrs;

        // 获取跟踪服务器连接池
        pool_c* tpool = (pool_c*)s_mngr->get(taddr);
        if (!tpool) {
            logger_warn("Tracker Connection Pool is NULL, taddr: %s", taddr);
            continue;
        }

        for (int sockerrs = 0; sockerrs < MAX_SOCKERRS; ++sockerrs) {
            // 获取跟踪服务器连接
            conn_c* tconn = (conn_c*)tpool->peek();
            if (!tconn) {
                logger_warn("Tracker Connection is NULL, taddr: %s", taddr);
                break;
            }

            // 从跟踪服务器获取存储服务器地址列表
            result = tconn->saddrs(appid, userid, fileid, saddrs);
            if (result == SOCKET_ERROR) {
                logger_warn("Get Storage Address Fail: %s", tconn->errdesc());
                tpool->put(tconn, false);
            } else {
                if (result == OK) {
                    tpool->put(tconn, true);
                } else {
                    logger_error("Get Storage Address Fail: %s", tconn->errdesc());
                    tpool->put(tconn, false);
                }
                return result;
            }
        }
    }

    return result;
}

// 从跟踪服务器获取组列表
int client_c::groups(std::string& groups) {

    return OK;
}

// 向存储服务器上传文件(文件格式)
int client_c::upload(const char* appid, const char* userid, const char* fileid, const char* filepath) {

    return OK;
}

// 向存储服务器上传文件(数据格式)
int client_c::upload(const char* appid, const char* userid, const char* fileid, const char* filedata, long long filesize) {

    return OK;
}

// 向存储服务器询问文件大小
int client_c::filesize(const char* appid, const char* userid, const char* fileid, long long* filesize) {

    return OK;
}

// 从存储服务器下载文件
int client_c::download(const char* appid, const char* userid, const char* fileid, long long offset, long long size, char** filedata, long long* filesize) {
    return OK;
}

// 删除存储服务器上的文件
int client_c::del(const char* appid, const char* userid, const char* fileid) {

    return OK;
}
