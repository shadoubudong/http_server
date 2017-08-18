#ifndef LOCK_H_
#define LOCK_H_

#include <pthread.h>
#include <semaphore.h>
#include "log.h"

class sem
{
public:
    sem()
    {
        if(sem_init(&m_sem, 0, 0) != 0)
        {
            DEBUG("sem_inti error in locker");
        }
    }
    ~sem()
    {
        sem_destroy(&m_sem);
    }

    bool wait()
    {
        return sem_wait(&m_sem);
    }

    bool post()
    {
        return sem_post(&m_sem);
    }

private:
    sem_t m_sem;
};

class locker
{
public:
    locker()
    {
        if(pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            DEBUG("Error when init mutex in locker");
        }
    }

    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

private:
    pthread_mutex_t m_mutex;
};


class cond
{
public:
    cond()
    {
        if(pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            DEBUG("mutex init fail in cond");
        }
        if(pthread_cond_init(&m_cond,NULL) != 0)
        {
            pthread_mutex_destroy(&m_mutex);
            DEBUG("cond init fail in cond");
        }
    }

    ~cond()
    {
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }

    bool wait()
    {
        int ret = 0;
        pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, &m_mutex);
        pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }

    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }

private:
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

#endif
