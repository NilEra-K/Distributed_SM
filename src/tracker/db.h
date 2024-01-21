// 跟踪服务器
// 声明数据库访问类
#pragma once

#include <string>
#include <vector>
#include <mysql/mysql.h>

// 数据库访问类
class db_c {
public:
    // 构造函数
    db_c(void);

    // 连接数据库
    int connect(void);

    // 根据 用户ID 获取其对应的 组名
    int get(const char* userid, std::string& groupname) const;

    // 设置 用户ID 和 组名 的对应关系
    int set(const char* appid, const char* userid, const char* groupname) const;

    // 获取全部组名
    int get(std::vector<std::string>& groupnames) const;

    // 析构函数
    ~db_c(void);
private:
    // MySQL 对象
    MYSQL* m_mysql;
};
