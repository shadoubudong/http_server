#ifndef FD_H_
#define FD_H_

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>

#include "lock.cpp"


int setnonblocking(int fd);

void addfd(int epollfd, int fd, bool one_shot);

void removefd(int epollfd, int fd);

void modfd(int epollfd, int fd, int ev);

#endif
