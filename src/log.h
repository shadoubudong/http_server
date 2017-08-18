#ifndef LOG_H
#define LOG_H

/**
*��־�࣬���õ������ģʽ���������ƿ���
**/

#include <assert.h>
#include <string>

/**������־����**/
#define LOG_NORMAL 0
#define LOG_DEBUG 1
#define LOG_WARN 2
#define LOG_ERROR 3

#define NORMAL(...)         log::get_instance()->logFun(__FILE__, __LINE__, LOG_NORMAL, __VA_ARGS__)
#define DEBUG(...)          log::get_instance()->logFun(__FILE__, __LINE__, LOG_DEBUG, __VA_ARGS__)
#define WARN(...)           log::get_instance()->logFun(__FILE__, __LINE__, LOG_WARN, __VA_ARGS__)
#define ERROR(...)          log::get_instance()->logFun(__FILE__, __LINE__, LOG_ERROR, __VA_ARGS__)

/**������־�������ɫ**/
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

/**���õ���ģʽ��һ���ͻ���ֻ������һ��logʵ��**/
class log
{
    private:
        log(){};
        virtual ~log(){};

    public:
        /**��ȡlogʵ��**/
        static log* get_instance()
        {
            static log mLog;
            return &mLog;
        }

        void logFun(const char* file, int line, const int level, const char* format, ...);

    protected:

    private:
        /**�����͸�ֵ����Ϊ�գ���������п����͸���**/
        log(const log&);
        log& operator=(const log& llog);

        /**������־����**/
        std::string get_level(int level);

};

#endif // LOG_H
