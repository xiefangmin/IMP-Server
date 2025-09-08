
#include "Server.h"
#include "utils.h"
#include "HttpParser.h"
#include "ImageProcessor.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <arpa/inet.h>
#include <Logger.h>

using namespace std;



#define TERMINAL_OUTPUT 0

// 调试工具函数
void debug_print_data(const char* data, size_t len, const std::string& prefix = "") 
{
    #if TERMINAL_OUTPUT
        if (!prefix.empty()) {
            std::cout << prefix << std::endl;
        }
        
        std::cout << "数据长度: " << len << " 字节" << std::endl;
        
        // 十六进制输出
        // std::cout << "Hex: ";
        // for (size_t i = 0; i < std::min(len, size_t(50)); ++i) {
        //     printf("%02x ", (unsigned char)data[i]);
        // }
        // if (len > 50) std::cout << "...";
        // std::cout << std::endl;
        
        // 字符串输出
        std::cout << "String: " << std::string(data, std::min(len, size_t(100))) << std::endl;
        std::cout << "---" << std::endl;
    # else
        if (!prefix.empty()) {
            LOG_INFO(prefix);
        }
        
        // LOG_INFO("<<< 数据长度: " + std::to_string(len) + " 字节") ;
        
        // // 十六进制输出
        // std::cout << "Hex: ";
        // for (size_t i = 0; i < std::min(len, size_t(50)); ++i) {
        //     printf("%02x ", (unsigned char)data[i]);
        // }
        // if (len > 50) std::cout << "...";
        // std::cout << std::endl;
        
        // 字符串输出
        LOG_INFO( "First 100 Bytes: \n" + std::string(data, std::min(len, size_t(100)))+
        "\n-----100 Bytes end-----");
    #endif
}



// 安全发送函数，检查send是否成功
bool safe_send(int fd, const void* data, size_t len) {
    if (fd < 0 || !data || len == 0) {
        std::cerr << "safe_send: 无效参数" << std::endl;
        return false;
    }
    
    const char* buffer = static_cast<const char*>(data);
    size_t total_sent = 0;
    
    while (total_sent < len) {
        ssize_t sent = send(fd, buffer + total_sent, len - total_sent, MSG_NOSIGNAL);
        
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 缓冲区满，等待一下再试
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            } else {
                std::cerr << "send失败: " << strerror(errno) << " (fd=" << fd << ")" << std::endl;
                return false;
            }
        } else if (sent == 0) {
            // 连接已关闭
            std::cerr << "连接已关闭 (fd=" << fd << ")" << std::endl;
            return false;
        }
        
        total_sent += sent;
    }
    
    // std::cout << "成功发送 " << total_sent << " 字节到客户端 fd=" << fd << std::endl;
    LOG_INFO("成功发送 " + std::to_string(total_sent) + " 字节到客户端 fd=" + std::to_string(fd) );
    return true;
}

// 发送HTTP响应
bool send_http_response(int fd, const std::string& response) {
    return safe_send(fd, response.c_str(), response.length());
}

// 发送图像数据
bool send_image_data(int fd, const std::vector<char>& image_data) {
    return safe_send(fd, image_data.data(), image_data.size());
}



// 支持百万级并发的配置
constexpr int MAX_EVENTS = 10000;  // 增加单次epoll_wait处理的事件数
constexpr int BUFFER_SIZE = 8192;  // 增加缓冲区大小
constexpr int MAX_CONNECTIONS = 1000000;  // 最大连接数限制

// SocketGuard 的析构函数，确保 socket 被关闭
SocketGuard::~SocketGuard() {
    if (_fd >= 0) {
        ::close(_fd);
    }
}

Server::Server(const char *addr,const std::vector<int>& ports, int thread_num)
    : _addr(addr),_ports(ports), _epoll_fd(-1), _thread_pool(thread_num), _current_connections(0) {


    // std::cout<<"Server() this->_addr: "<<this->_addr<<endl;// 输出 this->_addr: 1
    // 绑定，监听,, ip + port
    setup_listening_sockets();

    _epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1) {
        throw std::runtime_error("无法创建 epoll 实例");
    }

    // 将所有监听socket添加到epoll
    for (int listen_fd : _listen_fds) {
        epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = listen_fd;
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) == -1) {
            throw std::runtime_error("无法将监听 socket " + std::to_string(listen_fd) + " 添加到 epoll");
        }
    }
    
    // std::cout << "服务器配置: 监听端口数=" << _ports.size() 
    //           << ", 最大连接数=" << MAX_CONNECTIONS 
    //           << ", 单次事件处理数=" << MAX_EVENTS 
    //           << ", 缓冲区大小=" << BUFFER_SIZE << " 字节" << std::endl;
}

