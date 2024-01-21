// 跟踪服务器
// 实现数据库访问类
#include "globals.h"
#include "cache.h"
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
        if (m_mysql = mysql_real_connect(mysql, maddr->c_str(), "root", "123456", "tnv_trackerdb", 0, NULL, 0))
            return OK;
    }
    logger_error("Connect Database Fail: %s", mysql_error(m_mysql = mysql));
    return ERROR;
}

// 根据 用户ID 获取其对应的 组名
int db_c::get(const char* userid, std::string& groupname) const {
    // 先尝试从缓存中获取与 用户ID 对应的组名
    // userid:tnv001 -> group001
    cache_c cache;
    acl::string key;
    key.format("userid:%s", userid);
    acl::string value;
    if (cache.get(key, value) == OK) {
        groupname = value.c_str();
        return OK;
    }

    // 缓存中没有再查询数据库
    acl::string sql;
    sql.format("SELECT group_name FROM t_router WHERE userid='%s';", userid);
    if (mysql_query(m_mysql, sql.c_str())) {
        logger_error("Query Database Fail: %s, SQL: %s", mysql_error(m_mysql), sql.c_str());
        return ERROR;
    }

    // 获取查询结果
    MYSQL_RES* res = mysql_store_result(m_mysql);
    if (!res) {
        logger_error("No Result: %s, SQL: %s", mysql_error(m_mysql), sql.c_str());
        return ERROR;
    }

    // 获取结果记录
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        logger_warn("Empty Result: %s, SQL: %s", mysql_error(m_mysql), sql.c_str());
    } else {
        groupname = row[0];
        // 将用户 ID 和 组名 的对应关系保存在缓存中
        cache.set(key, groupname.c_str());
    }
    return OK;
}

// 设置 用户ID 和 组名 的对应关系
int db_c::set(const char* appid, const char* userid, const char* groupname) const {
    // 插入
    acl::string sql;
    sql.format("INSERT INTO t_router SET appid='%s', userid='%s', groupname='%s'", appid, userid, groupname);
    if (mysql_query(m_mysql, sql.c_str())) {
        logger_error("Insert Database Fail: %s, SQL: %s", mysql_error(m_mysql), sql.c_str());
        return ERROR;
    }

    // 检查插入结果
    MYSQL_RES* res = mysql_store_result(m_mysql);
    if (!res && mysql_field_count(m_mysql)) { // mysql_field_count(m_mysql) 可以查询 m_mysql 影响到的字段
                                              // 当该值为 0 时说明 res 是可以为空指针的, 说明本来就不应该查到
                                              // 当该值非 0 时说明 res 不可以为空指针, 因为影响到了字段, 本来应该查询到的
        logger_error("Insert Database Fail: %s, SQL: %s", mysql_error(m_mysql), sql.c_str());
        return ERROR;
    }
    return OK;
}

// 获取全部组名
int db_c::get(std::vector<std::string>& groupnames) const {
    // 查询全部组名
    acl::string sql;
    sql.format("SELECT group_name FROM t_group_info;");
    if (mysql_query(m_mysql, sql.c_str())) {
        logger_error("Query Database Fail: %s, SQL: %s", mysql_error(m_mysql), sql.c_str());
        return ERROR;
    }

    // 获取查询结果
    MYSQL_RES* res = mysql_store_result(m_mysql);
    if (!res) {
        logger_error("Result is NULL: %s, SQL: %s", mysql_error(m_mysql), sql.c_str());
        return ERROR;
    }

    // 获取结果记录
    int nrows = mysql_num_rows(res);
    for (int i = 0; i < nrows; ++i) {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (!row)   break;
        groupnames.push_back(row[0]);
    }
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
