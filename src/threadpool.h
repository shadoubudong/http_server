#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <list>
#include <cstdio>
#include <pthread.h>
#include <exception>

#include "lock.cpp"
#include "log.h"

template<class T>
class threadpool
{
public:
    threadpool(int thread_num = 8, int max_conn = 10000);
    ~threadpool()
    {
        delete[] m_pid;
        m_stop = true;
    }

    bool append(T* request);//add request to queue

private:
    static void* worker(void* arg); //worker fun
    void run(); //start pool

private:
    int m_thread_count; //number of thread
    int m_max_connection; //max number of connections
    pthread_t *m_pid; //thread id
    std::list<T*> m_queue; //worker queue, shared resourec among threads
    locker m_queue_lock; //locker of worker queue
    sem m_queue_stat; //sem used by woker queue
    bool m_stop; //flag indicate stop or not
};


#endif
