#ifndef __THREAD_POOL_HPP__
#define __THREAD_POOL_HPP__

#include <queue>
#include <thread>
#include <vector>

template <typename ElementType> class SpinLockQueue;

template <typename JobType, typename JobArg> class ThreadPool {
  public:
    ThreadPool(int num_threads) {
        stop_ = false;
        for (int i = 0; i < num_threads; ++i) {
            threads_.emplace_back(
                std::thread(&ThreadPool::worker_thread, this));
        }
    }
    ~ThreadPool() {
        for (auto &thread : threads_) {
            thread.join();
        }
    }

    void add_job(JobType job, JobArg args) {
        jobs_queue_.push(std::make_pair(job, args));
    }

  private:
    void worker_thread() {
        while (!stop_) {
            std::pair<JobType, JobArg> job_holder;
            jobs_queue_.pop(job_holder);
            auto job = job_holder.first;
            auto args = job_holder.second;
            job(args);
        }
    }
    std::vector<std::thread> threads_;
    SpinLockQueue<std::pair<JobType, JobArg>> jobs_queue_;
    bool stop_;
};

#endif // __THREAD_POOL_HPP__