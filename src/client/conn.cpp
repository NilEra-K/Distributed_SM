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
    logger("[Client] m_destaddr: %s", m_destaddr);
}

// 从跟踪服务器获取存储服务器地址列表
int conn_c::saddrs(const char* appid, const char* userid, const char* fileid, std::string& saddrs) {
    // 构造请求
    // | 包体长度 | 命令 | 状态 | 应用ID | 用户ID | 文件ID |
    // |    8    |  1  |  1  |  16+1  |  256  |  128  |
    long long bodylen = APPID_SIZE + USERID_SIZE + FILEID_SIZE;
    long long requlen = HEADLEN + bodylen;
    char requ[requlen];
    if (makerequ(CMD_TRACKER_SADDRS, appid, userid, fileid, requ) != OK) {
        return ERROR;
    }
    llton(bodylen, requ);

    if (!open()) {
        return SOCKET_ERROR;
    }

    // 发送请求
    if (m_conn->write(requ, requlen) < 0) {
        logger_error("Write Fail: %s, Requlen: %lld, TO: %s", acl::last_serror(), requlen, m_conn->get_peer());
        m_errnumb = -1;
        m_errdesc.format("Write Fail: %s, Requlen: %lld, TO: %s", acl::last_serror(), requlen, m_conn->get_peer());
        close();
        return SOCKET_ERROR;
    }

    // 包体指针
    char* body = NULL;

    // 接收包体
    int result = recvbody(&body, &bodylen);

    // 解析包体
    if (result == OK) {
        // 成功
        // | 包体长度 | 命令 | 状态 | 组名 | 存储服务器地址列表 |
        // |    8    |  1  |  1  | 16+1 | 包体长度-(16+1)  |
        saddrs = body + STORAGE_GROUPNAME_MAX + 1;
    } else if (result == STATUS_ERROR) {
        // 失败
        // | 包体长度 | 命令 | 状态 | 错误号 | 错误描述 |
        // |    8    |  1  |  1  |   2   |  <=1024 |
        m_errnumb = ntos(body);
        m_errdesc = bodylen > ERROR_NUMB_SIZE ? (body + ERROR_NUMB_SIZE) : ""; 
    }

    // 释放包体
    if (body) {
        free(body);
        body = NULL;
    }

    return result;
}

// 从跟踪服务器获取组列表
int conn_c::groups(std::string& groups) {
    // 构造请求
    // | 包体长度 | 命令 | 状态 |
    // |    8    |  1  |  1  |
    long long bodylen = 0;
    long long requlen = HEADLEN + bodylen;
    char requ[requlen] = {};
    llton(bodylen, requ);
    requ[BODYLEN_SIZE] = CMD_TRACKER_GROUPS;
    requ[BODYLEN_SIZE + COMMAND_SIZE] = 0;

    if (!open()) {
        return SOCKET_ERROR;
    }

    // 发送请求
    if (m_conn->write(requ, requlen) < 0) {
        logger_error("Write Fail: %s, Requlen: %lld, TO: %s", acl::last_serror(), requlen, m_conn->get_peer());
        m_errnumb = -1;
        m_errdesc.format("Write Fail: %s, Requlen: %lld, TO: %s", acl::last_serror(), requlen, m_conn->get_peer());
        close();
        return SOCKET_ERROR;
    }

    // 包体指针
    char* body = NULL;

    // 接收包体
    int result = recvbody(&body, &bodylen);
    // logger("[Client groups(...)] %s", body);

    // 解析包体
    if (result == OK) {
        // 成功
        // | 包体长度 | 命令 | 状态 |  组列表 |
        // |    8    |  1  |  1  | 包体长度 |
        groups = body;
        // logger("[Client] %s", body);
    } else if (result == STATUS_ERROR) {
        // 失败
        // | 包体长度 | 命令 | 状态 | 错误号 | 错误描述 |
        // |    8    |  1  |  1  |   2   |  <=1024 |
        m_errnumb = ntos(body);
        m_errdesc = bodylen > ERROR_NUMB_SIZE ? (body + ERROR_NUMB_SIZE) : ""; 
    }

    // 释放包体
    if (body) {
        free(body);
        body = NULL;
    }
    logger("[Client] Result: %d\n", result);

    return result;
}