Server::~Server() {
    // 关闭所有监听socket
    for (int listen_fd : _listen_fds) {
        if (listen_fd != -1) close(listen_fd);
    }
    if (_epoll_fd != -1) close(_epoll_fd);
}

void Server::setup_listening_sockets() {
    // 多个端口
    for (int port : _ports) {
        int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd == -1) {
            throw std::runtime_error("无法创建 socket 用于端口 " + std::to_string(port));
        }

        // 允许地址重用
        int reuse = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        
        // 设置发送和接收缓冲区大小
        int send_buf_size = 4096;
        int recv_buf_size = 4096;
        setsockopt(listen_fd, SOL_SOCKET, SO_SNDBUF, &send_buf_size, sizeof(send_buf_size));
        setsockopt(listen_fd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, sizeof(recv_buf_size));
        
        // 设置TCP_NODELAY，减少延迟
        // int tcp_nodelay = 1;
        // setsockopt(listen_fd, IPPROTO_TCP, TCP_NODELAY, &tcp_nodelay, sizeof(tcp_nodelay));

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        // server_addr.sin_addr.s_addr = INADDR_ANY;
        // server_addr.sin_addr.s_addr = inet_addr("192.168.25.130");
        server_addr.sin_addr.s_addr = inet_addr(this->_addr);
        // std::cout<<"set_up this->_addr: "<<this->_addr<<endl;// 输出 this->_addr: 1
        server_addr.sin_port = htons(port);

        if (bind(listen_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(listen_fd);
            throw std::runtime_error("无法绑定到端口 " + std::to_string(port));
        }

        // 增加监听队列大小，支持更多待处理连接
        int listen_backlog = 10000;
        if (listen(listen_fd, listen_backlog) < 0) {
            close(listen_fd);
            throw std::runtime_error("无法监听端口 " + std::to_string(port));
        }

        set_non_blocking(listen_fd);
        
        _listen_fds.push_back(listen_fd);
        
        // std::cout << "端口 " << port << " 监听socket配置: 发送缓冲区=" << send_buf_size 
        //           << " 字节, 接收缓冲区=" << recv_buf_size 
        //           << " 字节, 监听队列=" << listen_backlog << std::endl;
        LOG_INFO( "端口 " + std::to_string(port) + " 监听socket配置: 发送缓冲区=" + std::to_string(send_buf_size)
            + " 字节, 接收缓冲区=" + std::to_string(recv_buf_size)
            + " 字节, 监听队列=" + std::to_string(listen_backlog) );


    }
    
    // std::cout << "共创建 " << _listen_fds.size() << " 个监听socket" << std::endl;
    LOG_INFO("共创建 " + std::to_string(_listen_fds.size()) + " 个监听socket");
}

void Server::run() {
    std::vector<epoll_event> events(MAX_EVENTS);
    while (true) {
        int n = epoll_wait(_epoll_fd, events.data(), MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait 出错");
            break;
        }

        for (int i = 0; i < n; ++i) {
            // 检查是否是监听socket
            bool is_listen_socket = false;
            for (int listen_fd : _listen_fds) {
                if (events[i].data.fd == listen_fd) {
                    handle_new_connection(listen_fd);
                    is_listen_socket = true;
                    break;
                }
            }
            
            if (!is_listen_socket) {
                handle_client_data(events[i].data.fd);
            }
        }
    }
}

