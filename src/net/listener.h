#pragma once

#include "net/inner_pre.h"
#include "net/timestamp.h"

namespace net {
class EventLoop;
class FdChannel;

class Listener {
public:
	/*remote address with format "ip:port"*/
	/*remote address*/
    typedef std::function<void(net_socket_t sockfd,const std::string&, const struct sockaddr_in*)> NewConnectionCallback;
    Listener(EventLoop* loop, const std::string& addr/*local listening address : ip:port*/);
    ~Listener();

    // socket listen
    void Listen(int backlog = SOMAXCONN);

    // nonblocking accept
    void Accept();

    void Stop();

    void SetNewConnectionCallback(NewConnectionCallback cb) {
        new_conn_fn_ = cb;
    }

private:
    void HandleAccept();

private:
    net_socket_t fd_ = -1;// The listening socket fd
    EventLoop* loop_;
    std::string addr_;
    std::unique_ptr<FdChannel> chan_;
    NewConnectionCallback new_conn_fn_;
};
}