// 向存储服务器上传文件(文件格式)
int conn_c::upload(const char* appid, const char* userid, const char* fileid, const char* filepath) {
    // 构造请求
    // | 包体长度 | 命令 | 状态 | 应用ID | 用户ID | 文件ID | 文件大小 | 文件内容 |
    // |    8    |  1  |  1  |  16+1  |  256  |  128  |    8    | 文件大小 |
    long long bodylen = APPID_SIZE + USERID_SIZE + FILEID_SIZE + BODYLEN_SIZE;
    long long requlen = HEADLEN + bodylen;
    char requ[requlen];
    if (makerequ(CMD_STORAGE_UPLOAD, appid, userid, fileid, requ) != OK) {
        return ERROR;
    }
    acl::ifstream ifs;
    if (!ifs.open_read(filepath)) {
        logger_error("Open File Fail, Filepath: %s", filepath);
        return ERROR;
    }
    long long filesize = ifs.fsize();
    llton(filesize, requ + HEADLEN + APPID_SIZE + USERID_SIZE + FILEID_SIZE);
    bodylen += filesize; 
    llton(bodylen, requ);

    if (!open()) {
        return SOCKET_ERROR;
    }

    // 发送请求(暂时没有发送文件内容)
    if (m_conn->write(requ, requlen) < 0) {
        logger_error("Write Fail: %s, Requlen: %lld, TO: %s", acl::last_serror(), requlen, m_conn->get_peer());
        m_errnumb = -1;
        m_errdesc.format("Write Fail: %s, Requlen: %lld, TO: %s", acl::last_serror(), requlen, m_conn->get_peer());
        close();
        return SOCKET_ERROR;
    }

    // 发送文件
    long long remain = filesize;    // 未发送字节数
    off_t offset = 0;               // 文件读写位置
    while (remain) { // 还有未发送数据
        // 发送数据
        long long bytes = std::min(remain, (long long)STORAGE_RCVWR_SIZE);

        // sendfile(int __out_fd, int __in_fd, off_t *__offset, size_t __count);
        // 表示从 __in_fd 向 __out_fd , 从 offset 位置, 发送 __count 个字节的数据, 传入 offset 的地址, 表示可以修改 offset 的值
        // m_conn->sock_handle() 获取套接字句柄
        long long count = sendfile(m_conn->sock_handle(), ifs.file_handle(), &offset, bytes);
        if (count < 0) {
            logger_error("Send File Fail, Filesize: %lld, Remain: %lld", filesize, remain);
            m_errnumb = -1;
            m_errdesc.format("Send File Fail, Filesize: %lld, Remain: %lld", filesize, remain);
            close();
            return SOCKET_ERROR;
        }
        // 未发递减
        remain -= count;
    }
    ifs.close();

    // 包体指针
    char* body = NULL;

    // 接收包体
    int result = recvbody(&body, &bodylen);

    // 解析包体
    if (result == STATUS_ERROR) {
        // 失败
        // | 包体长度 | 命令 | 状态 | 错误号 | 错误描述 |
        // |    8    |  1  |  1  |   2   |  <=1024 |
        m_errnumb = ntos(body);
        m_errdesc = bodylen > ERROR_NUMB_SIZE ? (body + ERROR_NUMB_SIZE) : ""; 
    }

    // 释放包体
    if (body) {
        free(body);
        body = NULL;
    }
    logger("[Client] Result: %d", result);

    return result;
}

// 向存储服务器上传文件(数据格式)
int conn_c::upload(const char* appid, const char* userid, const char* fileid, const char* filedata, long long filesize) {
    // 构造请求
    // | 包体长度 | 命令 | 状态 | 应用ID | 用户ID | 文件ID | 文件大小 | 文件内容 |
    // |    8    |  1  |  1  |  16+1  |  256  |  128  |    8    | 文件大小 |
    long long bodylen = APPID_SIZE + USERID_SIZE + FILEID_SIZE + BODYLEN_SIZE;
    long long requlen = HEADLEN + bodylen;
    char requ[requlen];
    if (makerequ(CMD_STORAGE_UPLOAD, appid, userid, fileid, requ) != OK) {
        return ERROR;
    }
    llton(filesize, requ + HEADLEN + APPID_SIZE + USERID_SIZE + FILEID_SIZE);
    bodylen += filesize; 
    llton(bodylen, requ);

    if (!open()) {
        return SOCKET_ERROR;
    }

    // 发送请求(暂时没有发送文件内容)
    if (m_conn->write(requ, requlen) < 0) {
        logger_error("Write Fail: %s, Requlen: %lld, TO: %s", acl::last_serror(), requlen, m_conn->get_peer());
        m_errnumb = -1;
        m_errdesc.format("Write Fail: %s, Requlen: %lld, TO: %s", acl::last_serror(), requlen, m_conn->get_peer());
        close();
        return SOCKET_ERROR;
    }

    // 发送文件
    if (m_conn->write(filedata, filesize) < 0) {
        logger_error("Write Fail: %s, Filesize: %lld, TO: %s", acl::last_serror(), filesize, m_conn->get_peer());
        m_errnumb = -1;
        m_errdesc.format("Write Fail: %s, Filesize: %lld, TO: %s", acl::last_serror(), filesize, m_conn->get_peer());
        close();
        return SOCKET_ERROR;
    }

    // 包体指针
    char* body = NULL;

    // 接收包体
    int result = recvbody(&body, &bodylen);

    // 解析包体
    if (result == STATUS_ERROR) {
        // 失败
        // | 包体长度 | 命令 | 状态 | 错误号 | 错误描述 |
        // |    8    |  1  |  1  |   2   |  <=1024 |
        m_errnumb = ntos(body);
        m_errdesc = bodylen > ERROR_NUMB_SIZE ? (body + ERROR_NUMB_SIZE) : ""; 
    }

    // 释放包体
    if (body) {
        free(body);
        body = NULL;
    }
    return result;
}

