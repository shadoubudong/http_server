#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <signal.h>

#include "log.h"
#include "lock.cpp"
#include "threadpool.h"
#include "connection.h"
#include "fd.h"

#define MAX_FD 65535
#define MAX_EVENT_NUM 10000
#define BLACKLOG 5

/**信号处理函数**/
void addsig(int sig, void(handler)(int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if(restart)
    {
        sa.sa_flags |= SA_RESTART;
    }

    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void send_error(int connfd, const char* info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

void usage()
{
    printf("Usage is: ./server [ip_address] [port]\n");
}


int main(int argc, char** argv)
{
    if(argc <= 2)
    {
        usage();
    }

    const char* ip = argv[1];
    const int port = atoi(argv[2]);

    /**忽略Sigpipe信号，该信号默认动作是终止进程**/
    addsig(SIGPIPE, SIG_IGN);

    /**创建线程池**/
    threadpool<connection>* pool = NULL;
    try
    {
        pool = new threadpool< connection >;
    }
    catch( ... )
    {
        return 1;
    }

    /**预先给每个可能的用户分配一个connection对象，空间换时间**/
    connection* users = new connection[MAX_FD];
    assert(users);
    int user_count = 0;

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    struct linger tmp{1,0};
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret >= 0);

    ret = listen(listenfd, BLACKLOG);
    assert(ret >= 0);

    epoll_event events[MAX_EVENT_NUM];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd,false);

    /**m+epollfd is static, all the object of connection use the same one**/
    connection::m_epollfd = epollfd;

    while(true)
    {
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1);

        if( (num < 0)  && (errno != EAGAIN))
        {
            DEBUG("Epoll Fail");
            break;
        }

        int i=0;
        for(i=0; i<num; i++)
        {
            int sockfd = events[i].data.fd;
            if(listenfd == sockfd)
            {
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int connfd = accept(listenfd, (struct sockaddr*)&client, &len);

                if(connfd < 0)
                {
                    DEBUG("Error when accept");
                    continue;
                }
                if(connection::m_user_count > MAX_FD)
                {
                    DEBUG("too many connections, wait later");
                    continue;
                }

                /**init client**/
                users[connfd].init(connfd, client);
            }

            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                users[sockfd].close_connection();
            }

            else if(events[i].events & EPOLLIN)
            {
                if(users[sockfd].read())
                {
                    pool->append(users + sockfd);
                }
                else
                {
                    users[sockfd].close_connection();
                }
            }

            else if(events[i].events & EPOLLOUT)
            {
                if(!users[sockfd].write())
                {
                    users[sockfd].close_connection();
                }
            }
            else
            {
                /**do nothing*/
            }
        }
    }

    close(epollfd);
    close(listenfd);
    delete[] users;
    delete pool;
    return 0;
}

