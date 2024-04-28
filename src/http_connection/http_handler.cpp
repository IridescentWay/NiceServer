#include "http_handler.h"

#include <memory>

int HttpHandler::kClientCounter = 0;

void HttpHandler::closeConnection(client_meta &client) {
    Utils::getTimerManager().del_timer(client.timer);
    Utils::removeFd(client.sock_fd);
    --kClientCounter;

    time_t now = time(NULL);
    struct tm tm_time;
    localtime_r(&now, &tm_time); // 将 time_t 转换为本地时间

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_time); // 格式化输出时间
    printf("%s : client %d timeout \n", buffer, client.sock_fd);
}

HttpHandler::HttpHandler() {}

HttpHandler::~HttpHandler() {}

void HttpHandler::init_connection(int connfd, const sockaddr_in &client_addr) {
    client_.sock_fd = connfd;
    client_.address = client_addr;
    timer_ = std::make_shared<HTimer<client_meta &>>(
        Utils::getTimerManager().get_time_slot() * 3 + time(NULL), HttpHandler::closeConnection, client_);
    timer_->valid();
    client_.timer = timer_;
    Utils::getTimerManager().add_timer(timer_);

    time_t now = time(NULL);
    struct tm tm_time;
    localtime_r(&now, &tm_time); // 将 time_t 转换为本地时间

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_time); // 格式化输出时间
    printf("%s : client %d connected\n", buffer, connfd);
}

void HttpHandler::close_connection() {
    HttpHandler::closeConnection(client_);
}

void HttpHandler::process_read() {
    // 读取数据
    // 解析数据
    // 处理数据
    // 生成响应
    // 写入数据
    // 关闭连接
}

void HttpHandler::process_write() {
    // 写入数据
    // 关闭连接
}