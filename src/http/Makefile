PROJ   = ../../bin/http
OBJS   = $(patsubst %.cpp, %.o, $(wildcard ../01_common/*.cpp *.cpp))
CC     = g++
LINK   = g++
RM     = rm -rf
CFLAGS = -c -Wall \
         -I/usr/include/acl-lib/acl_cpp \
         -I../common -I../client
LIBS   = -pthread -L../../lib -lclient -lacl_all -lz

all: $(PROJ)

$(PROJ): $(OBJS)
	$(LINK) $^ $(LIBS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $^ -o $@

clean:
	$(RM) $(PROJ) $(OBJS)