// 向存储服务器询问文件大小
int conn_c::filesize(const char* appid, const char* userid, const char* fileid, long long* filesize) {
    // 构造请求
    // | 包体长度 | 命令 | 状态 | 应用ID | 用户ID | 文件ID |
    // |    8    |  1  |  1  |  16+1  |  256  |  128  |
    long long bodylen = APPID_SIZE + USERID_SIZE + FILEID_SIZE;
    long long requlen = HEADLEN + bodylen;
    char requ[requlen];
    if (makerequ(CMD_STORAGE_FILESIZE, appid, userid, fileid, requ) != OK) {
        return ERROR;
    }
    llton(bodylen, requ);

    if (!open()) {
        return SOCKET_ERROR;
    }

    // 发送请求
    if (m_conn->write(requ, requlen) < 0) {
        logger_error("Write Fail: %s, Requlen: %lld, TO: %s", acl::last_serror(), requlen, m_conn->get_peer());
        m_errnumb = -1;
        m_errdesc.format("Write Fail: %s, Requlen: %lld, TO: %s", acl::last_serror(), requlen, m_conn->get_peer());
        close();
        return SOCKET_ERROR;
    }

    // 包体指针
    char* body = NULL;

    // 接收包体
    int result = recvbody(&body, &bodylen);

    // 解析包体
    if (result == OK) {
        // 成功
        // | 包体长度 | 命令 | 状态 | 文件大小 |
        // |    8    |  1  |  1  |    8    |
        *filesize = ntoll(body);
    } else if (result == STATUS_ERROR) {
        // 失败
        // | 包体长度 | 命令 | 状态 | 错误号 | 错误描述 |
        // |    8    |  1  |  1  |   2   |  <=1024 |
        m_errnumb = ntos(body);
        m_errdesc = bodylen > ERROR_NUMB_SIZE ? (body + ERROR_NUMB_SIZE) : ""; 
    }

    // 释放包体
    if (body) {
        free(body);
        body = NULL;
    }

    return result;
}

// 从存储服务器下载文件
int conn_c::download(const char* appid, const char* userid, const char* fileid, long long offset, long long size, char** filedata, long long* filesize) {
    // 构造请求
    // | 包体长度 | 命令 | 状态 | 应用ID | 用户ID | 文件ID | 偏移 | 大小 | 
    // |    8    |  1  |  1  |  16+1  |  256  |  128  |   8  |  8  |
    long long bodylen = APPID_SIZE + USERID_SIZE + FILEID_SIZE + BODYLEN_SIZE + BODYLEN_SIZE;
    long long requlen = HEADLEN + bodylen;
    char requ[requlen];
    if (makerequ(CMD_STORAGE_DOWNLOAD, appid, userid, fileid, requ) != OK) {
        return ERROR;
    }
    llton(bodylen, requ);
    llton(offset, requ + HEADLEN + APPID_SIZE + USERID_SIZE + FILEID_SIZE);
    llton(size, requ + HEADLEN + APPID_SIZE + USERID_SIZE + FILEID_SIZE + BODYLEN_SIZE);

    if (!open()) {
        return SOCKET_ERROR;
    }

    // 发送请求
    if (m_conn->write(requ, requlen) < 0) {
        logger_error("Write Fail: %s, Requlen: %lld, TO: %s", acl::last_serror(), requlen, m_conn->get_peer());
        m_errnumb = -1;
        m_errdesc.format("Write Fail: %s, Requlen: %lld, TO: %s", acl::last_serror(), requlen, m_conn->get_peer());
        close();
        return SOCKET_ERROR;
    }

    // 包体指针
    char* body = NULL;

    // 接收包体
    int result = recvbody(&body, &bodylen);

    // 解析包体
    if (result == OK) {
        // 成功
        // | 包体长度 | 命令 | 状态 | 文件内容 |
        // |    8    |  1  |  1  | 内容大小 |
        *filedata = body;
        *filesize = bodylen;
        return result;  // 直接返回, 不经过释放包体部分, 不能释放 body 内存, 因为调用者还需要使用
    } 
    
    if (result == STATUS_ERROR) {
        // 失败
        // | 包体长度 | 命令 | 状态 | 错误号 | 错误描述 |
        // |    8    |  1  |  1  |   2   |  <=1024 |
        m_errnumb = ntos(body);
        m_errdesc = bodylen > ERROR_NUMB_SIZE ? (body + ERROR_NUMB_SIZE) : ""; 
    }

    // 释放包体
    if (body) {
        free(body);
        body = NULL;
    }

    return result;
}

