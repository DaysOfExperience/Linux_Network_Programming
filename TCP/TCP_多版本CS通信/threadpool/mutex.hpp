// RAII式的mutex互斥锁封装
#pragma once
#include <pthread.h>

// 对锁指针的封装，需要一个锁指针
class Mutex
{
public:
    Mutex(pthread_mutex_t *pmtx)
    :pmtx_(pmtx)
    {}
    void lock()
    {
        pthread_mutex_lock(pmtx_);
    }
    void unlock()
    {
        pthread_mutex_unlock(pmtx_);
    }
private:
    pthread_mutex_t *pmtx_;
};

// 需要一个锁指针，RAII式的加锁解锁。
class lockGuard
{
public:
    lockGuard(pthread_mutex_t *mtx)
    : mtx_(mtx)
    {
        mtx_.lock();
    }
    ~lockGuard()
    {
        mtx_.unlock();
    }
private:
    Mutex mtx_;
};