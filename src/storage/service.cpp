// 存储服务器
// 实现业务服务类
#include <linux/limits.h>
#include <algorithm>
#include "proto.h"
#include "utils.h"
#include "globals.h"
#include "db.h"
#include "file.h"
#include "id.h"
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
    case CMD_STORAGE_UPLOAD:    // 处理来自客户机的上传文件请求
        result = upload(conn, bodylen);
        break;

    case CMD_STORAGE_FILESIZE:  // 处理来自客户机的询问文件大小请求
        result = filesize(conn, bodylen);
        break;

    case CMD_STORAGE_DOWNLOAD:  // 处理来自客户机的下载文件请求
        result = download(conn, bodylen);
        break;

    case CMD_STORAGE_DELETE:    // 处理来自客户机的删除文件请求
        result = del(conn, bodylen);
        break;
    default:
        error(conn, -1, "Unknown Command: %d", command);
        return false;
    }
    return result;
}

/* 二级方法 */
// 处理来自客户机的上传文件请求
bool service_c::upload(acl::socket_stream* conn, long long bodylen) const {
    // | 包体长度 | 命令 | 状态 | 应用ID | 用户ID | 文件ID | 文件大小 | 文件内容 |
    // |    8    |  1  |  1  |   16   |  256  |  128  |    8    | 文件大小 |
    // 检查包体长度
    long long expected = APPID_SIZE + USERID_SIZE + FILEID_SIZE + BODYLEN_SIZE;
    if (bodylen < expected) {
        error(conn, -1, "Invalid Body Length: %lld < %lld", bodylen, expected);
        return false;
    }

    // 接收包体 -> | 应用ID | 用户ID |  文件ID | 文件大小 |
    // 暂时不接收 -> | 文件内容 |
    char body[expected];
    if (conn->read(body, expected) < 0) {
        logger_error("Read Fail: %s, Expected: %lld, FROM: %s", acl::last_serror(), expected, conn->get_peer());
        return false;
    }

    // 解析包体
    char appid[APPID_SIZE];
    strcpy(appid, body);
    char userid[USERID_SIZE];
    strcpy(userid, body + APPID_SIZE);
    char fileid[FILEID_SIZE];
    strcpy(fileid, body + APPID_SIZE + USERID_SIZE);
    long long filesize = ntoll(body + APPID_SIZE + USERID_SIZE + FILEID_SIZE);

    // 检查文件大小
    if (filesize != bodylen - expected) { // 正常情况下: bodylen - expected = filezise
        logger_error("Invalid File Size: %lld != %lld", filesize, bodylen - expected);
        error(conn, -1, "Invalid File Size: %lld != %lld", filesize, bodylen - expected);
        return false;
    }

    // 生成文件路径
    char filepath[PATH_MAX + 1];    // 用于存放路径的缓冲区
    if (genpath(filepath) != OK) {
        error(conn, -1, "Get Filepath Fail...");
        return false;
    }

    // 接收并保存文件
    int result = save(conn, appid, userid, fileid, filesize, filepath);
    if (result == SOCKET_ERROR) { // 套接字错误即表示无法通信, 直接返回 false 即可
        return false;
    } else if (result == ERROR) { // 本地错误还需要回复差错报文
        error(conn, -1, "Receive and Save File Fail, Fileid: %s", fileid);
        return false;
    }
    return true;
}

