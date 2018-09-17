#include <stdio.h>
#include <stdlib.h>

#include <event_loop_thread.h>

#include <httpc/conn_pool.h>
#include <httpc/request.h>
#include <httpc/conn.h>
#include <httpc/response.h>

static bool responsed = false;
static void HandleHTTPResponse(const std::shared_ptr<net::httpc::Response>& response, net::httpc::Request* request) {
    LOG_INFO("http_code:%d [%s]", response->http_code(), response->body().ToString().c_str());
    std::string header = response->FindHeader("Connection");
    LOG_INFO("HTTP HEADER Connection:%s",header.c_str());
    responsed = true;
    assert(request == response->request());
    delete request; // The request MUST BE deleted in EventLoop thread.
}

int main() {
    LOG_INIT(NULL, NULL, true, TRACE);

    net::EventLoopThread t;
    t.Start(true);

#if 0    
    net::httpc::GetRequest* r = new net::httpc::GetRequest(t.loop(), "http://www.so.com/status.html", net::Duration(2.0));
    LOG_INFO("Do http request");
    r->Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, r));
#else
    net::httpc::PostRequest* r = new net::httpc::PostRequest(t.loop(), 
    "https://author.189cube.com:443/author/index.json", 
    "{\"id\":\"20\",\"jsonrpc\":\"2.0\",\"method\":\"getSSN\",\"params\":{\"mac\":\"F09838D666FF\"}}",
    net::Duration(2.0));
    LOG_INFO("Do http request");
    //r->AddHeader("Content-Type", "application/json");
    r->Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, r));
#endif
    while (!responsed) {
        usleep(1);
    }

	t.Stop(true);
    LOG_INFO("EventLoopThread stopped.");
    return 0;
}