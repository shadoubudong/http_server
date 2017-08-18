#include "connection.h"

//定义了HTTP请求的返回状态信息，类似大家都熟悉的404
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";
const char* doc_root = "/home/Practice/Http Server/www/html";//服务端资源页的路径，将其和HTTP请求中解析的m_url拼接形成资源页的位置

int connection::m_user_count = 0;
int connection::m_epollfd = -1;

/**关闭连接，从epoll中移除描述符  **/
void connection::close_connection(bool close_true)
{
    if(close_true && m_sockfd != -1)
    {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

/***初始化连接***/
void connection::init(int sockfd, const sockaddr_in& address)
{
    m_sockfd = sockfd;
    m_address = address;

    /**avoid Time_wait**/
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    addfd(m_epollfd, m_sockfd, true);
    m_user_count++;

    /***调用重载函数***/
    init();
}


void connection::init()
{
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;

    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_len = 0;
    m_host = 0;
    m_start_line = 0;
    m_check_index = 0;
    m_read_index = 0;
    m_write_index = 0;

    memset(m_read_buf, '\0', READBUF_SIZE);
    memset(m_write_buf, '\0', WRITEBUF_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}

/**分析行，主要是看是否读到一个完整的行   '\r\n'**/
connection::LINE_STATUS connection::parse_line()
{
    char tmp;
    for(; m_check_index<m_read_index; ++m_check_index)
    {
        tmp = m_read_buf[m_check_index];
        if(tmp == '\r')
        {
            if(m_check_index + 1 == m_read_index)
            {
                return LINE_OPEN;
            }
            else if(m_read_buf[m_check_index+1] == '\n')
            {
                m_read_buf[m_check_index++] = '\0';
                m_read_buf[m_check_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if(tmp == '\n')
        {
            if(m_check_index > 1 && m_read_buf[m_check_index-1] == '\r')
            {
                m_read_buf[m_check_index-1] = '\0';
                m_read_buf[m_check_index] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    /**no '\n' or '\r'**/
    return LINE_OPEN;
}

/**循环读取HTTP请求数据  **/
bool connection::read()
{
    /***read buf full*/
    if(m_read_index >= READBUF_SIZE)
    {
        return false;
    }

    int bytes = 0;
    /**循环读取的原因是EPOLLONESHOT一个事件只触发一次所以需要一次性读取完全否则数据丢失**/
    while(true)
    {
        bytes = recv(m_sockfd, m_read_buf+m_read_index, READBUF_SIZE-m_read_index, 0);

        if(bytes == -1)
        {
            /**非阻塞描述符这两个errno不是网络出错而是设备当前不可得，在这里就是一次事件的数据读取完毕**/
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            return false; /**rcve error**/
        }
        else if(bytes == 0) /**客户端关闭了连接**/
        {
            return false;
        }
        m_read_index += bytes;
    }
    return true;
}


char* connection::get_line()
{
    return m_read_buf + m_start_line;
}

/**解析HTTP的请求行**/
connection::HTTP_CODE connection::parse_request_line(char* request)
{
    /**搜索\t的位置**/
    m_url = strpbrk(request, "\t");
    if(NULL == m_url)
    {
        return BAD_REQUEST;
    }

    *m_url++ = '\0';
    char *method = request;
    if(strcasecmp(method, "GET") == 0)
    {
        m_method = GET;
    }
    /***only surport GET**/
    else
    {
        BAD_REQUEST;
    }

    /**strspn函数是在m_url找到第一个\t位置，拼接资源页文件路径**/
    m_url += strspn(m_url, "\t");
    m_version = strpbrk(m_url, "\t");
    if(NULL == m_version)
    {
        return BAD_REQUEST;
    }
    *m_version++ = '\0';
    m_version += strspn(m_version, "\t");
    if(strcasecmp(m_version, "HTTP/1.1") != 0)
    {
        return BAD_REQUEST;
    }

    if(strncasecmp(m_url, "http://", 7) == 0)
    {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }

    if(!m_url || m_url[0] != '/')
    {
        return BAD_REQUEST;
    }

    /**状态机状态转换**/
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

/**解析HTTP头部**/
connection::HTTP_CODE connection::parse_header(char* text)
{
    if('\0' == text[0])
    {
        /**已经获取了一个完整的HTTP请求**/
        if(m_method == HEAD)
        {
            return GET_REQUEST;
        }

        /**若HTTP请求消息长度不为空,则解析头部后还要解析消息体，所以HTTP解析状态转移**/
        if(m_content_len != 0)
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    /**处理Connection头部字段**/
    else if(strncasecmp(text, "Connection:", 11) == 0)
    {
        text += 11;
        text += strspn(text, "\t");
        if(strcasecmp(text, "keep-alive") == 0)
        {
            m_linger = true;
        }
    }

    /**处理Content-Length字段**/
    else if(strncasecmp(text, "Content-Length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, "\t");
        m_content_len = atol(text);
    }

    /**处理Host字段**/
    else if(strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, "\t");
        m_host = text;
    }

    /****/
    else
    {
        DEBUG("Unknown Request");
    }
    return NO_REQUEST;
}

connection::HTTP_CODE connection::parse_content(char* text)
{
    if(m_read_index >= (m_content_len + m_check_index))
    {
        text[m_content_len] = '\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

/***主状态机, 完整的HTTP解析 ***/
connection::HTTP_CODE connection::process_read()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE  ret = NO_REQUEST;
    char* text = 0;

    /** 满足条件：正在进行HTTP解析、读取一个完整行 **/
    while(((m_check_state == CHECK_STATE_CONTENT) &&  (line_status == LINE_OK)) ||
            (line_status = parse_line()) == LINE_OK)
    {
        text = get_line();
        m_start_line = m_check_index;
        DEBUG("got a http line: %s /n", text);

        switch(m_check_state)
        {
            case CHECK_STATE_REQUESTLINE:
            {
                ret = parse_request_line(text);
                if(ret == BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }
                break;
            }

            case CHECK_STATE_HEADER:
            {
                ret = parse_header(text);
                if(ret == BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }
                else if(ret == GET_REQUEST)
                {
                    return do_request();
                }
                break;
            }

            case CHECK_STATE_CONTENT:
            {
                ret = parse_content(text);
                if(ret == GET_REQUEST)
                {
                    do_request();
                }
                line_status = LINE_OPEN;
                break;
            }

            default:
            {
                return INTERNAL_ERROR;
            }
        }
    }
    return NO_REQUEST;
}


/**得到一个完整的http请求，就分析目标文件的属性**/
connection::HTTP_CODE connection::do_request()
{
    strcpy(m_real_file, doc_root);
    int len = strlen(doc_root);
    strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
    if(stat(m_real_file, &m_file_stat)  <  0)
    {
        return NO_RESOURCE;
    }

    if(!(m_file_stat.st_mode & S_IROTH))
    {
        return FORBIDDEN_REQUEST;
    }

    if(S_ISDIR(m_file_stat.st_mode))
    {
        return BAD_REQUEST;
    }

    int fd = open(m_real_file, O_RDONLY);
    m_file_address = (char*) mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    close(fd);
    return FILE_REQUEST;
}


/**解内存映射**/
void connection::unmp()
{
    if(m_file_address != NULL)
    {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = NULL;
    }
}

/**http响应，发送给客户**/
bool connection::write()
{
    int byte_have_send = 0;
    int byte_to_send = m_write_index;
    if(byte_to_send == 0)
    {
        /**设置了oneshot，所以需要重新注册到epoll上**/
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        init();
        return true;
    }

    int tmp = 0;
    while(1)
    {
        tmp = writev(m_sockfd,  m_iv, m_iv_count);

        if(tmp <= -1)
        {
            /**TCP写缓存没有空间,等待下一轮epollout事件*/
            if(errno == EAGAIN)
            {
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            unmp();
            return false;
        }

        byte_to_send -= tmp;
        byte_have_send += tmp;

        /***发送成功*/
        if(byte_have_send >= byte_to_send)
        {
            unmp();
            if(m_linger)
            {
                init();
                modfd(m_epollfd, m_sockfd, EPOLLIN);
                return true;
            }
            else
            {
                modfd(m_epollfd, m_sockfd, EPOLLIN);
                return false;
            }
        }
    }
}

/**往写缓冲区写入待发送数据**/
bool connection::add_response(const char* format, ...)
{
    if(m_write_index >= WRITEBUF_SIZE)
    {
        return false;
    }

    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf+m_write_index, WRITEBUF_SIZE - 1 - m_write_index, format, arg_list);

    if(len >= (WRITEBUF_SIZE - m_write_index - 1))
    {
        return false;
    }

    m_write_index += len;
    va_end(arg_list);
    return true;
}

/***响应 添加状态行**/
bool connection::add_status_line(int status, const char* title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool connection::add_headers(int content_len)
{
    add_content_length(content_len);
    add_linger();
    add_blan_line();
    return true;
}

bool connection::add_content_length(int len)
{
    return add_response("Content-Length: %d\r\n", len);
}

bool connection::add_linger()
{
    return add_response("Connection: %s\r\n", (m_linger == true) ? "Keep-alive" : "Close");
}

bool connection::add_blan_line()
{
    return add_response("%s", "\r\n");
}

bool connection::add_content(const char* format)
{
    return  add_response("%s", format);
}

/**根据http请求的结果,封装应答内容**/
bool connection::process_write(HTTP_CODE ret)
{
    switch(ret)
    {
        case INTERNAL_ERROR:
        {
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if( !add_content(error_500_form))
            {
                return false;
            }
            break;
        }

        case BAD_REQUEST:
        {
            add_status_line(400, error_400_title);
            add_headers(strlen(error_400_form));
            if(!add_content(error_400_form))
            {
                return false;
            }
            break;
        }

        case NO_RESOURCE:
        {
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if( !add_content(error_404_form))
            {
                return false;
            }
            break;
        }

        case FORBIDDEN_REQUEST:
        {
            add_status_line(403, error_403_title);
            add_headers( strlen(error_403_form));
            if( !add_content(error_403_form))
            {
                return false;
            }
            break;
        }

        case FILE_REQUEST:
        {
            add_status_line(200, ok_200_title);
            /**目标文件有数据,响应头放在 m_iv[0]中， 目标文件放在 m_iv[1]中， 用writev同时写**/
            if(m_file_stat.st_size != 0)
            {
                add_headers( m_file_stat.st_size );
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_index;

                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;

                m_iv_count = 2;
                return true;
            }
            /**返回内容为空白**/
            else
            {
                const char* content = "<html><body>No Content</body></html>";
                add_headers( strlen(content));
                if(!add_content(content))
                {
                    return false;
                }
            }
        }
        default:
        {
            return false;
        }
    }

    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_index;
    m_iv_count = 1;
    return true;
}

/**处理http请求的入口函数，在线程池中有用到***/
void connection::process()
{
    HTTP_CODE read_ret = process_read();
    if(read_ret == NO_REQUEST)
    {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }

    bool write_ret = process_write(read_ret);
    if(!write_ret)
    {
        close_connection();
    }

    modfd(m_epollfd, m_sockfd, EPOLLOUT);
}
























