#ifndef SERVER_H
#define SERVER_H

#include "ThreadPool.h"
#include <sys/epoll.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <atomic>

class HttpParser;

// RAII 包装类，用于自动管理文件描述符
class SocketGuard {
public:
    explicit SocketGuard(int fd) : _fd(fd) {}
    ~SocketGuard();
    SocketGuard(const SocketGuard&) = delete;
    SocketGuard& operator=(const SocketGuard&) = delete;
private:
    int _fd;
};


class Server {
public:
    Server(const char *addr,const std::vector<int>& ports, int thread_num);
    ~Server();
    void run();

private:
    void setup_listening_sockets();
    void handle_new_connection(int listen_fd);
    void handle_client_data(int client_fd);
    void close_connection(int fd);

    const char * _addr;
    std::vector<int> _ports;
    std::vector<int> _listen_fds;  // 多个监听socket
    int _epoll_fd;
    ThreadPool _thread_pool;
    
    // 从 fd 映射到 HttpParser 实例
    std::unordered_map<int, std::unique_ptr<HttpParser>> _client_parsers;
    
    // 当前连接数统计
    std::atomic<int> _current_connections;
};

#endif // SERVER_H

