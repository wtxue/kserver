#pragma once

#include <map>

#include "net/inner_pre.h"
#include "net/event_loop.h"
#include "net/slice.h"

struct evhttp_request;
namespace net {
namespace httpc {
class Request;
class Response {
public:
    typedef std::map<net::Slice, net::Slice> Headers;
#if defined(HTTP_CLIENT_SUPPORTS_SSL)
    Response(Request* r, struct evhttp_request* evreq, bool had_ssl_error = false);
#else
    Response(Request* r, struct evhttp_request* evreq);
#endif
    ~Response();

    int http_code() const {
        return http_code_;
    }
#if defined(HTTP_CLIENT_SUPPORTS_SSL)
    bool had_ssl_error() const {
        return had_ssl_error_;
    }
#endif
    const net::Slice& body() const {
        return body_;
    }
    const Request* request() const {
        return request_;
    }
    const char* FindHeader(const char* key);
private:
    Request* request_;
    struct evhttp_request* evreq_;
    int http_code_;
#if defined(HTTP_CLIENT_SUPPORTS_SSL)
    bool had_ssl_error_;
#endif
    net::Slice body_;
};
}
}