// 处理来自客户机的询问文件大小请求
bool service_c::filesize(acl::socket_stream* conn, long long bodylen) const {
    // | 包体长度 | 命令 | 状态 | 应用ID | 用户ID | 文件ID |
    // |    8    |  1  |  1  |   16   |  256  |  128  |
    // 检查包体长度
    long long expected = APPID_SIZE + USERID_SIZE + FILEID_SIZE;
    if (bodylen != expected) {
        error(conn, -1, "Invalid Body Length: %lld < %lld", bodylen, expected);
        return false;
    }

    // 接收包体
    char body[bodylen];
    if (conn->read(body, bodylen) < 0) {
        logger_error("Read Fail: %s, Bodylen: %lld, FROM: %s", acl::last_serror(), bodylen, conn->get_peer());
        return false;
    }

    // 解析包体
    char appid[APPID_SIZE];
    strcpy(appid, body);
    char userid[USERID_SIZE];
    strcpy(userid, body + APPID_SIZE);
    char fileid[FILEID_SIZE];
    strcpy(fileid, body + APPID_SIZE + USERID_SIZE);

    // 数据库访问对象
    db_c db;

    // 连接数据库
    if (db.connect() != OK) {
        return false;
    }

    // 文件路径
    std::string filepath;

    // 文件大小
    long long filesize;

    // 根据文件ID获取其对应的路径及大小
    if (db.get(appid, userid, fileid, filepath, &filesize) != OK) {
        error(conn, -1, "Read Database Fail, Fileid: %s", fileid);
        return false;
    }
    logger("Function `filesize`, Appid: %s, Userid: %s, Fileid: %s, Filepath: %s, Filesize: %lld", appid, userid, fileid, filepath.c_str(), filesize);
    
    // | 包体长度 | 命令 | 状态 | 文件大小 | 
    // |    8    |  1  |  1  |    8    |
    // 构造响应
    bodylen = BODYLEN_SIZE;
    long long resplen = HEADLEN + bodylen;
    char resp[resplen] = {};
    llton(bodylen, resp);
    resp[BODYLEN_SIZE] = CMD_STORAGE_REPLY;
    resp[BODYLEN_SIZE + COMMAND_SIZE] = 0;
    llton(filesize, resp + HEADLEN);

    // 发送响应
    if (conn->write(resp, resplen) < 0) {
        logger_error("Write Fail: %s, Resplen: %lld, TO: %s", acl::last_serror(), resplen, conn->get_peer());
        return false;
    }

    return true;
}

// 处理来自客户机的下载文件请求
bool service_c::download(acl::socket_stream* conn, long long bodylen) const {
    // | 包体长度 | 命令 | 状态 | 应用ID | 用户ID | 文件ID | 偏移 | 大小 |
    // |    8    |  1  |  1  |   16   |  256  |  128  |   8  |  8  |
    // 检查包体长度
    long long expected = APPID_SIZE + USERID_SIZE + FILEID_SIZE + BODYLEN_SIZE + BODYLEN_SIZE;
    if (bodylen != expected) {
        error(conn, -1, "Invalid Body Length: %lld < %lld", bodylen, expected);
        return false;
    }

    // 接收包体
    char body[bodylen];
    if (conn->read(body, bodylen) < 0) {
        logger_error("Read Fail: %s, Bodylen: %lld, FROM: %s", acl::last_serror(), bodylen, conn->get_peer());
        return false;
    }

    // 解析包体
    char appid[APPID_SIZE];
    strcpy(appid, body);
    char userid[USERID_SIZE];
    strcpy(userid, body + APPID_SIZE);
    char fileid[FILEID_SIZE];
    strcpy(fileid, body + APPID_SIZE + USERID_SIZE);
    long long offset = ntoll(body + APPID_SIZE + USERID_SIZE + FILEID_SIZE);
    long long size = ntoll(body + APPID_SIZE + USERID_SIZE + FILEID_SIZE + BODYLEN_SIZE);

    // 数据库访问对象
    db_c db;

    // 连接数据库
    if (db.connect() != OK) {
        return false;
    }

    // 文件路径
    std::string filepath;

    // 文件大小
    long long filesize;

    // 根据文件ID获取其对应的路径及大小
    if (db.get(appid, userid, fileid, filepath, &filesize) != OK) {
        error(conn, -1, "Read Database Fail, Fileid: %s", fileid);
        return false;
    }

    // 检查位置
    if (offset < 0 || filesize < offset) {
        logger_error("Invalid Offset, %lld is NOT Between 0 AND %lld", offset, filesize);
        error(conn, -1, "Invalid Offset, %lld is NOT Between 0 AND %lld", offset, filesize);
        return false;
    }

    // 大小为零表示下载文件直到文件尾
    if (!size) {
        size = filesize - offset;   // 实际的下载大小
    }

    // 检查大小
    if (size < 0 || filesize - offset < size) {
        logger_error("Invalid Size, %lld is NOT Between 0 AND %lld", size, filesize - offset);
        error(conn, -1, "Invalid Offset, %lld is NOT Between 0 AND %lld", size, filesize - offset);
        return false;
    }

    logger("Function `download`, Appid: %s, Userid: %s, Fileid: %s, Offset: %lld, Size: %lld, Filepath: %s, Filesize: %lld", appid, userid, fileid, offset, size, filepath.c_str(), filesize);

    // 读取并发送文件
    int result = send(conn, filepath.c_str(), offset, size);
    if (result == SOCKET_ERROR) { // 套接字错误即表示无法通信, 直接返回 false 即可
        return false;
    } else if (result == ERROR) { // 本地错误还需要回复差错报文
        error(conn, -1, "Receive and Save File Fail, Fileid: %s", fileid);
        return false;
    }
    return true;
}

