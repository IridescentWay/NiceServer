#ifndef __UTILS_H__
#define __UTILS_H__

#include "http_parser.h"
#include "thread_pool.hpp"

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <thread>

template <typename ElementType> class SpinLockQueue {
  public:
    void push(ElementType &&element) {
        while (!locked_.exchange(true, std::memory_order_acquire))
            ;
        queue_.push(element);
        locked_.store(false, std::memory_order_release);
    }
    bool pop(ElementType &element) {
        while (true) {
            while (!locked_.exchange(true, std::memory_order_acquire))
                ;
            if (queue_.empty()) {
                locked_.store(false, std::memory_order_release);
                std::this_thread::yield();
                continue;
            }
            element = queue_.front();
            queue_.pop();
            locked_.store(false, std::memory_order_release);
            return true;
        }
    }

  private:
    std::queue<ElementType> queue_;
    std::mutex mutex_;
    std::atomic<bool> locked_{false};
};

template <typename Arg> class HTimer {
  public:
    using TimerCallback = std::function<void(Arg)>;

  public:
    HTimer(int expire, TimerCallback cb, Arg cb_args)
        : expire_(expire), cb_(cb), cb_args_(cb_args) {}
    ~HTimer() {}

    void set_expire(int expire) { expire_ = expire; }

    int get_expire() const { return expire_; }

    void callback() { cb_(cb_args_); }

    void invalid() { valid_ = false; }

    void valid() { valid_ = true; }

    bool is_valid() const { return valid_; }

  private:
    time_t expire_;
    TimerCallback cb_;
    Arg cb_args_;
    bool valid_{true};
};

template <typename Arg> class TimerManager {
  public:
    using TimerPtr = std::shared_ptr<HTimer<Arg>>;

  public:
    TimerManager() {}
    ~TimerManager() {}

    void add_timer(TimerPtr timer) {
        if (!timer) {
            return;
        }
        timer_container_.push(timer);
    }

    void update_timer(TimerPtr timer) {
        timer->set_expire(time(NULL) + time_slot_ * 3);
        TimerPtr tmp = timer_container_.top();
        // 更新完过期日期后，重新调整优先队列
        timer_container_.pop();
        timer_container_.push(tmp);
    }

    void del_timer(TimerPtr timer) {
        if (!timer) {
            return;
        }
        timer->invalid();
    }

    void tick() {
        time_t cur = time(NULL);
        while (!timer_container_.empty()) {
            TimerPtr timer = timer_container_.top();
            if (!timer->is_valid()) {
                timer_container_.pop();
                continue;
            } else if (timer->get_expire() > cur) {
                break;
            } else {
                timer->callback();
                timer_container_.pop();
            }
        }
    }

    void set_time_slot(int time_slot) { time_slot_ = time_slot; }

    int get_time_slot() { return time_slot_; }

  private:
    int time_slot_;
    struct TimerCmp {
        bool operator()(const TimerPtr timer1, const TimerPtr timer2) {
            return timer1->get_expire() > timer2->get_expire();
        }
    };
    std::priority_queue<TimerPtr, std::vector<TimerPtr>, TimerCmp>
        timer_container_;
};

struct client_meta {
    sockaddr_in address;
    int sock_fd;
    TimerManager<client_meta &>::TimerPtr timer;
};

class Utils {
  public:
    static int setNonBlocking(int fd);
    static void addFd(int fd, bool one_shot, int trig_mode);
    static void removeFd(int fd);
    static void modFd(int fd, int ev, int trig_mode);
    static void sigHandler(int sig);
    static void addSig(int sig, void(handler)(int), bool restart = true);
    static void showError(int conn_fd, const char *info);
    static TimerManager<client_meta &> &getTimerManager() {
        return kTimerManager;
    }
    static void setPipeFd(int *pipe_fd) { kPipeFd = pipe_fd; }
    static void setEpollFd(int epoll_fd) { kEpollFd = epoll_fd; }
    static void setTrigMode(int trig_mode) { kTrigMode = trig_mode; }
    static void setRootDir(char* root_dir) { kRootDir = root_dir; }
    static void setUser(std::string user) { kUser = user; }
    static void setPswd(std::string pswd) { kPswd = pswd; }
    static void setDbName(std::string db_name) { kDbName = db_name; }

    static int getTrigMode() { return kTrigMode; }

    // http_parser hooks
    static int parser_on_message_begin(http_parser *parser);
    static int parser_on_url(http_parser *parser, const char *at,
                             size_t length);
    static int parser_on_status(http_parser *parser, const char *at,
                                size_t length);
    static int parser_on_header_field(http_parser *parser, const char *at,
                                      size_t length);
    static int parser_on_header_value(http_parser *parser, const char *at,
                                      size_t length);
    static int parser_on_headers_complete(http_parser *parser);
    static int parser_on_body(http_parser *parser, const char *at,
                              size_t length);
    static int parser_on_message_complete(http_parser *parser);
    static int parser_on_chunk_header(http_parser *parser);
    static int parser_on_chunk_complete(http_parser *parser);

  private:
    static TimerManager<client_meta &> kTimerManager;
    static int *kPipeFd;
    static int kEpollFd;

    static int kTrigMode;
    static char* kRootDir;
    static std::string kUser;
    static std::string kPswd;
    static std::string kDbName;
};

#endif