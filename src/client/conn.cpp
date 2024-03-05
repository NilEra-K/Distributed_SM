// 客户机模块
// 实现连接类

#include <sys/sendfile.h>
#include <acl-lib/acl/lib_acl.h>    // 可以选择 C 语言的库
#include "proto.h"
#include "utils.h"
#include "conn.h"

// 构造函数
conn_c::conn_c(const char* destaddr, int ctimeout /* = 30 */, int rtimeout /* = 60 */) :m_ctimeout(ctimeout), m_rtimeout(rtimeout), m_conn(NULL) {
    // 检查目的地址
    // acl_assert(...) 断言, 当传入参数为真时不进行操作, 否则杀死程序
    acl_assert(destaddr && *destaddr);
    // 复制目的地址
    m_destaddr = acl_mystrdup(destaddr);
}

// 从跟踪服务器获取存储服务器地址列表
int conn_c::saddrs(const char* appid, const char* userid, const char* fileid, std::string& saddrs) {

    return OK;
}

// 从跟踪服务器获取组列表
int conn_c::groups(std::string& groups) {

    return OK;
}

// 向存储服务器上传文件(文件格式)
int conn_c::upload(const char* appid, const char* userid, const char* fileid, const char* filepath) {

    return OK;
}

// 向存储服务器上传文件(数据格式)
int conn_c::upload(const char* appid, const char* userid, const char* fileid, const char* filedata, long long filesize) {

    return OK;
}

// 向存储服务器询问文件大小
int conn_c::filesize(const char* appid, const char* userid, const char* fileid, long long* filesize) {

    return OK;
}

// 从存储服务器下载文件
int conn_c::download(const char* appid, const char* userid, const char* fileid, long long offset, long long size, char** filedata, long long* filesize) {

    return OK;
}

// 删除存储服务器上的文件
int conn_c::del(const char* appid, const char* userid, const char* fileid) {

    return OK;
}

// 获取错误号
short conn_c::errnumb(void) const {
    return m_errnumb;
}

// 获取错误描述
const char* conn_c::errdesc(void) const {
    return m_errdesc.c_str();
}

// 析构函数
conn_c::~conn_c(void) {
    // 关闭连接
    close();

    // 释放目的地址: 与构造函数中的 acl_mystrdup(destaddr); 对应
    acl_myfree(m_destaddr);
}

// 打开连接
bool conn_c::open(void) {
    if (m_conn) { // 防止出现多次调用 open() 函数导致内存泄漏
        return true;
    }

    // 创建连接对象
    m_conn = new acl::socket_stream;

    // 连接目的主机
    if (!m_conn->open(m_destaddr, m_ctimeout, m_rtimeout)) {
        logger_error("Open %s Fail: %s", m_destaddr, acl_last_serror());
        delete m_conn;
        m_conn = NULL;
        m_errnumb = -1;
        m_errdesc.format("Open %s Fail: %s", m_destaddr, acl_last_serror());
        return false;
    }

    return true;
}

// 关闭连接
void conn_c::close(void) {
    if (m_conn) {
        delete m_conn;
        m_conn = NULL;
    }
}

// 构造请求
int conn_c::makerequ(char command, const char* appid, const char* userid, const char* fileid, char* requ) {
    // | 包体长度 | 命令 | 状态 | 应用ID | 用户ID | 文件ID |
    // |    8    |  1  |  1  |   16   |  256  |  128  |
    requ[BODYLEN_SIZE] = command;           // 命令
    requ[BODYLEN_SIZE + COMMAND_SIZE] = 0;  // 状态

    // 应用ID
    if (strlen(appid) >= APPID_SIZE) {
        logger_error("Appid Too Big, %lu >= %d", strlen(appid), APPID_SIZE);
        m_errnumb = -1;
        m_errdesc.format("Appid Too Big, %lu >= %d", strlen(appid), APPID_SIZE);
        return ERROR;
    }
    strcpy(requ + HEADLEN, appid);

    // 用户ID
    if (strlen(userid) >= USERID_SIZE) {
        logger_error("Userid Too Big, %lu >= %d", strlen(userid), USERID_SIZE);
        m_errnumb = -1;
        m_errdesc.format("Userid Too Big, %lu >= %d", strlen(userid), USERID_SIZE);
        return ERROR;
    }
    strcpy(requ + HEADLEN + APPID_SIZE, userid);

    // 文件ID
    if (strlen(fileid) >= FILEID_SIZE) {
        logger_error("Fileid Too Big, %lu >= %d", strlen(fileid), FILEID_SIZE);
        m_errnumb = -1;
        m_errdesc.format("Fileid Too Big, %lu >= %d", strlen(fileid), FILEID_SIZE);
        return ERROR;
    }
    strcpy(requ + HEADLEN + APPID_SIZE + USERID_SIZE, fileid);
    return OK;
}

// 接收包体
int conn_c::recvbody(char** body, long long* bodylen) {


    return OK;
}

// 接收包头
int conn_c::recvhead(long long* bodylen) {
    // 验证连接有效
    
    // 包头缓冲区
    char head[HEADLEN];    

    // 接收包头
    if (m_conn->read(head, HEADLEN) < 0) {
        logger_error("Read Fail: %s, FROM: %s", acl::last_serror(), m_conn->get_peer());
        m_errnumb = -1;
        m_errdesc.format("Read Fail: %s, FROM: %s", acl::last_serror(), m_conn->get_peer());
        close();
        return SOCKET_ERROR;
    }

    // 解析包头
    // | 包体长度 | 命令 | 状态 |
    // |    8    |  1  |  1  |
    if ((*bodylen = ntoll(head)) < 0) { // 包体长度
        logger_error("Invalid Body Length: %lld < 0, FROM: %s", *bodylen, m_conn->get_peer());
        m_errnumb = -1;
        m_errdesc.format("Invalid Body Length: %lld < 0, FROM: %s", *bodylen, m_conn->get_peer());
        return ERROR;
    }

    // 命令
    // 状态
    return OK;
}

