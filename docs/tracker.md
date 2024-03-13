# Tracker

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

01. `globals.h`
02. `globals.cpp`
03. `cache.h`
04. `cache.h`
05. `db.h`
06. `db.cpp`
07. `service.h`
08. `service.cpp` <p>
    `service.cpp` 用于实现服务器类
	https://blog.csdn.net/weixin_38278993/article/details/104417489
	复习作用域: https://blog.csdn.net/qq_51301663/article/details/115271292
    1. 外层作用域中定义的变量在内层作用域中仍然可以使用
    2. 内层作用域中定义的变量在外层作用域中不能使用
    3. 在内层作用域中可以对外层作用域重新定义
       **注意 :** 重新定义是定义了一个新的只属于这个内层作用域的变量（名字可以不变），原本的那个外层作用域中的变量未受到影响，这个新定义的内层作用域里的变量只能在这个内层作用域中使用。（可以通过`::`来在内层作用域中使用全局作用域中的变量）
    4. 在内层作用域中可以对外层作用域中的变量更改（非重新定义）
    注意：在这种情况下，内层作用域并未新定义一个变量，而是对原本外层作用域中的变量进行了更改。
    造成这种现象的原因是：标识符的可见性。当出现两个或多个具有包含关系的作用域时，如果内层的作用域没有声明与外层作用域中同名的变量，则外层作用域中的那个变量在内层作用域中是可见的。但是如果内层作用域中声明了与外层作用域中同名的变量，则外层作用域中的那个变量在内层作用域中就是不可见的了。
09. `status.h`
10. `status.cpp`
    ```cpp
    // 复习线程类
    class Thread {
    public:
        void start(void) {
            pthread_create(..., ..., run, this);
        }

    protected:
        static void* run(void* arg) {
            return ((Thread*)arg)->run;
        }

        // 纯虚函数: 一般用于描述抽象的概念
        // 用于表达一个无法具体实现但是又必须存在的东西
        virtual void* run(void) = 0;

    private:
        ...;
    };

    // 继承线程类
    class MyThread: public Thread {
    protected:
        void* run(void) {
            // 线程执行的代码
            ...;
        }
    };
    ```
14. Makefile
    ```Makefile
    PROJ	= ../../bin/tracker
    OBJS	= $(pathsubst %.cpp, %.o, $(wildcard ../common/*.cpp *.cpp))
    CC		= g++
    LINK	= g++
    RM		= rm -rf
    CFLAGS	= -c -wall \
            -I/usr/include/acl-lib/acl_cpp \
            -I../common
    LIBS	= -pthread -lacl_all `mysql_config --libs`

    all: $(PROJ)

    $(PROJ): $(OBJS)
        $(LINK) $^ $(LIBS) -o $@

    .cpp.o:
        $(CC) $(CFLAGS)

    clean:
        $(RM) $(PROJ) $(OBJS)
    ```