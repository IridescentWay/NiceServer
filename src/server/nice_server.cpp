#include "nice_server.h"
#include "utils.h"
#include <cassert>
#include <csignal>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>

NiceServer::NiceServer(int port, int trig_mode)
    : port_(port), epoll_trig_mode_(trig_mode) {
    events_ = new epoll_event[MAX_EVENTS_NUM];
    clients_ = new HttpHandler[MAX_CONNECTION_NUM];
}

NiceServer::~NiceServer() { server_destroy(); }

void NiceServer::server_init(std::string db_user, std::string db_pswd,
                             std::string db_name, int sql_num, int thread_num) {
    db_user_ = db_user;
    db_pswd_ = db_pswd;
    db_name_ = db_name;
    sql_num_ = sql_num;
    thread_num_ = thread_num;
    server_create();
}

void NiceServer::server_destroy() { close(epoll_fd_); }

void NiceServer::server_create() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_fd_ >= 0);
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port_);
    int reuse_opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse_opt,
               sizeof(reuse_opt));
    struct linger linger_opt = {1, 1};
    setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &linger_opt,
               sizeof(linger_opt));
    assert(bind(listen_fd_, (struct sockaddr *)&address, sizeof(address)) >= 0);

    epoll_fd_ = epoll_create(5);
    assert(epoll_fd_ != -1);
    Utils::addFd(listen_fd_, false, epoll_trig_mode_);
    // 创建一对 socket 用于主线程与工作线程之间的通信，pipe_fd_[0]
    // 用于主线程读，pipe_fd_[1] 用于工作线程写
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pipe_fd_) != -1);
    Utils::addFd(pipe_fd_[0], false, 0);

    Utils::setEpollFd(epoll_fd_);
    Utils::setPipeFd(pipe_fd_);

    Utils::addSig(SIGPIPE, SIG_IGN);
    Utils::addSig(SIGALRM, Utils::sigHandler, false);
    Utils::addSig(SIGTERM, Utils::sigHandler, false);

    alarm(TIME_SLOT);

    Utils::getTimerManager().set_time_slot(TIME_SLOT);

    assert(listen(listen_fd_, 5) == 0);
}

void NiceServer::server_loop() {
    bool stopServer = false;
    while (!stopServer) {
        int event_num = epoll_wait(epoll_fd_, events_, MAX_EVENTS_NUM, -1);
        if (event_num < 0 && errno != EINTR) {
            break;
        }
        for (int i = 0; i < event_num; ++i) {
            int fd = events_[i].data.fd;
            if (fd == listen_fd_) {
                // 处理新的客户连接，创建连接
                // client，把文件描述符加到epoll内核事件表
                bool success = dealwith_new_connection();
                if (!success) {
                    continue;
                }
            } else if (fd == pipe_fd_[0] && (events_[i].events & EPOLLIN)) {
                // 主线程接收到信号，处理信号
                bool success = dealwith_signal(stopServer);
                if (!success) {
                    // 日志输出处理信号失败
                }
            } else if (events_[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 客户端关闭连接，从epoll内核事件表中删除文件描述符
                clients_[fd].close_connection();
            } else if (events_[i].events & EPOLLIN) {
                // 客户端有数据可读，处理读事件
                clients_[fd].process_read();
            } else if (events_[i].events & EPOLLOUT) {
                // 客户端有数据可写，处理写事件
                clients_[fd].process_write();
            }
        }
    }
}

bool NiceServer::dealwith_new_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    if (epoll_trig_mode_ == 0) {
        // 水平触发模式，事件只要未处理会一直通知，所以可以慢慢处理，反正下次epoll_wait时还会通知
        int conn_fd = accept(listen_fd_, (struct sockaddr *)&client_addr,
                             &client_addr_len);
        if (conn_fd < 0) {
            return false;
        }
        if (HttpHandler::kClientCounter >= MAX_CONNECTION_NUM) {
            Utils::showError(conn_fd, "Internal server busy");
            return false;
        }
        clients_[conn_fd].init_connection(conn_fd, client_addr);
    } else if (epoll_trig_mode_ == 1) {
        // 边缘触发模式，事件只会通知一次，所以要在while中处理到没有就绪事件为止
        while (true) {
            int conn_fd = accept(listen_fd_, (struct sockaddr *)&client_addr,
                                 &client_addr_len);
            if (conn_fd < 0) {
                break;
            }
            if (HttpHandler::kClientCounter >= MAX_CONNECTION_NUM) {
                Utils::showError(conn_fd, "Internal server busy");
                return false;
            }
            clients_[conn_fd].init_connection(conn_fd, client_addr);
        }
        return false;
    } else {
        // 日志输出触发模式参数设置错误
    }
    return true;
}

bool NiceServer::dealwith_signal(bool& stop_server) {
    char signals[1024];
    int ret = recv(pipe_fd_[0], signals, sizeof(signals), 0);
    if (ret <= 0) {
        return false;
    }
    for (int i = 0; i < ret; ++i) {
        switch (signals[i]) {
            case SIGALRM:
                // 定时器超时，处理定时器事件
                Utils::getTimerManager().tick();
                alarm(TIME_SLOT);
                break;
            case SIGTERM:
                // 服务器关闭，处理关闭事件
                stop_server = true;
                break;
            default:
                break;
        }
    }
    return true;
}
