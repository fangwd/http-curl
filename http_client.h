#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

#include <string>
#include <vector>

#include <curl/curl.h>

namespace http {

class Response {
private:
    std::string url_;                   // CURLINFO_EFFECTIVE_URL
    int status_code_;                   // CURLINFO_RESPONSE_CODE
    std::vector<std::string> headers_;
    std::string body_;

    friend class Client;

public:
    Response(const char *url) : url_(url), status_code_(0) {}

    int status_code() const { return status_code_; }

    std::string& url() { return url_; }
    std::string& body() { return body_; }
    std::vector<std::string>& headers() { return headers_; }
};

enum Method {
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE,
    PATCH,
};

class Client {
private:
    CURL *curl_;
    struct curl_slist *slist_;
    char error_[CURL_ERROR_SIZE];   // CURLOPT_ERRORBUFFER
    CURLcode curl_code_;

    std::string proxy_;

public:
    Client(const char *proxy = NULL);
    ~Client();

    const char *error() const { return error_; }
    int error_code() const { return (int) curl_code_; }

    void set_header(std::string key, std::string value);
    void clear_header();

    void set_proxy(const char *proxy);
    void set_user_agent(const char *name);

    void set_verbose(bool);

    const char *proxy() const { return proxy_.c_str(); }

    Response *request(const char *, Method = GET, const char * = NULL, size_t =
            0);
};

}

#endif /* HTTP_CLIENT_H_ */
