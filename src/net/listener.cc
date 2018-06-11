#include "inner_pre.h"
#include "listener.h"
#include "event_loop.h"
#include "fd_channel.h"
#include "libevent.h"
#include "sockets.h"

namespace net {
Listener::Listener(EventLoop* l, const std::string& addr)
    : loop_(l), addr_(addr) {
    DLOG_TRACE("create Listener addr:%s",addr.c_str());
}

Listener::~Listener() {
    DLOG_TRACE("destroy fd:%d",chan_->fd());
    chan_.reset();
    EVUTIL_CLOSESOCKET(fd_);
    fd_ = INVALID_SOCKET;
}

void Listener::Listen(int backlog) {
    DLOG_TRACE("Listen backlog:%d",backlog);
    fd_ = sock::CreateNonblockingSocket();
    if (fd_ < 0) {
        int serrno = errno;
        LOG_FATAL("Create a nonblocking socket failed %s",strerror(serrno).c_str());
        return;
    }

    struct sockaddr_storage addr = sock::ParseFromIPPort(addr_.data());
    // TODO Add retry when failed
    int ret = ::bind(fd_, sock::sockaddr_cast(&addr), static_cast<socklen_t>(sizeof(struct sockaddr)));
    if (ret < 0) {
        int serrno = errno;
        LOG_FATAL("bind error:%d %s addr=%s",serrno,strerror(serrno).c_str(), addr_.c_str());
    }

    ret = ::listen(fd_, backlog);
    if (ret < 0) {
        int serrno = errno;
        LOG_FATAL("Listen error:%d %s",serrno,strerror(serrno).c_str());
    }
}

void Listener::Accept() {
    DLOG_TRACE("Listener Accept fd_:%d",fd_);
    chan_.reset(new FdChannel(loop_, fd_, true, false));
    chan_->SetReadCallback(std::bind(&Listener::HandleAccept, this));
    loop_->RunInLoop(std::bind(&FdChannel::AttachToLoop, chan_.get()));
    LOG_INFO("TCPServer is running at fd_:%d %s",fd_, addr_.c_str());
}

void Listener::HandleAccept() {
    assert(loop_->IsInLoopThread());
    struct sockaddr_storage ss;
    socklen_t addrlen = sizeof(ss);
    int nfd = -1;
    if ((nfd = ::accept(fd_, sock::sockaddr_cast(&ss), &addrlen)) == -1) {
        int serrno = errno;
        if (serrno != EAGAIN && serrno != EINTR) {
            LOG_WARN("bad accept %s",strerror(serrno).c_str());
        }
        return;
    }

    DLOG_TRACE("A new connection is comming in nfd:%d",nfd);
    if (evutil_make_socket_nonblocking(nfd) < 0) {
        LOG_ERROR("set fd=%s nonblocking failed.",nfd);
        EVUTIL_CLOSESOCKET(nfd);
        return;
    }

    sock::SetKeepAlive(nfd, true);

    std::string raddr = sock::ToIPPort(&ss);
    if (raddr.empty()) {
        LOG_ERROR("sock::ToIPPort(&ss) failed.");
        EVUTIL_CLOSESOCKET(nfd);
        return;
    }

    DLOG_TRACE("accepted a connection from %s, listen fd=%d, client fd=%d",raddr.c_str(),fd_,nfd);

    if (new_conn_fn_) {
	    DLOG_TRACE("new_conn_fn_ call");
        new_conn_fn_(nfd, raddr, sock::sockaddr_in_cast(&ss));
    }
}

void Listener::Stop() {
    assert(loop_->IsInLoopThread());
    chan_->DisableAllEvent();
    chan_->Close();
}
}
