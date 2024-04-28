#include "utils.h"
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <cassert>

TimerManager<client_meta &> Utils::timer_manager_;
int *Utils::pipe_fd_;
int Utils::epoll_fd_;

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
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event);
    Utils::setNonBlocking(fd);
}

void Utils::modFd(int fd, int ev, int TRIGMode) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    if (TRIGMode == 1) {
        event.events |= EPOLLET;
    }
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
}

void Utils::removeFd(int fd) {
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void Utils::sigHandler(int sig) {
    send(pipe_fd_[1], (char*)&sig, 1, 0);
}

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

void Utils::showError(int conn_fd, const char *info)
{
    send(conn_fd, info, strlen(info), 0);
    close(conn_fd);
}
