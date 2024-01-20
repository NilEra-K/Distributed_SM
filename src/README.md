### 开发
1. common
2. tracker
    | 文档/代码 | 说明 |
    | :-: | :-: |
    | `tracker/globals.h` | 声明全局变量 |
    | `tracker/globals.cpp` | 定义全局变量 |
    | `tracker/cache.h` | 声明缓存类 |
    | `tracker/cache.cpp` | 实现缓存类 |
    | `tracker/db.h` | 声明数据库访问类 |
    | `tracker/db.cpp` | 实现数据库访问类 |
    | `tracker/service.h` | 声明业务服务类 |
    | `tracker/service.cpp` | 实现业务服务类 |
    | `tracker/status.h` | 声明存储服务器状态检查线程类 |
    | `tracker/status.cpp` | 实现存储服务器状态检查线程类 |
    | `tracker/server.h` | 声明服务器类 |
    | `tracker/server.cpp` | 实现服务器类 |
    | `tracker/main.cpp` | 定义主函数 |
    | `tracker/Makefile` | 构建脚本 |
    | `etc/tracker.cfg` | 配置文件 |
    | `sql/tracker.sql` | 建表脚本 |

    在这里需要注意，应使用 `g++ -c globals.h -I/usr/include/lib-acl/acl_cpp -I../common` 执行编译操作，`-I`用于指定**非系统**头文件的位置，这里`/usr/include/lib-acl/acl_cpp`是ACL库的头文件位置，`../common`表示`types.h`的头文件位置。
    