// 删除存储服务器上的文件
int conn_c::del(const char* appid, const char* userid, const char* fileid) {
        // 构造请求
    // | 包体长度 | 命令 | 状态 | 应用ID | 用户ID | 文件ID |
    // |    8    |  1  |  1  |  16+1  |  256  |  128  |
    long long bodylen = APPID_SIZE + USERID_SIZE + FILEID_SIZE;
    long long requlen = HEADLEN + bodylen;
    char requ[requlen];
    if (makerequ(CMD_STORAGE_DELETE, appid, userid, fileid, requ) != OK) {
        return ERROR;
    }
    llton(bodylen, requ);

    if (!open()) {
        return SOCKET_ERROR;
    }

    // 发送请求
    if (m_conn->write(requ, requlen) < 0) {
        logger_error("Write Fail: %s, Requlen: %lld, TO: %s", acl::last_serror(), requlen, m_conn->get_peer());
        m_errnumb = -1;
        m_errdesc.format("Write Fail: %s, Requlen: %lld, TO: %s", acl::last_serror(), requlen, m_conn->get_peer());
        close();
        return SOCKET_ERROR;
    }

    // 包体指针
    char* body = NULL;

    // 接收包体
    int result = recvbody(&body, &bodylen);

    // 解析包体
    if (result == STATUS_ERROR) {
        // 失败
        // | 包体长度 | 命令 | 状态 | 错误号 | 错误描述 |
        // |    8    |  1  |  1  |   2   |  <=1024 |
        m_errnumb = ntos(body);
        m_errdesc = bodylen > ERROR_NUMB_SIZE ? (body + ERROR_NUMB_SIZE) : ""; 
    }

    // 释放包体
    if (body) {
        free(body);
        body = NULL;
    }

    return result;
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
    logger("[Client] Start Open Socket...\n");
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
    // 接收包头
    int result = recvhead(bodylen);

    // 即非本地错误, 亦非套接字错误, 且包体非空
    if (result != ERROR && result != SOCKET_ERROR && *bodylen) {
        // 分配包体
        if (!(*body = (char*)malloc(*bodylen))) {
            logger_error("Call `malloc()` Fail: %s, Bodylen: %lld", strerror(errno), *bodylen);
            m_errnumb = -1;
            m_errdesc.format("Call `malloc()` Fail: %s, Bodylen: %lld", strerror(errno), *bodylen);
            return ERROR;
        }

        // 接收包体
        if (m_conn->read(*body, *bodylen) < 0) {
            logger_error("Read Fail: %s, FROM: %s", acl::last_serror(), m_conn->get_peer());
            m_errnumb = -1;
            m_errdesc.format("Read Fail: %s, FROM: %s", acl::last_serror(), m_conn->get_peer());
            free(*body);
            *body = NULL;
            close();
            return SOCKET_ERROR;
        }
        logger("[Client] `m_conn->read()` OK");
        logger("%s", *body);
    }

    return result;
}

// 接收包头
int conn_c::recvhead(long long* bodylen) {
    // 验证连接有效
    if (!open()) {
        return SOCKET_ERROR;
    }
    
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

    int command = head[BODYLEN_SIZE];                   // 命令
    int status  = head[BODYLEN_SIZE + COMMAND_SIZE];    // 状态
    if (status) { // 如果 status 为 0 表示正常返回, 非 0 表示出现问题
        logger_error("Response Status %d != 0, FROM: %s", status, m_conn->get_peer());
        return STATUS_ERROR;
    }
    logger("Bodylen: %lld, Command: %d, Status: %d", *bodylen, command, status);
    return OK;
}
