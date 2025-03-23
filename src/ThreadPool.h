#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

class ThreadPool {
private:
    std::vector<std::thread> _threads;
    std::queue<std::function<void()>> _tasks;
    std::mutex _mtx_tasks;
    std::condition_variable _cv;
    bool _terminated;
public:
    ThreadPool(size_t size = std::thread::hardware_concurrency());
    ~ThreadPool();

    template<class F, class... Args>
    // std::result_of is used to determine this return type at compile time.
    auto add(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
};

// add() takes a callable object f and its arguments args as input.
// It returns a std::future that will hold the result of the asynchronous execution of f with args.
template<class F, class... Args>
auto ThreadPool::add(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;

    // std::packaged_task is a class template that wraps a callable object (in this case, the result of std::bind) and makes its result available through a std::future.
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        // std::forward is used for perfect forwarding, ensuring that the arguments are passed to f with their original value category (lvalue or rvalue).
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    // The task now holds the bound function and is ready to be executed. When executed, it will store the result (or any exception thrown) internally.

    // get_future() is a method of packaged_task that retrieves the std::future object associated with the packaged_task.
    std::future<return_type> res = task->get_future();

    {
        std::unique_lock<std::mutex> lock(_mtx_tasks);
        if (_terminated)
            throw std::runtime_error("ThreadPool terminated, add task failed");
        _tasks.emplace([task](){ (*task)(); });
    }
    _cv.notify_one();

    // The function returns the std::future object res. The caller can use this future to wait for the result of the task and retrieve it (or handle any exceptions).
    return res;
}

#endif // THREADPOOL_H_
