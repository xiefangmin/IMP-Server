#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <string>
#include <vector>
#include <unordered_map>

enum class ParseState {
    METHOD,
    PATH,
    VERSION,
    HEADERS,
    BODY,
    COMPLETE
};

class HttpParser {
public:
    HttpParser();

    void parse(const char* data, size_t len);
    void reset(); // 添加重置方法

    bool is_request_ready() const;
    std::string get_method() const;
    std::string get_path() const;
    std::vector<char> get_image_data() const;
    std::string get_filter_type() const;
    std::string get_image_uuid() const;
    std::string get_blur_intensity() const;
    std::string get_sharpen_intensity() const;

private:
    void parse_headers();
    void parse_multipart_body();

    ParseState _state;
    std::string _buffer;
    std::string _method;
    std::string _path;
    std::unordered_map<std::string, std::string> _headers;
    std::vector<char> _body;
    size_t _content_length;

    // 用于 multipart/form-data
    std::string _boundary;
    std::vector<char> _image_data;
    std::string _filter_type;
    std::string _image_uuid;
    std::string _blur_intensity;
    std::string _sharpen_intensity;
};

#endif // HTTP_PARSER_H
