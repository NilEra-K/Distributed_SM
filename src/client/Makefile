PROJ	= ../../bin/client
LPRJ	= ../../lib/libclient.a
OBJS	= $(patsubst %.cpp, %.o, $(wildcard ../common/*.cpp *.cpp))
LOBJ	= $(filter-out %main.o, $(OBJS))
CC		= g++
LINK	= g++
AR		= ar rv
RM		= rm -rf
CFLAGS	= -c -Wall \
		  -I../common \
		  -I/usr/include/acl-lib/acl\
		  -I/usr/include/acl-lib/acl_cpp
		#   `mysql_config --cflags`
# ACL_DIR	= ~/Software/ACL/acl-master/
# LIBS	= -L"$(ACL_DIR)" -lacl_all -pthread `mysql_config --libs`
LIBS = -pthread -lacl_all

all: $(PROJ) $(LPRJ)

## 这里需要添加 `shared`选项, 表示链接共享库
$(PROJ): $(OBJS)
	$(LINK) $^ $(LIBS) -o $@

$(LPRJ): $(LOBJ)
	$(AR) $@ $^

.cpp.o:
	$(CC) $(CFLAGS) $^ -o $@

clean:
	$(RM) $(PROJ) $(OBJS) $(LPRJ) $(OBJS)

# /usr/bin/ld: /usr/lib/gcc/x86_64-linux-gnu/11/../../../x86_64-linux-gnu/Scrt1.o: in function `_start':
# (.text+0x1b): undefined reference to `main'
