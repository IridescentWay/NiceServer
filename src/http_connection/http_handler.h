#ifndef __HTTP_HANDLER_H__
#define __HTTP_HANDLER_H__

#include "utils.h"
#include <unistd.h>

class HttpHandler {
  public:
    using ClientTimer = TimerManager<client_meta &>::TimerPtr;

  public:
    HttpHandler();
    ~HttpHandler();
    void init_connection(int connfd, const sockaddr_in &client_addr);
    void close_connection();
    void process_read();
    void process_write();

  public:
    static void closeConnection(client_meta &client);

  public:
    static int kClientCounter;

  private:
    ClientTimer timer_;
    client_meta client_;
};

#endif