// 处理来自客户机的删除文件请求
bool service_c::del(acl::socket_stream* conn, long long bodylen) const {
    // | 包体长度 | 命令 | 状态 | 应用ID | 用户ID | 文件ID |
    // |    8    |  1  |  1  |   16   |  256  |  128  |
    // 检查包体长度
    long long expected = APPID_SIZE + USERID_SIZE + FILEID_SIZE;
    if (bodylen != expected) {
        error(conn, -1, "Invalid Body Length: %lld < %lld", bodylen, expected);
        return false;
    }

    // 接收包体
    char body[bodylen];
    if (conn->read(body, bodylen) < 0) {
        logger_error("Read Fail: %s, Bodylen: %lld, FROM: %s", acl::last_serror(), bodylen, conn->get_peer());
        return false;
    }

    // 解析包体
    char appid[APPID_SIZE];
    strcpy(appid, body);
    char userid[USERID_SIZE];
    strcpy(userid, body + APPID_SIZE);
    char fileid[FILEID_SIZE];
    strcpy(fileid, body + APPID_SIZE + USERID_SIZE);

    // 数据库访问对象
    db_c db;

    // 连接数据库
    if (db.connect() != OK) {
        return false;
    }

    // 文件路径
    std::string filepath;

    // 文件大小
    long long filesize;

    // 根据文件ID获取其对应的路径及大小
    if (db.get(appid, userid, fileid, filepath, &filesize) != OK) {
        error(conn, -1, "Read Database Fail, Fileid: %s", fileid);
        return false;
    }

    // 删除文件ID
    if (db.del(appid, userid, fileid) != OK) {
        error(conn, -1, "Delete Database Fail, Fileid: %s", fileid);
        return false;
    }

    // 删除文件
    if (file_c::del(filepath.c_str()) != OK) {
        error(conn, -1, "Delete File Fail, Fileid: %s", fileid);
        return false;
    }

    logger("Function `del`, Delete File Success, Appid: %s, Userid: %s, Fileid: %s, Filepath: %s, Filesize: %lld", appid, userid, fileid, filepath.c_str(), filesize);
    
    return ok(conn);
}

/* 三级方法 */
// 生成文件路径
// mount
// data1 -> /dev/sda1
// data2 -> /dev/sda2
// data3 -> /dev/sda3
// ../data3/000/000/000/xxxx_000
// ../data3/000/000/000/xxxx_001
// ../data3/000/000/000/xxxx_002
//                          .
//                          .
//                          .
// ../data3/000/000/000/xxxx_511 // 进位
// ../data3/000/000/001/xxxx_000
int service_c::genpath(char* filepath) const {
    // 从存储路径表中随机抽取一个存储路径
    srand(time(NULL));
    int nspaths = g_spaths.size();
    int nrand = rand() % nspaths;
    std::string spath = g_spaths[nrand];

    // 以存储路径为键从ID服务器获取与之对应的值作为文件ID
    id_c id;
    long fileid = id.get(spath.c_str());
    if (fileid < 0) {
        return ERROR;
    }

    // 先将文件ID转换为512进制, 再根据他生成文件路径
    return id2path(spath.c_str(), id512(fileid), filepath);
}

// 将ID转换为512进制
long service_c::id512(long id) const {
    // aaa bbb ccc ddd
    // aaa*512^3 + bbb*512^2 + ccc*512 + ddd = id
    long result = 0;
    for (int i = 1; id; i *= 1000) {
        result += (id % 512) * i;
        id /= 512;
    }
    return result;
}

