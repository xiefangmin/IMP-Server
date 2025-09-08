#include "HttpParser.h"
#include <iostream>
#include <algorithm>
#include <cstring>

HttpParser::HttpParser() : _state(ParseState::METHOD), _content_length(0) {}

void HttpParser::reset() {
    _state = ParseState::METHOD;
    _buffer.clear();
    _method.clear();
    _path.clear();
    _headers.clear();
    _body.clear();
    _content_length = 0;
    _boundary.clear();
    _image_data.clear();
    _filter_type.clear();
    _image_uuid.clear();
    _blur_intensity.clear();
    _sharpen_intensity.clear();
}

void HttpParser::parse(const char* data, size_t len) {
    // 状态机：只要请求还未完成，就持续解析
    // 步骤1: 解析请求行和头部
    if (_state != ParseState::BODY && _state != ParseState::COMPLETE) {
        // 将新数据追加到内部缓冲区
        _buffer.append(data, len);
        
        // 查找头部结束标记 "\r\n\r\n"
        size_t header_end_pos = _buffer.find("\r\n\r\n");
        if (header_end_pos != std::string::npos) {
            // 解析所有头部信息
            parse_headers();
            
            // 将头部之后的数据移入 _body
            std::string body_part = _buffer.substr(header_end_pos + 4);
            if (!body_part.empty()) {
                _body.assign(body_part.begin(), body_part.end());
            }
            _buffer.clear(); // 清空缓冲区，因为它只用于暂存头部
            
            // 检查是否需要进入BODY状态
            if (_content_length > 0) {
                _state = ParseState::BODY;
            } else {
                // GET请求或没有body的请求，直接标记完成
                _state = ParseState::COMPLETE;
            }
        }
    } 
    // 步骤2: 如果已进入BODY状态，继续接收数据
    else if (_state == ParseState::BODY) {
        _body.insert(_body.end(), data, data + len);
    }

    // 步骤3: 检查body是否接收完整
    if (_state == ParseState::BODY && _content_length > 0 && _body.size() >= _content_length) {
       // 确保body不多不少，丢弃可能存在的多余数据 (如流水线请求的下一部分)
       _body.resize(_content_length); 
       // 解析 multipart 格式的 body
       parse_multipart_body();
       _state = ParseState::COMPLETE; // 标记整个请求解析完成
    }
}


void HttpParser::parse_headers() {
    size_t line_end = 0;
    size_t line_start = 0;

    // 解析请求行 (例如 "POST /upload HTTP/1.1")
    line_end = _buffer.find("\r\n");
    if (line_end != std::string::npos) {
        std::string request_line = _buffer.substr(0, line_end);
        size_t method_end = request_line.find(' ');
        if(method_end != std::string::npos) {
            _method = request_line.substr(0, method_end);
            size_t path_start = method_end + 1;
            size_t path_end = request_line.find(' ', path_start);
            if(path_end != std::string::npos) {
                _path = request_line.substr(path_start, path_end - path_start);
            }
        }
        line_start = line_end + 2;
    }

    // 循环解析每一个头部字段
    while((line_end = _buffer.find("\r\n", line_start)) != std::string::npos) {
        // 遇到空行，表示头部区域结束
        if (line_end == line_start) { 
            break;
        }
        std::string header_line = _buffer.substr(line_start, line_end - line_start);
        size_t colon_pos = header_line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = header_line.substr(0, colon_pos);
            // 跳过冒号和空格
            std::string value = header_line.substr(colon_pos + 2); 
            _headers[key] = value;
        }
        line_start = line_end + 2;
    }

    // 从头部中提取关键信息
    if (_headers.count("Content-Length")) {
        try {
            _content_length = std::stoul(_headers["Content-Length"]);
        } catch(const std::exception& e) {
            _content_length = 0;
        }
    }
    if (_headers.count("Content-Type")) {
        std::string content_type = _headers["Content-Type"];
        size_t boundary_pos = content_type.find("boundary=");
        if (boundary_pos != std::string::npos) {
            // multipart 的边界字符串
            _boundary = "--" + content_type.substr(boundary_pos + 9);
        }
    }
}

void HttpParser::parse_multipart_body() {
    if (_boundary.empty() || _body.empty()) return;

    // 使用 char* 和 memmem 进行高效的二进制查找
    const char* body_data = _body.data();
    size_t body_size = _body.size();
    const char* boundary_cstr = _boundary.c_str();
    size_t boundary_len = _boundary.length();

    const char* current_pos = body_data;
    
    // 循环查找每个 part
    while(current_pos < body_data + body_size) {
        const char* boundary_start = static_cast<const char*>(memmem(current_pos, (body_data + body_size) - current_pos, boundary_cstr, boundary_len));
        
        if (!boundary_start) break;
        
        // 查找下一个边界
        const char* next_boundary_start = static_cast<const char*>(memmem(boundary_start + boundary_len, (body_data + body_size) - (boundary_start + boundary_len), boundary_cstr, boundary_len));
        
        if(!next_boundary_start) break;

        // 提取一个 part 的内容 (不包括边界字符串)
        const char* part_start = boundary_start + boundary_len;
        // 跳过 boundary 后的 \r\n
        if(part_start + 2 <= body_data + body_size && part_start[0] == '\r' && part_start[1] == '\n') {
            part_start += 2;
        }
        
        size_t part_len = next_boundary_start - part_start;
        
        // 查找 part 的头部和内容的分隔符 "\r\n\r\n"
        const char* part_header_end = static_cast<const char*>(memmem(part_start, part_len, "\r\n\r\n", 4));

        if (part_header_end) {
            std::string part_header(part_start, part_header_end - part_start);
            
            const char* part_body_start = part_header_end + 4;
            // part body 的长度要去掉结尾的 \r\n
            size_t part_body_len = next_boundary_start - part_body_start - 2;

            // 根据 part header 判断是图片还是滤镜类型
            if (part_header.find("name=\"image\"") != std::string::npos) {
                _image_data.assign(part_body_start, part_body_start + part_body_len);
            } else if (part_header.find("name=\"filter\"") != std::string::npos) {
                _filter_type.assign(part_body_start, part_body_len);
            } else if (part_header.find("name=\"uuid\"") != std::string::npos) {
                _image_uuid.assign(part_body_start, part_body_len);
            } else if (part_header.find("name=\"blur_intensity\"") != std::string::npos) {
                _blur_intensity.assign(part_body_start, part_body_len);
            } else if (part_header.find("name=\"sharpen_intensity\"") != std::string::npos) {
                _sharpen_intensity.assign(part_body_start, part_body_len);
            }
        }
        current_pos = next_boundary_start;
    }
}

bool HttpParser::is_request_ready() const {
    return _state == ParseState::COMPLETE;
}

std::string HttpParser::get_method() const {
    return _method;
}

std::string HttpParser::get_path() const {
    return _path;
}

std::vector<char> HttpParser::get_image_data() const {
    return _image_data;
}

std::string HttpParser::get_filter_type() const {
    return _filter_type;
}

std::string HttpParser::get_image_uuid() const {
    return _image_uuid;
}

std::string HttpParser::get_blur_intensity() const {
    return _blur_intensity;
}

std::string HttpParser::get_sharpen_intensity() const {
    return _sharpen_intensity;
}

