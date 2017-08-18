APP = server
CXX = g++ -g -Wall -O3 -march=native -std=c++11

OBJS = server.o threadpool.o connection.o fd.o lock.o log.o
all: ${APP} server

${APP}: ${OBJS}
	${CXX} ${OBJS} -o $@ -pthread

server.o: server.cpp
connection.o: connection.cpp
threadpool.o: threadpool.cpp
fd.o: fd.cpp
lock.o: lock.cpp
log.o: log.cpp


clean:
	rm -rf *.o *.a *.so ${APP} test