// 用文件ID生成文件路径
int service_c::id2path(const char* spath, long fileid, char* filepath) const {
    // 检查存储路径
    if (!spath || !strlen(spath)) {
        logger_error("Storage Path is NULL...");
        return false;
    }

    // 生成文件路径中的各个分量
    unsigned short subdir1 = (fileid / 1000000000) % 1000;    // 一级子目录
    unsigned short subdir2 = (fileid / 1000000) % 1000;       // 二级子目录
    unsigned short subdir3 = (fileid / 1000) % 1000;          // 三级子目录
    time_t         curtime = time(NULL);                      // 当前时间戳
    unsigned short postfix = (fileid / 1) % 1000;             // 文件名后缀

    // 格式化完成的文件路径
    if (spath[strlen(spath) - 1] == '/') {
        snprintf(filepath, PATH_MAX + 1, "%s%03x/%03x/%03x/%0lx_%03x", spath, subdir1, subdir2, subdir3, curtime, postfix);
    } else {
        snprintf(filepath, PATH_MAX + 1, "%s/%03x/%03x/%03x/%0lx_%03x", spath, subdir1, subdir2, subdir3, curtime, postfix);
    }

    return OK;
}

// 接收并保存文件
int service_c::save(acl::socket_stream* conn, const char* appid, const char* userid, const char* fileid, long long filesize, const char* filepath) const {
    // 文件操作对象
    file_c file;

    // 打开文件
    if (file.open(filepath, file_c::O_WRITE) != OK) {
        return ERROR;
    }

    // 依次将接收到的数据块写入文件
    long long remain = filesize;    // 未接收的字节数
    char rcvwr[STORAGE_RCVWR_SIZE]; // 接收写入缓冲区
    while (remain) { // 还有未接收数据
        // 接收数据
        long long bytes = std::min(remain, (long long)sizeof(rcvwr));
        long long count = conn->read(rcvwr, bytes);
        if (count < 0) {
            logger_error("Read Fail: %s, Bytes: %lld, FROM: %s", acl::last_serror(), bytes, conn->get_peer());
            file.close();
            return SOCKET_ERROR;
        }
        
        // 写入文件
        if (file.write(rcvwr, count) != OK) {
            file.close();
            return ERROR;
        }
        
        // 未收递减
        remain -= count;
    }

    // 关闭文件

    // 数据库访问对象

    // 连接数据库

    // 设置文件ID和路径及大小的对应关系


    return OK;
}

// 读取并发送文件
// -filepath: 发送路径 -offset: 偏移量 -size: 发送大小
int service_c::send(acl::socket_stream* conn, const char* filepath, long long offset, long long size) const {

    return OK;
}


// 应答成功
bool service_c::ok(acl::socket_stream* conn) const {
    // |包体长度|命令|状态|
    // |   8   | 1 | 1 |
    // 构造响应
    long long bodylen = 0;
    long long resplen = HEADLEN + bodylen;
    char resp[resplen] = {};
    llton(bodylen, resp);
    resp[BODYLEN_SIZE] = CMD_TRACKER_REPLY;
    resp[BODYLEN_SIZE + COMMAND_SIZE] = 0;

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
    char resp[resplen] = {};                                // |--------|-|-|--|-----|
    llton(bodylen, resp);                                   // |########|-|-|--|-----|
    resp[BODYLEN_SIZE] = CMD_TRACKER_REPLY;                 // |########|#|-|--|-----|
    resp[BODYLEN_SIZE + COMMAND_SIZE] = STATUS_ERROR;       // |########|#|#|--|-----|
    ston(errnumb, resp + HEADLEN);                          // |########|#|#|##|-----|
    if (desclen) {                                          // |########|#|#|##|#####|  当 `desclen` 不为 `0` 时执行 `if` 代码
        strcpy(resp + HEADLEN + ERROR_NUMB_SIZE, errdesc);  //              ^
                                                            //              |错误号|错误描述| 
    }

    // 发送响应
    if (conn->write(resp, resplen) < 0) {
        logger_error("Write Fail: %s, Resplen: %lld, To: %s", acl::last_serror(), resplen, conn->get_peer());
        return false;
    }

    return OK;
}
