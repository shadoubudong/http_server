#include "threadpool.h"

template<class T>
threadpool<T>::threadpool(int thread_num, int max_conn):m_thread_count(thread_num),
                m_max_connection(max_conn), m_stop(false), m_pid(NULL)
{
    if((thread_num <= 0) || (max_conn <= 0))
    {
        DEBUG("Construct error: param error");
    }

    m_pid = new pthread_t[thread_num]; //thread array
    if(m_pid == NULL)
    {
        DEBUG("error when create thread queue");
    }

    int i=0;
    for(i=0;i<thread_num;i++)
    {
        DEBUG("Create the &dth thread", i);
        /**注意C++调用pthread_create函数的第三个参数必须是一个静态函数，
        一个静态成员使用动态成员的方式：通过类静态对象、将类对象作为参数传给静态函数。这里使用了后者所以有this**/
        if(pthread_create(m_pid + i, NULL, worker, this) != 0)
        {
            delete[] m_pid;
            DEBUG("Create thread error");
        }

        /**let worker thread detach**/
        if(pthread_detach(m_pid[i]) != 0)
        {
            delete[] m_pid;
            DEBUG("detach thread error");
        }
    }
}

template<class T>
bool threadpool<T>::append(T* request)
{
    m_queue_lock.lock();//lock the queue
    if(m_queue.size() > m_max_connection)
    {
        m_queue_lock.unlock();
        return false;
    }
    m_queue.push_back(request);
    m_queue_lock.unlock();
    m_queue_stat.post(); //信号量+1
    return true;
}

template<class T>
void* threadpool<T>::worker(void* arg)
{
    threadpool* pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template<class T>
void threadpool<T>::run()
{
    while(!m_stop)
    {
        m_queue_stat.wait(); //信号量-1
        m_queue_lock.lock();
        if(m_queue.empty())
        {
            m_queue_lock.unlock();
            continue;
        }
        T* request = m_queue.front(); //get the request
        m_queue.pop_front();

        m_queue_lock.unlock();

        if(NULL == request)
        {
            continue;
        }
        request->process(); ////执行任务T的相应逻辑，任务T中必须有process接口
    }
}

/**test***/
/*
int main()
{
    threadpool<int> pool = NULL;
    printf("hello world \n");
    return 0;
}
*/
