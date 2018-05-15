#include "http_client.h"

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

namespace http {

#define USER_AGENT "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/64.0.3282.119 Safari/537.36"

static size_t http_content_callback(char *data, size_t size, size_t nmemb,
        void *p) {
    Response* response = (Response *) p;
    response->body().append(data, size * nmemb);
    return size * nmemb;
}

static size_t http_header_callback(char *data, size_t size, size_t nmemb,
        void *p) {

    size_t len = size * nmemb, n = len;

    while (n > 0 && std::isspace(data[n - 1])) {
        n--;
    }

    if (n == 0) {
        return len;
    }

    std::vector<std::string> &headers = ((Response*) p)->headers();

    if (n > 4 && data[0] == 'H' && data[1] == 'T' && data[2] == 'T'
            && data[3] == 'P' && data[4] == '/') {
        headers.clear();
        return len;
    }

    if (std::isspace(data[0])) {
        if (headers.size() > 0) {
            size_t i = 0;
            while (std::isspace(data[i])) {
                i++;
            }
            std::string &value = headers.back();
            if (value.size() > 0) {
                value.push_back(' ');
            }
            value.append(data + i, n - i);
        }
        return len;
    }

    std::string key, value;
    size_t i = 0;

    while (i < n && data[i] != ':' && !std::isspace(data[i])) {
        i++;
    }

    key.append(data, i);

    while (i < n && (data[i] == ':' || std::isspace(data[i]))) {
        i++;
    }

    value.append(data + i, n - i);

    headers.push_back(key);
    headers.push_back(value);

    return len;
}

extern "C" const char *find_user_agent_string();

Client::Client(const char *proxy) : curl_(NULL), slist_(NULL) {
    curl_ = curl_easy_init();

    if (curl_ != NULL) {
        curl_easy_setopt(curl_, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 30L);
        curl_easy_setopt(curl_, CURLOPT_COOKIEFILE, "");
        curl_easy_setopt(curl_, CURLOPT_ERRORBUFFER, error_);
        curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 0L);
        curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, http_header_callback);
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl_, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1);
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 300L);
        curl_easy_setopt(curl_, CURLOPT_USERAGENT, USER_AGENT);
        curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, http_content_callback);

        if (proxy) {
            curl_easy_setopt(curl_, CURLOPT_PROXY, proxy);
        }

        set_header("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
        set_header("Accept-Language", "en-US,en;q=0.5");
        set_header("Cache-Control", "max-age=0");
        set_header("Connection", "close");
    }
}

Client::~Client() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
    if (slist_) {
        curl_slist_free_all(slist_);
    }
}

void Client::set_header(std::string key, std::string value) {
    if (value.size() > 0) {
        key += ": ";
        key += value;
    }
    slist_ = curl_slist_append(slist_, key.c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, slist_);
}

void Client::clear_header() {
    if (slist_) {
        curl_slist_free_all (slist_);
        slist_ = NULL;
    }
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, NULL);
}

void Client::set_proxy(const char* proxy) {
    proxy_ = proxy && proxy[0] ? proxy : "";
    curl_easy_setopt(curl_, CURLOPT_PROXY, proxy_.c_str());
}

void Client::set_user_agent(const char * name) {
    curl_easy_setopt(curl_, CURLOPT_USERAGENT, name);
}

void Client::set_timeout(size_t value) {
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, value);
}

void Client::set_timeout(size_t value, size_t speed) {
    curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_TIME, value);
    curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_LIMIT, speed);
}

void Client::set_follow_location(bool value) {
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, (long)value);
}

void Client::set_verbose(bool on) {
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, on ? 1L : 0L);
}

Response *Client::request(const char *url, Method method, const char *data,
        size_t length) {

    curl_easy_setopt(curl_, CURLOPT_URL, url);

    switch (method) {
    case GET:
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
        break;
    case POST:
        curl_easy_setopt(curl_, CURLOPT_POST, 1L);
        break;
    default:
        throw -1;
    }

    if (data) {
        curl_easy_setopt(curl_, CURLOPT_POST, 1L);
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, (long ) length);
    }

    Response *res = new Response(url);

    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, res);
    curl_easy_setopt(curl_, CURLOPT_HEADERDATA, res);

    if ((curl_code_ = curl_easy_perform(curl_)) == CURLE_OK) {
        char *url = NULL;
        curl_easy_getinfo(curl_, CURLINFO_EFFECTIVE_URL, &url);
        if (url) {
            res->url_ = url;
        }
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &res->status_code_);
        return res;
    }

    delete res;

    return NULL;
}


}
