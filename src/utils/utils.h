#ifndef __UTILS_H__
#define __UTILS_H__

#include <functional>
#include <memory>
#include <netinet/in.h>
#include <queue>

template <typename Arg> class HTimer {
  public:
    using TimerCallback = std::function<void(Arg)>;

  public:
    HTimer(int expire, TimerCallback cb, Arg cb_args) : expire_(expire), cb_(cb), cb_args_(cb_args) {}
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
    bool valid_ = true;
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
        bool operator()(const TimerPtr timer1,
                        const TimerPtr timer2) {
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
    static TimerManager<client_meta &> getTimerManager() {
        return timer_manager_;
    }
    static void setPipeFd(int *pipe_fd) { pipe_fd_ = pipe_fd; }
    static void setEpollFd(int epoll_fd) { epoll_fd_ = epoll_fd; }

  private:
    static TimerManager<client_meta &> timer_manager_;
    static int *pipe_fd_;
    static int epoll_fd_;
};

#endif