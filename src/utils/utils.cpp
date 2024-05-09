#include "utils.h"
#include "http_handler.h"
#include <cassert>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

TimerManager<client_meta &> Utils::kTimerManager;
int *Utils::kPipeFd;
int Utils::kEpollFd;

int Utils::kTrigMode;
char *Utils::kRootDir;
std::string Utils::kUser;
std::string Utils::kPswd;
std::string Utils::kDbName;

int Utils::setNonBlocking(int fd) {
    int oldOpt = fcntl(fd, F_GETFL);
    int newOpt = oldOpt | O_NONBLOCK;
    fcntl(fd, F_SETFL, newOpt);
    return oldOpt;
}

void Utils::addFd(int fd, bool one_shot, int TRIGMode) {
    epoll_event event;
    event.data.fd = fd;

    event.events = EPOLLIN | EPOLLRDHUP;
    if (one_shot) {
        event.events |= EPOLLONESHOT;
    }
    if (TRIGMode == 1) {
        event.events |= EPOLLET;
    }
    epoll_ctl(kEpollFd, EPOLL_CTL_ADD, fd, &event);
    Utils::setNonBlocking(fd);
}

void Utils::modFd(int fd, int ev, int TRIGMode) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    if (TRIGMode == 1) {
        event.events |= EPOLLET;
    }
    epoll_ctl(kEpollFd, EPOLL_CTL_MOD, fd, &event);
}

void Utils::removeFd(int fd) {
    epoll_ctl(kEpollFd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void Utils::sigHandler(int sig) { send(kPipeFd[1], (char *)&sig, 1, 0); }

void Utils::addSig(int sig, void(handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void Utils::showError(int conn_fd, const char *info) {
    send(conn_fd, info, strlen(info), 0);
    close(conn_fd);
}

int Utils::parser_on_message_begin(http_parser *parser) { return 0; }

int Utils::parser_on_url(http_parser *parser, const char *at, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        printf("%c", at[i]);
    }
    printf("\n%p %p\n", at, reinterpret_cast<ConnectionHandler*>(parser->data)->get_read_buffer());
    return 0;
}

int Utils::parser_on_status(http_parser *parser, const char *at,
                            size_t length) {
    return 0;
}

int Utils::parser_on_header_field(http_parser *parser, const char *at,
                                  size_t length) {
    return 0;
}

int Utils::parser_on_header_value(http_parser *parser, const char *at,
                                  size_t length) {
    return 0;
}

int Utils::parser_on_headers_complete(http_parser *parser) { return 0; }

int Utils::parser_on_body(http_parser *parser, const char *at, size_t length) {
    return 0;
}

int Utils::parser_on_message_complete(http_parser *parser) { return 0; }

int Utils::parser_on_chunk_header(http_parser *parser) { return 0; }

int Utils::parser_on_chunk_complete(http_parser *parser) { return 0; }
