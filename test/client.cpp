#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

static const char* httprequest = "GET http://localhost/index.html HTTP/1.1\r\nConnection: keep-alive\r\n\r\nxxxxxxxxxxx";

int setnonblocking(int fd)
{
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

void addfd(int epollfd, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLERR;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

bool write_nbytes(int sockfd, const char* buffer, int len)
{
    int byte_write = 0;
    printf("write out %d bytes to sockfd %d\n", len, sockfd);

    while(1)
    {
        byte_write = send(sockfd, buffer, len, 0);

        if(byte_write == -1)
        {
            return false;
        }

        else if(byte_write == 0)
        {
            return false;
        }

        len -= byte_write;
        buffer = buffer + byte_write;

        if(len <= 0)
        {
            return true;
        }
    }
}

bool read_once(int sockfd, char* buffer, int len)
{
    int byte_read = 0;
    memset(buffer, '\0', len);
    byte_read = recv(sockfd, buffer, len, 0);

    if(byte_read == -1)
    {
        return false;
    }

    else if(byte_read == 0)
    {
        return false;
    }

    printf("read %d bytes from sockfd: %n. content is %s\n", byte_read, sockfd, buffer);
    return true;
}


/**发起num个连接**/
void start_conn(int epollfd, int num, const char* ip, int port)
{
    int ret = 0;
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(PF_INET, ip, &addr.sin_addr);

    int i=0;
    for(i=0; i<num; i++)
    {
        sleep(1);
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        printf("creat %th sock\n", (i+1));
        if(sockfd < 0)
        {
            continue;
        }

        ret = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
        if(ret == 0)
        {
            printf("the %th connection is built\n", (i+1));
            addfd(epollfd, sockfd);
        }
    }
}

void close_conn(int epollfd, int sockfd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, 0);
    close(sockfd);
}

void usage()
{
    printf("Usage is: ./client [server_address] [server_port] [number of client to create]\n");
}


int main(int argc, char **argv)
{
    /*if(argc != 4);
    {
        usage();
        exit(1);
    }
    */
    int epollfd = epoll_create(100);
    start_conn(epollfd, atoi(argv[3]), argv[1], atoi(argv[2]));
    epoll_event events[10000];

    char buffer[2048];

    while(1)
    {
        int fds = epoll_wait(epollfd, events, 10000, 2000);

        int i=0;
        for(i=0; i<fds; i++)
        {
            int sockfd = events[i].data.fd;
            if(events[i].events & EPOLLIN)
            {
                if(!read_once(sockfd, buffer, 2048))
                {
                    close_conn(epollfd, sockfd);
                }

                /***change to writeable**/
                struct epoll_event event;
                event.events = EPOLLOUT | EPOLLET | EPOLLERR;
                event.data.fd = sockfd;
                epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event);
            }

            else if(events[i].events & EPOLLOUT)
            {
                if(!write_nbytes(sockfd, httprequest, strlen(httprequest)))
                {
                    close_conn(epollfd, sockfd);
                }

                /***change to readable**/
                struct epoll_event event;
                event.events = EPOLLIN | EPOLLET | EPOLLERR;
                event.data.fd = sockfd;
                epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &event);
            }

            else if(events[i].events & EPOLLERR)
            {
                close_conn(epollfd, sockfd);
            }
        }
    }
}
