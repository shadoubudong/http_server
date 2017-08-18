#include "fd.h"

int setnonblocking(int fd)
{
    int old_op = fcntl(fd, F_GETFL);
    int new_op = old_op | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_op);
    return old_op;
}

void addfd(int epollfd, int fd, bool one_shot)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if(one_shot)
    {
        /**这样就可以通过手动的方式来保证同一SOCKET只能被一个线程处理，不会跨越多个线程**/
        event.events |= EPOLLONESHOT;
    }
    int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    if(ret != 0)
    {
         DEBUG("ADD FD TO EPOLL ERROR");
         return;
    }
    setnonblocking(fd);
}

void removefd(int epollfd, int fd)
{
    int ret = epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    if(ret != 0)
    {
        DEBUG("DEL FD FROM EPOLL ERROR");
        return ;
    }
    close(fd);
}

void modfd(int epollfd, int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLIN | EPOLLET | EPOLLRDHUP;
    int ret = epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
    if(ret != 0)
    {
        DEBUG("MOD FD ERROR ");
        return;
    }
}
