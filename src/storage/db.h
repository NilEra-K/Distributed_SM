// 存储服务器
// 声明数据库访问类
#pragma once

#include <string>
#include <mysql/mysql.h>

// 数据库访问类
class db_c {
public:
    // 构造函数
    db_c(void);

    // 连接数据库
    int connect(void);

    // 根据文件ID获取其路径及大小
    int get(const char* appid, const char* userid, const char* fileid, std::string& filepath, long long* filesize) const;

    // 设置文件ID和路径及大小的对应关系
    int set(const char* appid, const char* userid, const char* fileid, const char* filepath, long long filesize) const;

    // 删除文件ID
    int del(const char* appid, const char* userid, const char* fileid) const;

    // 析构函数
    ~db_c(void);
private:
    // 根据用户ID获取其对应表名
    std::string table_of_user(const char* userid) const;

    // 计算哈希值
    unsigned int hash(const char* buf, size_t len) const;

    // MySQL 对象
    MYSQL* m_mysql;
};
