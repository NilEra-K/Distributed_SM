// ID服务器
// 实现数据库访问类
#include "globals.h"
#include "db.h"

// 数据库访问类
// 构造函数
db_c::db_c(void) : m_mysql(mysql_init(NULL)) { // 创建 MySQL 对象
    if (!m_mysql) {
        logger_error("Create DAO Fail: %s", mysql_error(m_mysql));
    }
}

// 连接数据库
int db_c::connect(void) {
    // 防止 m_mysql 对象变为 NULL, 而 mysql_real_connect() 期待传入一个 MYSQL 对象而不是 NULL, 所以备份一份 m_mysql
    MYSQL* mysql = m_mysql;

    // m_mysql = m_mysql(m_mysql, 地址, 用户名, 口令, 库名, 0, NULL, 0);
    // ^                    ^
    // 有连接状态的MySQL指针  无连接状态的MySQL指针
    // 因为我们无法保证MySQL服务器一旦连接就可以连接成功, 所以我们应该遍历MySQL列表, 尝试连接数据库
    for (std::vector<std::string>::const_iterator maddr = g_maddrs.begin(); maddr != g_maddrs.end(); ++maddr) {
        if ((m_mysql = mysql_real_connect(mysql, maddr->c_str(), "root", "123456", "tnv_idsdb", 0, NULL, 0)))
            return OK;
    }
    logger_error("Connect Database Fail: %s", mysql_error(m_mysql = mysql));
    return ERROR;
}

// 获取 ID当前值, 同时产生下一个值
int db_c::get_id(const char* key, int inc, long* value) const {
    // 关闭自动提交: 不关闭也可以, 这里是为了练习这种手动提交的方式
    mysql_autocommit(m_mysql, 0);   // `0` 表示关闭, `1` 表示开启

    // 查询数据库
    acl::string sql;
    sql.format("SELECT id_value FROM t_id_gen WHERE id='%s';", key);
    if (mysql_query(m_mysql, sql.c_str())) {
        logger_error("Query Database Fail: %s, SQL: %s", mysql_error(m_mysql), sql.c_str());
        mysql_autocommit(m_mysql, 1);
        return ERROR;
    }

    // 获取查询结果
    MYSQL_RES* res = mysql_store_result(m_mysql);
    if (!res) {
        logger_error("Result is NULL: %s, SQL: %s", mysql_error(m_mysql), sql.c_str());
        mysql_autocommit(m_mysql, 1);
        return ERROR;
    }

    // 获取结果记录
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row) { // 有记录 -> 更新旧记录->提交数据库->库中当前值
        // 更新旧记录
        sql.format("UPDATE t_id_gen SET id_value=id_value+%d WHERE id='%s';", inc, key);
        if (mysql_query(m_mysql, sql.c_str())) { // 如果执行查询出错
            logger_error("Update Database Fail: %s, SQL: %s", mysql_error(m_mysql), sql.c_str());
            mysql_autocommit(m_mysql, 1);
            return ERROR;
        }

        // 提交数据库
        mysql_commit(m_mysql);

        // 库中当前值
        *value = atol(row[0]);  // 将字符串转化为 long 型, (string) "12345" -> 12345 (long)
    } else { // 无记录(一般出现在系统刚刚启动时) -> 插入新纪录->提交数据库->缺省当前值
        // 插入新纪录
        sql.format("INSERT INTO t_id_gen SET id='%s', id_value='%d';", key, inc);
        if (mysql_query(m_mysql, sql.c_str())) { // 如果执行查询出错
            logger_error("Insert Database Fail: %s, SQL: %s", mysql_error(m_mysql), sql.c_str());
            mysql_autocommit(m_mysql, 1);
            return ERROR;
        }

        // 提交数据库
        mysql_commit(m_mysql);

        // 缺省当前值, 因为系统刚刚启动, 所以直接插入 0 即可
        *value = 0;
    }

    // 打开自动提交
    mysql_autocommit(m_mysql, 1);

    return OK;
}

// 析构函数
db_c::~db_c(void) {
    // 销毁 MySQL 对象
    if (m_mysql) {
        mysql_close(m_mysql);   // 关闭 MySQL
        m_mysql = NULL;         // 将 m_mysql 指针置为空
    }
}
