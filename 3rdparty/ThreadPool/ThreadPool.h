#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <syscall.h>
#include <unistd.h>
#include <unordered_map>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t);
    bool bind_core(const std::vector<int32_t>& bindcores);   
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    void wait();
    size_t getThreadCount() const;
    ~ThreadPool();
private:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
    // the task queue
    std::queue< std::function<void()> > tasks;
    
    // synchronization
    std::mutex queue_mutex;
    std::condition_variable queue_condition;
    bool stop;

    size_t thread_count;
    std::atomic<int64_t> excuting_task_num_;
    std::condition_variable waitdone_condition_;   
};
 
// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads)
    :   stop(false), excuting_task_num_(0),  thread_count(threads)
{
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
            [this]{
                std::thread::id id = std::this_thread::get_id();
                std::cout<<"user thread id: "<<id<<", kernel thread id: "<< syscall(SYS_gettid)<<std::endl;
                for(;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->queue_condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                        excuting_task_num_++;
                    }

                    task();
                    
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    excuting_task_num_--;
                    if(tasks.size() + excuting_task_num_  == 0){
                        lock.unlock();
                        this->waitdone_condition_.notify_one();
                    }
                }
            }
        );
}

inline bool ThreadPool::bind_core(const std::vector<int32_t>& bind_cores){   
    if(workers.size() != bind_cores.size()){
        std::cout<<"thread nums not equal to bind cores."<<std::endl;
        return false;
    }
    
    for(size_t i = 0; i < workers.size(); i++){
        auto threadHandle = workers[i].native_handle();
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(bind_cores[i], &cpu_set);

        if(pthread_setaffinity_np(threadHandle, sizeof(cpu_set_t), &cpu_set) != 0){
            return false;  
        }
    }

    return true;
 }

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        if(tasks.size() >= workers.size()){
            tasks.pop();
        }
        tasks.emplace([task](){ (*task)(); });
    }
    queue_condition.notify_one();
    return res;
}

inline void ThreadPool::wait(){
    std::unique_lock<std::mutex> lock(this->queue_mutex);
    this->waitdone_condition_.wait(lock, [this](){
        if(tasks.size() + excuting_task_num_ == 0)
            return true;
        return false;
        }
    );
}

inline size_t ThreadPool::getThreadCount() const {
    return thread_count;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    queue_condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}

#endif
