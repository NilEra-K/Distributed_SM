// 存储服务器
// 定义主函数
#include "globals.h"
#include "server.h"

int main(void) {
    // 初始化 ACL 库
    acl::acl_cpp_init();
    acl::log::stdout_open(true);    // 将日志打印在标准输出上

    // 创建并运行服务器
    server_c& server = acl::singleton2<server_c>::get_instance();   // singleton: 单例, 这里是创建一个单例模板
                                                                    // 使用单例模板创建一个具体的实例
    server.set_cfg_str(cfg_str);    // 设置字符型参数配置表 `cfg_str`
    server.set_cfg_int(cfg_int);    // 设置整型参数配置表 `cfg_int`
    server.run_alone("127.0.0.1:23000", "../etc/storage.cfg");
    return 0;
}
