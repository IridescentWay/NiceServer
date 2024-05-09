#ifndef __HTTP_HANDLER_H__
#define __HTTP_HANDLER_H__

#include "utils.h"
#include <unistd.h>

class ConnectionHandler {
  public:
    using ClientTimer = TimerManager<client_meta &>::TimerPtr;

  public:
    ConnectionHandler();
    ~ConnectionHandler();
    void init_connection(int connfd, const sockaddr_in &client_addr);
    void close_connection();
    // client_meta &get_client() { return client_; }

    // http_parser
    void init_parser();
    void parser_execute(const char *data, size_t len);
    bool read();
    bool write();
    const char *get_read_buffer() const { return read_buffer_; }

    // http handle
    bool do_get();
    bool do_post();

  public:
    static void closeConnection(client_meta &client);
    static void processRead(ConnectionHandler &handler);
    static void processWrite(ConnectionHandler &handler);

  public:
    static int kClientCounter;

  private:
    static constexpr int WRITE_BUFFER_SIZE = 2048;
    static constexpr int READ_BUFFER_SIZE = 2048;
    ClientTimer timer_;
    client_meta client_;
    char write_buffer_[WRITE_BUFFER_SIZE];
    int write_idx_;
    char read_buffer_[READ_BUFFER_SIZE];
    int read_idx_;
    // 如果同时往客户端写数据和文件可以分别使用iov_[0]和iov_[1]
    struct iovec iov_[2];
    int iov_cnt_;
    http_parser_url url_;
    http_parser parser_;
    static http_parser_settings kParserSetting;
};

#endif