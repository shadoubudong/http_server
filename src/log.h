#ifndef LOG_H
#define LOG_H

/**
*日志类，采用单例设计模式，不允许复制拷贝
**/

#include <assert.h>
#include <string>

/**定义日志级别**/
#define LOG_NORMAL 0
#define LOG_DEBUG 1
#define LOG_WARN 2
#define LOG_ERROR 3

#define NORMAL(...)         log::get_instance()->logFun(__FILE__, __LINE__, LOG_NORMAL, __VA_ARGS__)
#define DEBUG(...)          log::get_instance()->logFun(__FILE__, __LINE__, LOG_DEBUG, __VA_ARGS__)
#define WARN(...)           log::get_instance()->logFun(__FILE__, __LINE__, LOG_WARN, __VA_ARGS__)
#define ERROR(...)          log::get_instance()->logFun(__FILE__, __LINE__, LOG_ERROR, __VA_ARGS__)

/**定义日志输出的颜色**/
#define PRINT_COLOR_NONE        "\033[m"
#define PRINT_COLOR_BLACK       "\033[30m"
#define PRINT_COLOR_RED         "\033[31m"
#define PRINT_COLOR_GREED       "\033[32m"
#define PRINT_COLOR_YELLOW      "\033[33m"
#define PRINT_COLOR_BLUE        "\033[34m"
#define PRINT_COLOR_PURPLE      "\033[35m"
#define PRINT_COLOR_DARK_GREEN  "\033[36m"
#define PRINT_COLOR_WHITR       "\033[37m"

void LogAssertError(const char * exp);
#ifdef NDEBUG
#define ASSERT(exp) ((exp) ? (void)0 : LogAssertError(#exp))
#else
#define ASSERT(exp) assert(exp)
#endif

/**采用单例模式，一个客户端只允许有一个log实例**/
class log
{
    private:
        log(){};
        virtual ~log(){};

    public:
        /**获取log实例**/
        static log* get_instance()
        {
            static log mLog;
            return &mLog;
        }

        void logFun(const char* file, int line, const int level, const char* format, ...);

    protected:

    private:
        /**拷贝和赋值定义为空，不允许进行拷贝和复制**/
        log(const log&);
        log& operator=(const log& llog);

        /**返回日志级别**/
        std::string get_level(int level);

};

#endif // LOG_H
