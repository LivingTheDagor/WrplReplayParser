#ifndef MYEXTENSION_THREADPOOL_H
#define MYEXTENSION_THREADPOOL_H

#include <queue>
#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool {
public:
  explicit ThreadPool(size_t numThreads) : stop(false), activeTasks(0) {
    for (size_t i = 0; i < numThreads; ++i) {
      workers.emplace_back([this] {
        for (;;) {
          std::function<void()> task;

          {
            std::unique_lock<std::mutex> lock(this->queueMutex);
            this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });

            if (this->stop && this->tasks.empty())
              return;

            task = std::move(this->tasks.front());
            this->tasks.pop();
            ++activeTasks;
          }

          // Run the task outside the lock
          task();

          {
            std::unique_lock<std::mutex> lock(this->queueMutex);
            --activeTasks;
            if (tasks.empty() && activeTasks == 0) {
              joinCondition.notify_all();
            }
          }
        }
      });
    }
  }

  template<class F>
  void enqueue(F &&f) {
    {
      std::unique_lock<std::mutex> lock(queueMutex);
      if (stop)
        throw std::runtime_error("enqueue on stopped ThreadPool");
      tasks.emplace(std::forward<F>(f));
    }
    condition.notify_one();
  }

  // Waits until all tasks are done
  void join() {
    std::unique_lock<std::mutex> lock(queueMutex);
    joinCondition.wait(lock, [this] { return tasks.empty() && activeTasks == 0; });
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(queueMutex);
      stop = true;
    }

    condition.notify_all();
    for (std::thread &worker: workers) {
      worker.join();
    }
  }

private:
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;

  std::mutex queueMutex;
  std::condition_variable condition;
  std::condition_variable joinCondition;

  bool stop;
  size_t activeTasks;
};
#endif // MYEXTENSION_THREADPOOL_H
