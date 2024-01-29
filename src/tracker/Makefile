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
