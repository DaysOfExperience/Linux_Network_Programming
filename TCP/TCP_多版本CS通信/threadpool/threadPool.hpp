// 线程池类定义
#pragma once
#include <vector>
#include <queue>
#include <pthread.h>
#include <iostream>
#include "thread.hpp"
#include "mutex.hpp"
#include "task.hpp"

const int g_thread_num = 3;

/*
// 这样不太合适，因为这样的话，除非把它设计为单例模式，否则所有线程池共用一个任务队列，锁，条件变量，显然是不行的。
// template <class T>
// class ThreadPool
// {
// public:
//     // 构造函数
//     ThreadPool(int num = g_thread_num)
//         : thread_num_(num)
//     {
//         pthread_mutex_init(&mutex_, nullptr);
//         pthread_cond_init(&cond_, nullptr);
//     }
//     ~ThreadPool()
//     {
//         pthread_mutex_destroy(&mutex_);
//         pthread_cond_destroy(&cond_);
//     }
// 
// public:
//     void run()
//     {
//         pthread_t tid;
//         // 让所有线程执行起来，等待任务，执行任务
//         for (int i = 0; i < thread_num_; ++i)
//         {
//             pthread_create(&tid, nullptr, routine, (void *)this);
//         }
//     }
//     // 因为类成员函数，有一个隐藏的this指针，不能直接作为void* (*p)(void*)函数
//     //因此设置为static成员函数，但是routine内部要访问锁和条件变量，以及任务队列，因此把它们设置为static数据成员。
//     //但是，这样的话，所有线程池都使用一个锁，一个条件变量，一个任务队列，这无疑不是最佳方案。
//     static void *routine(void *args) // 消费者pop任务，此处为线程。
//     {
//         ThreadPool *tp = (ThreadPool *)args;
//         while (true)
//         {
//             pthread_mutex_lock(&mutex_);
//             while (task_queue_.empty())
//                 pthread_cond_wait(&cond_, &mutex_); // 线程等待任务投放，并解锁
//             T t;
//             t = task_queue_.front();
//             task_queue_.pop();
//             std::cout << "线程 " << pthread_self() << " 执行任务 : " << t() << std::endl; // 执行任务
//             pthread_mutex_unlock(&mutex_);
//         }
//         return nullptr;
//     }
//     void pushTask(const T &task) // 生产者push任务，此处为主线程。
//     {
//         pthread_mutex_lock(&mutex_);
//         task_queue_.push(task);
//         pthread_cond_signal(&cond_); // signal一下条件变量，通知线程
//         pthread_mutex_unlock(&mutex_);
//     }

// private:
//     int thread_num_;
//     static std::queue<T> task_queue_; // 缓冲区，存放任务
//     static pthread_mutex_t mutex_;    // 锁，因为static函数访问了这个
//     static pthread_cond_t cond_;      // 条件变量，因为static函数访问了这个
// };

// template <class T>
// std::queue<T> ThreadPool<T>::task_queue_;

// template <class T>
// pthread_mutex_t ThreadPool<T>::mutex_;

// template <class T>
// pthread_cond_t ThreadPool<T>::cond_;
*/

// 单例模式：把构造函数私有化，把拷贝和赋值设为delete，设置一个static接口获取单例指针。
template <class T>
class ThreadPool
{
public:
    static ThreadPool<T> *getThreadPool(int thread_num = g_thread_num)
    {
        // 这样是不安全的，因为可能多个线程执行这个getThreadPool，这样可能new出多个单例对象。
        // 因此要加锁保护
        // if(thread_pool_ptr == nullptr)
        //     thread_pool_ptr = new ThreadPool(thread_num);
        // return thread_pool_ptr;

        // 可能多次调用这个getThreadPool，但是没必要每次进来都加锁。
        // 因为只有第一次进来的时候加锁保护，防止创建多个单例对象是有意义的。
        // 
        if (nullptr == thread_pool_ptr)
        {
            lockGuard lockguard(&mutex);
            // pthread_mutex_lock(&mutex);
            if (nullptr == thread_pool_ptr)
            {
                thread_pool_ptr = new ThreadPool<T>(thread_num);
            }
            // pthread_mutex_unlock(&mutex);
        }
        return thread_pool_ptr;
    }
private:
    ThreadPool(int num = g_thread_num)
        : thread_num_(num)
    {
        pthread_mutex_init(&mutex_, nullptr);
        pthread_cond_init(&cond_, nullptr);
        for (int i = 1; i <= thread_num_; ++i)
        {
            threads_.push_back(new Thread(i, routine, (void *)this));
        }
    }
    ThreadPool(const ThreadPool& tp) = delete;
    ThreadPool& operator=(const ThreadPool& tp) = delete;
public:
    ~ThreadPool()
    {
        for (auto &iter : threads_)
        {
            iter->join();
            delete iter;
        }
        pthread_mutex_destroy(&mutex_);
        pthread_cond_destroy(&cond_);
        logMessage(NORMAL, "线程池内所有线程退出成功");
    }
    void run()
    {
        // 线程池内所有线程开始执行
        for (auto &i : threads_)
        {
            i->create();
            logMessage(NORMAL, "%s启动成功", i->name().c_str());
        }
    }
    // 避免this指针
    static void *routine(void *args)
    {
        ThreadData *td = (ThreadData *)args;
        ThreadPool *tp = (ThreadPool *)td->args_;
        while (true)
        {
            T task;
            {
                lockGuard lockguard(tp->getMutex()); // RAII加锁
                while (tp->isEmpty())
                    tp->waitCond();
                task = tp->getTask();
            }
            task(td->name_);
            // T task;
            // {
            //     // 临界区，临界资源是任务队列task_queue_
            //     lockGuard lockguard(&tp->mutex_);
            //     while(tp->task_queue_.empty())
            //         pthread_cond_wait(&tp->cond_, &tp->mutex_);
            //     task = tp->task_queue_.front();
            //     tp->task_queue_.pop();
            // }
            // task(td->name_);
        }
    }
    void pushTask(const T &task)
    {
        lockGuard lockguard(&mutex_);
        task_queue_.push(task);
        pthread_cond_signal(&cond_);
    }
public:
    pthread_mutex_t *getMutex()
    {
        return &mutex_;
    }
    bool isEmpty()
    {
        return task_queue_.empty();
    }
    void waitCond()
    {
        pthread_cond_wait(&cond_, &mutex_);
    }
    T getTask()
    {
        T ret = task_queue_.front();
        task_queue_.pop();
        return ret;
    }
private:
    std::vector<Thread *> threads_;
    int thread_num_;
    std::queue<T> task_queue_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;

    static ThreadPool<T> *thread_pool_ptr; // 单例模式指针
    static pthread_mutex_t mutex;
};

template <class T>
ThreadPool<T> *ThreadPool<T>::thread_pool_ptr = nullptr;
template <typename T>
pthread_mutex_t ThreadPool<T>::mutex = PTHREAD_MUTEX_INITIALIZER;