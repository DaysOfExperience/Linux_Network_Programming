// 封装一下线程，线程池里包含线程
#include <functional>
#include <string>
#include <cstring>

// typedef std::function<void*(void*)> func_t;
typedef void *(*func_t)(void*);

// 实际创建线程时，传递的参数是ThreadData类型
struct ThreadData
{
public:
    std::string name_;
    void* args_;        // 创建Thread对象时，传来的例程参数，void*
};

class Thread
{
public:
    Thread(int num, func_t callback, void *args)
    : func_(callback)
    {
        char nameBuffer[1024];
        snprintf(nameBuffer, sizeof nameBuffer, "thread_%d", num);
        name_ = nameBuffer;

        tdata_.name_ = name_;
        tdata_.args_ = args;    // 线程池那里传来的this指针
    }
    ~Thread() = default;
    void create()
    {
        pthread_create(&tid_, nullptr, func_, (void*)&tdata_);
    }
    void join()
    {
        pthread_join(tid_, nullptr);
    }
    std::string name()
    {
        return name_;
    }
private:
    std::string name_;
    pthread_t tid_;   // 线程id，是在pthread_create的时候才产生的
    func_t func_;     // 线程执行例程函数
    ThreadData tdata_;  // 执行例程函数的参数
};