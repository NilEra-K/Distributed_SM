// 客户机模块
// 声明连接类
#pragma once

#include <string>
#include <acl-lib/acl_cpp/lib_acl.hpp>

// 连接类
class conn_c: public acl::connect_client {
public:
    // 构造函数
    conn_c(const char* destaddr, int ctimeout = 30, int rtimeout = 60);

    // 从跟踪服务器获取存储服务器地址列表
    int saddrs(const char* appid, const char* userid, const char* fileid, std::string& saddrs);

    // 从跟踪服务器获取组列表
    int groups(std::string& groups);

    // 向存储服务器上传文件(文件格式)
    int upload(const char* appid, const char* userid, const char* fileid, const char* filepath);

    // 向存储服务器上传文件(数据格式)
    int upload(const char* appid, const char* userid, const char* fileid, const char* filedata, long long filesize);

    // 向存储服务器询问文件大小
    int filesize(const char* appid, const char* userid, const char* fileid, long long* filesize);

    // 从存储服务器下载文件
    int download(const char* appid, const char* userid, const char* fileid, long long offset, long long size, char** filedata, long long* filesize);

    // 删除存储服务器上的文件
    int del(const char* appid, const char* userid, const char* fileid);

    // 获取错误号
    short errnumb(void) const;

    // 获取错误描述
    const char* errdesc(void) const;

    // 析构函数
    ~conn_c(void);

protected:
    // 打开连接
    bool open(void);

    // 关闭连接
    void close(void);

private:
    // 构造请求
    int makerequ(char command, const char* appid, const char* userid, const char* fileid, char* requ);

    // 接收包体
    int recvbody(char** body, long long* bodylen);

    // 接收包头
    int recvhead(long long* bodylen);

    // 成员变量
    char*               m_destaddr;   // 目的地址
    int                 m_ctimeout;   // 连接超时
    int                 m_rtimeout;   // 读写超时
    acl::socket_stream* m_conn;       // 连接对象
    short               m_errnumb;    // 错误号
    acl::string         m_errdesc;    // 错误描述
};
