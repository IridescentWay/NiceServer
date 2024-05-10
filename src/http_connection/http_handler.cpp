#include "http_handler.h"

#include <memory>

int ConnectionHandler::kClientCounter = 0;

void ConnectionHandler::closeConnection(client_meta &client) {
    if (!client.timer->is_valid()) {
        return ;
    }
    Utils::getTimerManager().del_timer(client.timer);
    Utils::removeFd(client.sock_fd);
    --kClientCounter;

    time_t now = time(NULL);
    struct tm tm_time;
    localtime_r(&now, &tm_time); // 将 time_t 转换为本地时间

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
             &tm_time); // 格式化输出时间
    printf("%s : client %d timeout \n", buffer, client.sock_fd);
}

ConnectionHandler::ConnectionHandler() {
    write_idx_ = 0;
    read_idx_ = 0;
    iov_cnt_ = 0;
}

ConnectionHandler::~ConnectionHandler() {}

void ConnectionHandler::init_connection(int connfd,
                                        const sockaddr_in &client_addr) {
    ++kClientCounter;
    client_.sock_fd = connfd;
    client_.address = client_addr;
    Utils::addFd(connfd, true, Utils::getTrigMode());
    timer_ = std::make_shared<HTimer<client_meta &>>(
        Utils::getTimerManager().get_time_slot() * 3 + time(NULL),
        ConnectionHandler::closeConnection, client_);
    timer_->valid();
    client_.timer = timer_;
    Utils::getTimerManager().add_timer(timer_);
    parser_.data = this;
    init_parser();

    time_t now = time(NULL);
    struct tm tm_time;
    localtime_r(&now, &tm_time); // 将 time_t 转换为本地时间

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
             &tm_time); // 格式化输出时间
    printf("%s : client %d connected\n", buffer, connfd);
}

void ConnectionHandler::close_connection() {
    ConnectionHandler::closeConnection(client_);
}

bool ConnectionHandler::read() {
    if (read_idx_ >= READ_BUFFER_SIZE) {
        return false;
    }
    ssize_t bytes_read;
    if (0 == Utils::getTrigMode()) {
        // 水平出发读一次
        bytes_read = recv(client_.sock_fd, read_buffer_ + read_idx_,
                          READ_BUFFER_SIZE - read_idx_, 0);
        read_idx_ += bytes_read;
        if (bytes_read <= 0) {
            return false;
        }
    } else {
        // 边缘触发读到没数据
        while (true) {
            bytes_read = recv(client_.sock_fd, read_buffer_ + read_idx_,
                              READ_BUFFER_SIZE - read_idx_, 0);
            if (bytes_read == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                return false;
            } else if (bytes_read == 0) {
                return false;
            }
            read_idx_ += bytes_read;
        }
    }
    return true;
}

void ConnectionHandler::processRead(ConnectionHandler &handler) {
    // 连接活跃，更新定时器
    Utils::getTimerManager().update_timer(handler.client_.timer);
    if (!handler.read()) {
        handler.close_connection();
        return;
    }
    printf("\n%p %p %p\n", handler.get_read_buffer(), handler.read_buffer_, reinterpret_cast<ConnectionHandler *>(handler.parser_.data)->get_read_buffer());
    handler.parser_execute(handler.read_buffer_, handler.read_idx_);
    if (handler.parser_.http_errno != HPE_OK) {
        handler.close_connection();
        return;
    }
    switch (handler.parser_.method) {
    case HTTP_GET:
        handler.do_get();
        break;
    case HTTP_POST:
        handler.do_post();
        break;
    default:
        break;
    }
}

bool ConnectionHandler::write() {
    return true;
}

void ConnectionHandler::processWrite(ConnectionHandler &handler) {
    // 写入数据
    // 关闭连接
}

http_parser_settings ConnectionHandler::kParserSetting = {
    .on_message_begin = Utils::parser_on_message_begin,
    .on_url = Utils::parser_on_url,
    .on_status = Utils::parser_on_status,
    .on_header_field = Utils::parser_on_header_field,
    .on_header_value = Utils::parser_on_header_value,
    .on_headers_complete = Utils::parser_on_headers_complete,
    .on_body = Utils::parser_on_body,
    .on_message_complete = Utils::parser_on_message_complete,
    .on_chunk_header = Utils::parser_on_chunk_header,
    .on_chunk_complete = Utils::parser_on_chunk_complete};

void ConnectionHandler::init_parser() {
    // http_parser_url_init(&url_);
    http_parser_init(&parser_, HTTP_REQUEST);
}

void ConnectionHandler::parser_execute(const char *data, size_t len) {
    http_parser_execute(&parser_, &kParserSetting, data, len);
}

bool ConnectionHandler::do_get() {
    // std::cout << url_.field_data[UF_PATH].len << std::endl;
    return true;
}

bool ConnectionHandler::do_post() {
    return true;
}
