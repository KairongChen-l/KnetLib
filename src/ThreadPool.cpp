#include "knetlib/ThreadPool.h"

ThreadPool::ThreadPool(int size) :stop(false){
    for(int i = 0;i<size;++i){
        threads.emplace_back(std::thread([this](){
            while (true) {
                std::function<void()> task;
                {
                    //作用域加锁
                    std::unique_lock<std::mutex> lock(task_mtx);
                    //当wait的第二个条件为false时继续等
                    cv.wait(lock,[this](){
                        return stop || !tasks.empty();
                    });
                    if(stop && tasks.empty()) return;
                    task = tasks.front();
                    tasks.pop();
                }
                task(); //执行task
            }
        }));
    }
}

ThreadPool::~ThreadPool(){
    {
        std::unique_lock<std::mutex> lock(task_mtx);
        stop = true;
    }
    cv.notify_all();
    for(std::thread &th : threads){
        if(th.joinable())
            th.join();
    }
}


// void ThreadPool::add(std::function<void ()> f){
//     {
//         std::unique_lock<std::mutex> lock(task_mtx);
//         if(stop)
//             throw std::runtime_error("ThreadPool already stop, can't add task any more");
//         tasks.emplace(f);
//     }
//     //减少锁竞争用one
//     cv.notify_one();  // 每次只添加一个任务，只需唤醒一个线程
// }