void Server::handle_new_connection(int listen_fd) {
    // 检查连接数限制
    if (_current_connections >= MAX_CONNECTIONS) {
        std::cout << "达到最大连接数限制 (" << MAX_CONNECTIONS << ")，拒绝新连接" << std::endl;
        return;
    }
    
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(listen_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        // 在 ET 模式下，需要处理所有排队的连接
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
             return;
        }
        perror("accept 出错");
        return;
    }

    set_non_blocking(client_fd);
    
    _current_connections++;
    
    // 获取客户端连接的端口信息
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);
    
    // std::cout << "接受新连接，fd = " << client_fd 
    //           << " 来自 " << client_ip << ":" << client_port
    //           << " (当前连接数: " << _current_connections << "/" << MAX_CONNECTIONS << ")" << std::endl;


    LOG_INFO("接受新连接，fd = " + std::to_string(client_fd) 
              + " 来自 " + client_ip + ":" + std::to_string(client_port)
              + " (当前连接数: " + std::to_string(_current_connections) 
              + "/" + std::to_string(MAX_CONNECTIONS) + ")" ); 

    epoll_event event;
    event.events = EPOLLIN | EPOLLET; // 使用边缘触发
    event.data.fd = client_fd;
    if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
        perror("无法将客户端 socket 添加到 epoll");
        close(client_fd);
        _current_connections--;
        return;
    }
    _client_parsers[client_fd] = std::make_unique<HttpParser>();
}

