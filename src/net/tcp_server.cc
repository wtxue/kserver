#include "inner_pre.h"
#include "tcp_server.h"
#include "listener.h"
#include "tcp_conn.h"
#include "libevent.h"

namespace net {
TCPServer::TCPServer(EventLoop* loop,
                     const std::string& laddr,
                     const std::string& name,
                     uint32_t thread_num)
    : loop_(loop)
    , listen_addr_(laddr)
    , name_(name)
    , conn_fn_(&internal::DefaultConnectionCallback)
    , msg_fn_(&internal::DefaultMessageCallback)
    , next_conn_id_(0) {
    DLOG_TRACE("name=%s listening addr:%s thread_num=%d",name.c_str(), laddr.c_str(), thread_num);
    tpool_.reset(new EventLoopThreadPool(loop_, thread_num));
}

TCPServer::TCPServer(EventLoop* loop,	  
					const std::string& laddr, 
					const std::string& name, 
					const std::shared_ptr<EventLoopThreadPool>& tpool)
		: loop_(loop)
		, listen_addr_(laddr)
		, name_(name)
		, tpool_(tpool) 
		, conn_fn_(&internal::DefaultConnectionCallback)
		, msg_fn_(&internal::DefaultMessageCallback)
		, next_conn_id_(0) {
	DLOG_TRACE("name=%s laddr:%s tpool=%p",name.c_str(), laddr.c_str(), tpool.get());
}

TCPServer::~TCPServer() {
	DLOG_TRACE("destroy TCPServer");
    assert(connections_.empty());
    assert(!listener_);
    if (tpool_) {
        assert(tpool_->IsStopped());
        tpool_.reset();
    }
}

bool TCPServer::Init() {
	DLOG_TRACE("TCPServer Init");
    assert(status_ == kNull);
    listener_.reset(new Listener(loop_, listen_addr_));
    listener_->Listen();
    status_.store(kInitialized);
    return true;
}

void TCPServer::AfterFork() {
    tpool_->AfterFork();
}

bool TCPServer::Start() {
	DLOG_TRACE("TCPServer Start");
    assert(status_ == kInitialized);
    status_.store(kStarting);
    assert(listener_.get());
    
    bool rc = tpool_->Start(true);
    if (rc) {
		DLOG_TRACE("TCPServer Start HandleNewConn");
        assert(tpool_->IsRunning());
        listener_->SetNewConnectionCallback(
            std::bind(&TCPServer::HandleNewConn,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      std::placeholders::_3));

        // We must set status_ to kRunning firstly and then we can accept new
        // connections. If we use the following code :
        //     listener_->Accept();
        //     status_.store(kRunning);
        // there is a chance : we have accepted a connection but status_ is not
        // kRunning that will cause the assert(status_ == kRuning) failed in
        // TCPServer::HandleNewConn.        
        status_.store(kRunning);
        listener_->Accept();
    }
    return rc;
}

void TCPServer::Stop(DoneCallback on_stopped_cb) {
    DLOG_TRACE("Stop ...");
    assert(status_ == kRunning);
    status_.store(kStopping);
    substatus_.store(kStoppingListener);
    loop_->RunInLoop(std::bind(&TCPServer::StopInLoop, this, on_stopped_cb));
}

void TCPServer::StopInLoop(DoneCallback on_stopped_cb) {
    DLOG_TRACE("StopInLoop ...");
    assert(loop_->IsInLoopThread());
    listener_->Stop();
    listener_.reset();

    if (connections_.empty()) {
        // Stop all the working threads now.
        DLOG_TRACE("no connections");
        StopThreadPool();
        if (on_stopped_cb) {
            on_stopped_cb();
            on_stopped_cb = DoneCallback();
        }
        status_.store(kStopped);
    } else {
        DLOG_TRACE("close connections");
        for (auto& c : connections_) {
            if (c.second->IsConnected()) {
                DLOG_TRACE("close connection id=%lu fd=%d",c.second->id(),c.second->fd());
                c.second->Close();
            } else {
                DLOG_TRACE("Do not need to call Close for this TCPConn it may be doing disconnecting. TCPConn=%d fd=%d status=%s",c.second.get(),c.second->fd(),StatusToString().c_str());
            }
        }

        stopped_cb_ = on_stopped_cb;

        // The working threads will be stopped after all the connections closed.
    }

    DLOG_TRACE("exited, status=%s" ,StatusToString().c_str());
}

void TCPServer::StopThreadPool() {
    DLOG_TRACE("pool=%p",tpool_.get());
    assert(loop_->IsInLoopThread());
    assert(IsStopping());
    substatus_.store(kStoppingThreadPool);
    tpool_->Stop(true);
    assert(tpool_->IsStopped());

    // Make sure all the working threads totally stopped.
    tpool_->Join();
    tpool_.reset();

    substatus_.store(kSubStatusNull);
}

void TCPServer::HandleNewConn(net_socket_t sockfd,     const std::string& remote_addr,const struct sockaddr_in* raddr) {
    DLOG_TRACE("fd=%d",sockfd);
    assert(loop_->IsInLoopThread());
    if (IsStopping()) {
        LOG_WARN("this=%p The server is at stopping status. Discard this socket fd=%d remote_addr=%s",this, sockfd, remote_addr.c_str());
        EVUTIL_CLOSESOCKET(sockfd);
        return;
    }

    assert(IsRunning());
    EventLoop* io_loop = GetNextLoop(raddr);
#ifdef H_DEBUG_MODE
    std::string n = name_ + "-" + remote_addr + "#" + std::to_string(next_conn_id_);
#else
    std::string n = remote_addr;
#endif
    ++next_conn_id_;    
    DLOG_TRACE("fd=%d name:%s",sockfd,n.c_str());
    TCPConnPtr conn(new TCPConn(io_loop, n, sockfd, listen_addr_, remote_addr, next_conn_id_));
    assert(conn->type() == TCPConn::kIncoming);
    conn->SetMessageCallback(msg_fn_);
    conn->SetConnectionCallback(conn_fn_);
    conn->SetCloseCallback(std::bind(&TCPServer::RemoveConnection, this, std::placeholders::_1));
    io_loop->RunInLoop(std::bind(&TCPConn::OnAttachedToLoop, conn));    
    DLOG_TRACE("new conn:%p %d id:%d",conn.get(),conn.use_count(),conn->id());    
    connections_[conn->id()] = conn;
}

EventLoop* TCPServer::GetNextLoop(const struct sockaddr_in* raddr) {
    if (IsRoundRobin()) {
        return tpool_->GetNextLoop();
    } else {
        return tpool_->GetNextLoopWithHash(raddr->sin_addr.s_addr);
    }
}

void TCPServer::RemoveConnection(const TCPConnPtr& conn) {
    DLOG_TRACE("conn use_count:%d fd:%d connections:%d",conn.use_count(),conn->fd(),connections_.size());
    auto f = [this, conn]() {
        // Remove the connection in the listening EventLoop
		DLOG_TRACE("conn use_count:%d fd:%d",conn.use_count(),conn->fd());
        assert(this->loop_->IsInLoopThread());
        this->connections_.erase(conn->id());
		DLOG_TRACE("conn use_count:%d",conn.use_count());
        if (IsStopping() && this->connections_.empty()) {
            // At last, we stop all the working threads
            DLOG_TRACE("stop thread pool");
            assert(substatus_.load() == kStoppingListener);
            StopThreadPool();
            if (stopped_cb_) {
                stopped_cb_();
                stopped_cb_ = DoneCallback();
            }
            status_.store(kStopped);
        }
    };
    loop_->RunInLoop(f);
}

}
