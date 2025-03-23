#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t size) : _terminated(false) {
    for (size_t i = 0; i < size; ++i) {
        _threads.emplace_back(std::thread([this](){ // pass 'this' to access data member
            while (true) { // waiting task's loop
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(_mtx_tasks);
                    _cv.wait(lock, [this](){ // wait for another thread to fetch a task
                        return _terminated || !_tasks.empty();
                    });
                    // if we are calling dtor and there is no task, exit this thread permanently
                    if (_terminated && _tasks.empty()) return;
                    // otherwise fetch a task
                    task = _tasks.front(); _tasks.pop();
                }
                task(); // exec the task, after the task is done, go back to endless loop again
            }
        }));
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(_mtx_tasks); // TODO is it necessary to lock here?
        _terminated = true;
    }
    // notify all  in the ThreadPool
    _cv.notify_all();
    // before we release resources, wait for each thread to complete its task
    for (auto &thread : _threads)
        if (thread.joinable()) thread.join();
}
