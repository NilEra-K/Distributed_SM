// HTTP服务器
// 定义主函数
#include "globals.h"
#include "server.h"

int main(void) {
	// 初始化ACL库
	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	// 创建并运行服务器
	server_c& server = acl::singleton2<server_c>::get_instance();
	server.set_cfg_str(cfg_str);
	server.set_cfg_int(cfg_int);
	server.set_cfg_bool(cfg_bool);
	server.run_alone("127.0.0.1:8080", "../etc/http.cfg");

	return 0;
}
