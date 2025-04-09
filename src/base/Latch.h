#include <condition_variable>
#include <mutex>

class Latch {
private:
    int count_;
    std::mutex mutex_;
    std::condition_variable cond_;

public:
  explicit Latch(int count) : count_(count) {}
  ~Latch() = default;

  void Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this]() { return count_ == 0; });
  }

  void CountDown() {
    std::unique_lock<std::mutex> lock(mutex_);
    --count_;
    if (count_ == 0) {
      cond_.notify_all();
    }
  }

  int GetCount() {
    std::unique_lock<std::mutex> lock(mutex_);
    return count_;
  }
};