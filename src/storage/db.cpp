// 存储服务器
// 实现数据库访问类
#include "types.h"
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
        if ((m_mysql = mysql_real_connect(mysql, maddr->c_str(), "root", "123456", "tnv_storagedb", 0, NULL, 0)))
            return OK;
    }
    logger_error("Connect Database Fail: %s", mysql_error(m_mysql = mysql));
    return ERROR;
}

// 根据 文件ID 获取其对应的 路径及大小
int db_c::get(const char* appid, const char* userid, const char* fileid, std::string& filepath, long long* filesize) const {
    // 先尝试从缓存中获取与文件ID对应的路径及大小
    
    return OK;
}

// 设置 文件ID和路径 及 大小 的对应关系
int db_c::set(const char* appid, const char* userid, const char* fileid, const char* filepath, long long filesize) const {
    return OK;
}

// 删除文件ID
int db_c::del(const char* appid, const char* userid, const char* fileid) const {
    return OK;
}

// 根据用户ID获取其对应的表名
std::string db_c::table_of_user(const char* userid) const {
    return "";
}

// 计算哈希值
unsigned int db_c::hash(const char* buf, size_t len) const {
    return 0;
}

// 析构函数
db_c::~db_c(void) {
    // 销毁 MySQL 对象
    if (m_mysql) {
        mysql_close(m_mysql);   // 关闭 MySQL
        m_mysql = NULL;         // 将 m_mysql 指针置为空
    }
}
