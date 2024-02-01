// ID服务器
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

    // 获取ID当前值, 同时产生下一个值
    // 通过 `key` 获取 `value`, `inc` 是增量
    int get_id(const char* key, int inc, long* value) const;

    // 析构函数
    ~db_c(void);
private:
    // MySQL 对象
    MYSQL* m_mysql;
};
