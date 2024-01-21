// 跟踪服务器
// 实现缓存类

#include "globals.h"
#include "cache.h"

// 根据键获取其值
int cache_c::get(const char* key, acl::string& value) const {
    // 构造键
    acl::string tracker_key;
    tracker_key.format("%s:%s", TRACKER_REDIS_PREFIX, key);

    // 检查 Redis 连接池
    if (!g_rconns) {
        logger_warn("Redis Connection Pool is NULL, Key: %s", tracker_key.c_str());
        return ERROR;
    }

    // 从连接池中获取一个 Redis 连接
    // 使用 peek() 可以拿出一个连接, 此时可以查看 peek() 函数的返回类型为 connect_client 类型
    // 使用 类型转换可以向下"造型"
    acl::redis_client* rconn = (acl::redis_client*)g_rconns->peek();
    if (!rconn) {
        logger_warn("Peek Redis Connection Fail, Key: %s", tracker_key.c_str());
        return ERROR;
    }

    // 持有此连接的 Redis 对象即为 Redis 客户机
    acl::redis redis;           // 此时 Redis 对象和只是缺省构造, 并没有与服务器建立联系
    redis.set_client(rconn);    // 使用 rconn 连接给 redis, rconn已经连接好了服务器

    // 借助 Redis 客户机, 根据键获取其值
    if (!redis.get(tracker_key.c_str(), value)) {
        logger_warn("Get Cache Fail, Key: %s", tracker_key.c_str());
        g_rconns->put(rconn, false);    // 将连接放回连接池, 并关闭连接, 即认为该连接是有问题的
        return ERROR;
    }

    // 检查空值
    if (value.empty()) {
        logger_warn("Value is Empty, Key: %s", tracker_key.c_str());
        return ERROR;
    }

    logger("Get Cache OK, Key: %s, Value: %s", tracker_key.c_str(), value.c_str());
    g_rconns->put(rconn, true);         // 将连接放回连接池, 但是不关闭连接, 即将 rconn 标记为闲
    return OK;
}

// 设置指定键的值
int cache_c::set(const char* key, const char* value, int timeout) const {
    // 构造键
    acl::string tracker_key;
    tracker_key.format("%s:%s", TRACKER_REDIS_PREFIX, key);

    // 检查 Redis 连接池
     if (!g_rconns) {
        logger_warn("Redis Connection Pool is NULL, Key: %s", tracker_key.c_str());
        return ERROR;
    }

    // 从连接池中获取一个 Redis 连接
    acl::redis_client* rconn = (acl::redis_client*)g_rconns->peek();
    if (!rconn) {
        logger_warn("Peek Redis Connection Fail, Key: %s", tracker_key.c_str());
        return ERROR;
    }

    // 持有此连接的 Redis 对象即为 Redis 客户机
    acl::redis redis;           // 此时 Redis 对象和只是缺省构造, 并没有与服务器建立联系
    redis.set_client(rconn);    // 使用 rconn 连接给 redis, rconn已经连接好了服务器

    // 借助 Redis 客户机, 设置指定键的值
    if (timeout < 0)    timeout = cfg_ktimeout; // 如果 timeout < 0, 则将 timeout 设置为默认的配置值
    if (!redis.setex(tracker_key.c_str(), value, timeout)) {
        logger_warn("Set Cache Fail, Key: %s, Value: %s, Timeout: %d", tracker_key.c_str(), value, timeout);
        g_rconns->put(rconn, false);    // 将连接放回连接池, 并关闭连接, 即认为该连接是有问题的
        return ERROR;
    }
    
    // 如果成功, 打印日志
    logger("Set Cache OK, Key: %s, Value: %s", tracker_key.c_str(), value);
    g_rconns->put(rconn, true);         // 将连接放回连接池, 但是不关闭连接, 即将 rconn 标记为闲
    return OK;
}

//
int cache_c::del(const char* key) const {
    // 构造键
    acl::string tracker_key;
    tracker_key.format("%s:%s", TRACKER_REDIS_PREFIX, key);

    // 检查 Redis 连接池
     if (!g_rconns) {
        logger_warn("Redis Connection Pool is NULL, Key: %s", tracker_key.c_str());
        return ERROR;
    }

    // 从连接池中获取一个 Redis 连接
    acl::redis_client* rconn = (acl::redis_client*)g_rconns->peek();
    if (!rconn) {
        logger_warn("Peek Redis Connection Fail, Key: %s", tracker_key.c_str());
        return ERROR;
    }

    // 持有此连接的 Redis 对象即为 Redis 客户机
    acl::redis redis;           // 此时 Redis 对象和只是缺省构造, 并没有与服务器建立联系
    redis.set_client(rconn);    // 使用 rconn 连接给 redis, rconn已经连接好了服务器

    // 借助 Redis 客户机, 删除指定键值对
    if (!redis.del_one(tracker_key.c_str())) {
        logger_warn("Delete Cache Fail, Key: %s", tracker_key.c_str());
        g_rconns->put(rconn, false);    // 将连接放回连接池, 并关闭连接, 即认为该连接是有问题的
        return ERROR;
    }
    
    // 如果成功, 打印日志
    logger("Delete Cache OK, Key: %s", tracker_key.c_str());
    g_rconns->put(rconn, true);         // 将连接放回连接池, 但是不关闭连接, 即将 rconn 标记为闲
    return OK;
}