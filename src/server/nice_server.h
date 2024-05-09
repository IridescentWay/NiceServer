#ifndef __NICE_SERVER_H__
#define __NICE_SERVER_H__

#include "http_handler.h"
#include <string>
#include <sys/epoll.h>
#include <unistd.h>

constexpr int MAX_EVENTS_NUM = 10000;
constexpr int MAX_CONNECTION_NUM = 65536;
constexpr int TIME_SLOT = 5;

class NiceServer {
    using ThreadPoolPtr = std::unique_ptr<
        ThreadPool<std::function<void(ConnectionHandler &)>, ConnectionHandler>>;

  public:
    NiceServer(int port, int trig_mode);
    ~NiceServer();

    void server_init(std::string db_user, std::string db_pswd,
                     std::string db_name, int sql_num, int thread_num);
    void server_destroy();
    void server_create();
    void init_thread_pool();
    void init_sql_conn_pool();
    void server_loop();

    bool dealwith_signal(bool &stop_server);
    bool dealwith_new_connection();

  private:
    int port_;
    int listen_fd_;
    int pipe_fd_[2];
    int epoll_fd_;
    int epoll_trig_mode_;
    epoll_event *events_;

    ConnectionHandler *clients_;
    ThreadPoolPtr thread_pool_;
    char* root_path_;

    std::string db_user_;
    std::string db_pswd_;
    std::string db_name_;
    int sql_num_;

    int thread_num_;
};

#endif