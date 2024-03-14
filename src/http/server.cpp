// HTTP服务器
// 实现服务器类
#include "types.h"
#include "client.h"
#include "globals.h"
#include "service.h"
#include "server.h"

// 进程切换用户后被调用
void server_c::proc_on_init(void) {
	// 初始化客户机
	client_c::init(cfg_taddrs);

	// 创建并初始化Redis集群
	m_redis = new acl::redis_client_cluster;
	m_redis->init(NULL, cfg_raddrs, cfg_maxthrds, cfg_ctimeout, cfg_rtimeout);

	// 打印配置信息
	logger("cfg_taddrs: %s, cfg_raddrs: %s, cfg_maxthrds: %d, cfg_ctimeout: %d, cfg_rtimeout: %d, cfg_rsession: %d", cfg_taddrs, cfg_raddrs, cfg_maxthrds, cfg_ctimeout, cfg_rtimeout, cfg_rsession);
}

// 子进程意图退出时被调用
// 返回true，子进程立即退出，否则
// 若配置项ioctl_quick_abort非0，子进程立即退出，否则
// 待所有客户机连接都关闭后，子进程再退出
bool server_c::proc_exit_timer(size_t nclients, size_t nthreads) {
	if (!nclients || !nthreads) {
		logger("nclients: %lu, nthreads: %lu", nclients, nthreads);
		return true;
	}

	return false;
}

// 进程退出前被调用
void server_c::proc_on_exit(void) {
	delete m_redis;

	// 终结化客户机
	client_c::deinit();
}

// 线程获得连接时被调用
// 返回true，连接将被用于后续通信，否则
// 函数返回后即关闭连接
bool server_c::thread_on_accept(acl::socket_stream* conn) {
	logger("connect, from: %s", conn->get_peer());

	// 设置读写超时
	conn->set_rw_timeout(cfg_rtimeout);

	// 创建会话
	acl::session* session = cfg_rsession ? (acl::session*)new acl::redis_session(*m_redis, cfg_maxthrds) :
		(acl::session*)new acl::memcache_session("127.0.0.1:11211");

	// 创建并设置业务服务对象
	conn->set_ctx(new service_c(conn, session));

	return true;
}

// 与线程绑定的连接可读时被调用
// 返回true，保持长连接，否则
// 函数返回后即关闭连接
bool server_c::thread_on_read(acl::socket_stream* conn) {
	service_c* service = (service_c*)conn->get_ctx();
	if (!service)
		logger_fatal("service is null");

	return service->doRun();
}

// 线程读写连接超时时被调用
// 返回true，继续等待下一次读写，否则
// 函数返回后即关闭连接
bool server_c::thread_on_timeout(acl::socket_stream* conn) {
	logger("read timeout, from: %s", conn->get_peer());
	return false;
}

// 与线程绑定的连接关闭时被调用
void server_c::thread_on_close(acl::socket_stream* conn) {
	logger("client disconnect, from: %s", conn->get_peer());

	service_c* service = (service_c*)conn->get_ctx();
	acl::session* session = &service->getSession();
	delete session;
	delete service;
}
