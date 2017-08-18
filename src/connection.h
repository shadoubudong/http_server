#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>

#include "lock.cpp"
#include "fd.h"

class connection
{
public:
    connection()
    {

    }
    ~connection()
    {

    }
public:
    static const int FILENAME_LEN = 200;
    static const int READBUF_SIZE = 4096;
    static const int WRITEBUF_SIZE = 4096;
    /**HTTP Method**/
    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH };
    /**HTTP请求状态：正在解析请求行、正在解析头部、解析中  **/
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
    /**请求结果：未完整的请求(客户端仍需要提交请求)、完整的请求、错误请求*/
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
    /**HTTP每行解析状态：改行解析完毕、错误的行、正在解析行**/
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };

public:
    /**所有socket上的事件都注册到一个epoll事件表中所以用static**/
    static int m_epollfd;
    static int m_user_count;

public:
    void init(int sockfd, const sockaddr_in& address); //init the new http connection
    void close_connection(bool real_close = true);
    void process(); //handle the request , use in threadpool
    bool read(); //读取客户发送来的数据(HTTP请求)
    bool write(); //将请求结果返回给客户端


private:
    /**重载init初始化连接，用于内部调用, 保护私有变量*/
    void init();
    HTTP_CODE process_read(); ////解析HTTP请求,内部调用parse_系列函数
    bool process_write(HTTP_CODE ret); //填充HTTP应答，通常是将客户请求的资源页发送给客户，内部调用add_系列函数

    /**process_read()**/
    HTTP_CODE parse_request_line(char* request); //解析HTTP请求行
    HTTP_CODE parse_header(char* text); //解析HTTP头部数据
    HTTP_CODE parse_content(char* text); //获取解析结果
    HTTP_CODE do_request(); //处理HTTP连接：内部调用process_read(),process_write()
    char* get_line();
    LINE_STATUS parse_line(); //解析行内部调用parse_request_line和parse_headers

    /***process_write***/
    void unmp(); //解除内存映射，这里内存映射是指将客户请求的资源页文件映射通过mmap映射到内存
    bool add_response(const char* format, ...);
    bool add_content(const char* format);
    bool add_status_line(int status, const char* title);
    bool add_headers(int content_len);
    bool add_content_length(int content_len);
    bool add_linger();
    bool add_blan_line();


private:
    /**连接对端的sockfd and address**/
    int m_sockfd;
    sockaddr_in m_address;

    /**read_buffer**/
    char m_read_buf[READBUF_SIZE];
    int m_read_index; //the start index to read
    int m_check_index; //index of the char which is in porcess
    int m_start_line; //the start position of the line under processing

    /**write buffer**/
    char m_write_buf[WRITEBUF_SIZE];
    int m_write_index; //写缓冲区待发送的数据

    /**HTTP解析的状态：请求行解析、头部解析**/
    CHECK_STATE m_check_state;

    /**method**/
    METHOD m_method;

    /**HTTP请求的资源页对应的文件名称，和服务端的路径拼接就形成了资源页的路径  **/
    char m_real_file[FILENAME_LEN];
    char* m_url; //请求的具体资源页名称
    char* m_version; //HTTP协议版本号
    char* m_host; //主机名，客户端要在HTTP请求中的目的主机名
    int m_content_len; //HTTP消息体的长度，简单的GET请求这个为空
    bool m_linger; //HTTP请求是否保持连接

    char* m_file_address; //资源页文件内存映射后的地址
    struct stat m_file_stat; //资源页文件的状态，stat文件结构体
    struct iovec m_iv[2]; //调用writev集中写函数需要m_iv_count表示被写内存块的数量，iovec结构体存放了一段内存的起始位置和长度
    int m_iv_count; //iovec结构体数组的长度即多少个内存块
};

#endif