void Server::handle_client_data(int client_fd) {
    std::vector<char> buffer(BUFFER_SIZE);
    
    HttpParser* parser = _client_parsers[client_fd].get();

    while(true) {
        ssize_t bytes_read = recv(client_fd, buffer.data(), BUFFER_SIZE, 0);
        // cout<<"bytes_read:"<<bytes_read<<endl;
        if (bytes_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // ET 模式下，数据已读完
                break;
            }
            perror("recv 出错");
            close_connection(client_fd);
            return;
        } else if (bytes_read == 0) {
            // 客户端关闭连接
            // std::cout << "客户端 fd=" << client_fd << " 断开连接" << std::endl;
            LOG_INFO("客户端 fd=" + std::to_string(client_fd) + " 断开连接"); 
            close_connection(client_fd);
            return;
        }


        // 使用调试函数输出读取的数据
        if(bytes_read>5 && buffer[0]=='P' && buffer[1]=='O'&& buffer[2]=='S'&& buffer[3]=='T')
            debug_print_data(buffer.data(), bytes_read, "<<< 接收客户端 fd=" + std::to_string(client_fd) 
            + " 的POST数据头, 数据长度="+std::to_string(bytes_read));
        parser->parse(buffer.data(), bytes_read);
    }


    // cout<<"读取完数据"<<endl;
    // cout<<"parser->is_request_ready():"<<parser->is_request_ready()<<endl;

    // 输出解析后的HTTP信息
    if(parser->is_request_ready()) {
#if DEBUG_OUTPUT
        std::cout << "=== HTTP请求解析完成 ===" << std::endl;
        std::cout << "方法: " << parser->get_method() << std::endl;
        std::cout << "路径: " << parser->get_path() << std::endl;
        
        std::vector<char> image_data = parser->get_image_data();
        std::string filter = parser->get_filter_type();
        
        std::cout << "滤镜类型: " << filter << std::endl;
        std::cout << "图像数据大小: " << image_data.size() << " 字节" << std::endl;
        
        if (!image_data.empty()) {
            debug_print_data(image_data.data(), std::min(image_data.size(), size_t(100)), "图像数据预览:");
        }
        std::cout << "================================" << std::endl;
#endif
    }

    if(parser->is_request_ready()) {
        std::string path = parser->get_path();

        if (path == "/" && parser->get_method() == "GET") {
            // 提供 HTML 页面
            // cout<<"GET方法，需要 提供 HTML 页面"<<endl;
            std::string html_content = load_file("web/index.html");
            if (!html_content.empty()) {
                std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(html_content.length()) + "\r\n\r\n" + html_content;
                // send(client_fd, response.c_str(), response.length(), 0);
                LOG_INFO("response to fd="+std::to_string(client_fd) 
                                +":\n HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " 
                                + std::to_string(html_content.length())  
                                + "\r\n\r\n[Serving file: web/index.htm]");


                if (!send_http_response(client_fd, response)) {
                    std::cerr << "发送HTML页面失败，客户端fd=" << client_fd << std::endl;
                }


            } else {
                std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";


                //  send(client_fd, response.c_str(), response.length(), 0);
                 if (!send_http_response(client_fd, response)) {
                    LOG_INFO("response to fd=: "+std::to_string(client_fd) + response);
                     std::cerr << "发送404响应失败，客户端fd=" << client_fd << std::endl;
                 }

            }
             close_connection(client_fd);
        } 
        else if (path == "/upload" && parser->get_method() == "POST") 
        {
            // cout<<"POST 方法，上传了图片，需要处理"<<endl;
            // 将图像处理任务添加到线程池
            std::vector<char> image_data = parser->get_image_data();
            std::string filter = parser->get_filter_type();
            std::string image_uuid = parser->get_image_uuid();
            std::string blur_intensity = parser->get_blur_intensity();
            std::string sharpen_intensity = parser->get_sharpen_intensity();
            // cout<<"filter: "<<filter<<"\nuuid: "<<image_uuid<<"\nblur_intensity: "<<blur_intensity<<"\nsharpen_intensity: "<<sharpen_intensity<<endl;
            LOG_INFO("POST DESC:\nfilter: "+filter+"\nuuid: "+image_uuid+"\nblur_intensity: "
                +blur_intensity+"\nsharpen_intensity: "+sharpen_intensity);
            
            // // 保存原始图片到根目录
            // std::string saved_filename = save_image(image_data);
            // if (!saved_filename.empty()) {
            //     std::cout << "图片已保存到根目录: " << saved_filename << std::endl;
            // } else {
            //     std::cout << "图片保存失败" << std::endl;
            // }

			                
			// // 计算并输出MD5值
			// std::string md5_hash = calculate_md5(image_data);
			// std::cout << "原始图片MD5值: " << md5_hash << std::endl;



            _thread_pool.enqueue([client_fd, image_data = std::move(image_data), filter = std::move(filter), blur_intensity = std::move(blur_intensity), sharpen_intensity = std::move(sharpen_intensity)]() {
                // RAII 包装器，确保函数退出时关闭 fd
                SocketGuard sg(client_fd); 

                std::vector<char> processed_image;
                std::string content_type = "image/jpeg";
                bool success = ImageProcessor::process(image_data, processed_image, filter, content_type, blur_intensity, sharpen_intensity);
                // std::cout<<"ImageProcessor State: "<<success<<endl;
                LOG_INFO("ImageProcessor State: " + std::to_string(success));

                // 保存处理后图片到根目录
                // std::string saved_filename = save_image(processed_image);
                // if (!saved_filename.empty()) {
                //     std::cout << "图片已保存到根目录: " << saved_filename << std::endl;
                // } else {
                //     std::cout << "图片保存失败" << std::endl;
                // }

                std::string response;
                bool send_success = false;


                if(success) {
                    response = "HTTP/1.1 200 OK\r\nContent-Type: " + content_type + "\r\nContent-Length: " + std::to_string(processed_image.size()) + "\r\n\r\n";
                    // // send(client_fd, response.c_str(), response.length(), 0);
                    // send(client_fd, processed_image.data(), processed_image.size(), 0);
                    LOG_INFO("response to fd="+std::to_string(client_fd)+ ":\n"+response + "[processed_image]");
                    // 发送HTTP响应头
                    if (!send_http_response(client_fd, response)) {
                        std::cerr << "发送HTTP响应头失败，客户端fd=" << client_fd << std::endl;
                        return;
                    }
                    
                    // 发送图像数据
                    if (!send_image_data(client_fd, processed_image)) {
                        std::cerr << "发送图像数据失败，客户端fd=" << client_fd << std::endl;
                        return;
                    }


                } else {
                    std::string error_msg = "图像处理失败";
                    response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(error_msg.length()) + "\r\n\r\n" + error_msg;
                    // send(client_fd, response.c_str(), response.length(), 0);
                    if (!send_http_response(client_fd, response)) {
                        std::cerr << "发送错误响应失败，客户端fd=" << client_fd << std::endl;
                        return;
                    }
                    send_success = true;

                }
                if (send_success) {
                    std::cout << "图像处理请求完成，客户端fd=" << client_fd << std::endl;
                }
            });

             // 从 epoll 中移除，因为任务将在线程池中完成并关闭连接
            epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
            _client_parsers.erase(client_fd);

        } else {
             std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
             send(client_fd, response.c_str(), response.length(), 0);
             close_connection(client_fd);
        }
    }
}

void Server::close_connection(int fd) {
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    _client_parsers.erase(fd);
    _current_connections--;
    // std::cout << "关闭连接 fd=" << fd << " (当前连接数: " << _current_connections << "/" << MAX_CONNECTIONS << ")" << std::endl;
    LOG_INFO("关闭连接 fd=" + std::to_string(fd) + " (当前连接数: " + std::to_string(_current_connections) + "/" + std::to_string(MAX_CONNECTIONS) + ")");
}
