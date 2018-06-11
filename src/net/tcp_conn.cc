#include "net/inner_pre.h"

#include "net/libevent.h"

#include "net/tcp_conn.h"
#include "net/fd_channel.h"
#include "net/event_loop.h"
#include "net/sockets.h"
#include "net/invoke_timer.h"

namespace net {
TCPConn::TCPConn(EventLoop* l,
                 const std::string& n,
                 net_socket_t sockfd,
                 const std::string& laddr,
                 const std::string& raddr,
                 uint64_t conn_id)
    : loop_(l)
    , fd_(sockfd)
    , id_(conn_id)
    , name_(n)
    , local_addr_(laddr)
    , remote_addr_(raddr)
    , type_(kIncoming)
    , status_(kDisconnected) {
    if (sockfd >= 0) {
        chan_.reset(new FdChannel(l, sockfd, false, false));
        chan_->SetReadCallback(std::bind(&TCPConn::HandleRead, this));
        chan_->SetWriteCallback(std::bind(&TCPConn::HandleWrite, this));        
    }

    DLOG_TRACE("create TCPConn[%s] channel=%p fd=%d addr=%s", name_.c_str(), chan_.get(), sockfd,AddrToString().c_str());
}

TCPConn::~TCPConn() {
	DLOG_TRACE("destroy TCPConn[%s] channel=%p addr=%s", name_.c_str(), chan_.get(), AddrToString().c_str());
    assert(status_ == kDisconnected);

    if (fd_ >= 0) {
		//DLOG_TRACE("enter destroy fd_:%d",fd_);
        assert(chan_);
        assert(fd_ == chan_->fd());
        assert(chan_->IsNoneEvent());
        EVUTIL_CLOSESOCKET(fd_);
        fd_ = INVALID_SOCKET;        
		//DLOG_TRACE("leave destroy fd");
    }

    assert(!delay_close_timer_.get());
}

void TCPConn::Close() {
    DLOG_TRACE("fd=%d status=%s addr=%s",fd_,StatusToString().c_str(),AddrToString().c_str());
    status_ = kDisconnecting;
    auto c = shared_from_this();
    auto f = [c]() {
        assert(c->loop_->IsInLoopThread());
        c->HandleClose();
    };

    // Use QueueInLoop to fix TCPClient::Close bug when the application delete TCPClient in callback
    loop_->QueueInLoop(f);
}

void TCPConn::Send(const std::string& d) {
    if (status_ != kConnected) {
        return;
    }

    if (loop_->IsInLoopThread()) {
        SendInLoop(d);
    } else {
        loop_->RunInLoop(std::bind(&TCPConn::SendStringInLoop, shared_from_this(), d));
    }
}

void TCPConn::Send(const Slice& message) {
    if (status_ != kConnected) {
        return;
    }

    if (loop_->IsInLoopThread()) {
        SendInLoop(message);
    } else {
        loop_->RunInLoop(std::bind(&TCPConn::SendStringInLoop, shared_from_this(), message.ToString()));
    }
}

void TCPConn::Send(const void* data, size_t len) {
    if (loop_->IsInLoopThread()) {
        SendInLoop(data, len);
        return;
    }
    Send(Slice(static_cast<const char*>(data), len));
}

void TCPConn::Send(Buffer* buf) {
    if (status_ != kConnected) {
        return;
    }

    if (loop_->IsInLoopThread()) {
        SendInLoop(buf->data(), buf->length());
        buf->Reset();
    } else {
        loop_->RunInLoop(std::bind(&TCPConn::SendStringInLoop, shared_from_this(), buf->NextAllString()));
    }
}

void TCPConn::SendInLoop(const Slice& message) {
    SendInLoop(message.data(), message.size());
}

void TCPConn::SendStringInLoop(const std::string& message) {
    SendInLoop(message.data(), message.size());
}

void TCPConn::SendInLoop(const void* data, size_t len) {
    assert(loop_->IsInLoopThread());

    if (status_ == kDisconnected) {
        LOG_WARN("disconnected, give up writing");
        return;
    }

    ssize_t nwritten = 0;
    size_t remaining = len;
    bool write_error = false;

    // if no data in output queue, writing directly
    if (!chan_->IsWritable() && output_buffer_.length() == 0) {
        nwritten = ::send(chan_->fd(), static_cast<const char*>(data), len, MSG_NOSIGNAL);
        if (nwritten >= 0) {
            remaining = len - nwritten;
            if (remaining == 0 && write_complete_fn_) {
                loop_->QueueInLoop(std::bind(write_complete_fn_, shared_from_this()));
            }
        } else {
            int serrno = errno;
            nwritten = 0;
            if (!EVUTIL_ERR_RW_RETRIABLE(serrno)) {
                LOG_ERROR("SendInLoop write failed errno=%d %s",serrno,strerror(serrno).c_str());
                if (serrno == EPIPE || serrno == ECONNRESET) {
                    write_error = true;
                }
            }
        }
    }

    if (write_error) {
        HandleError();
        return;
    }

    assert(!write_error);
    assert(remaining <= len);

    if (remaining > 0) {
        size_t old_len = output_buffer_.length();
        if (old_len + remaining >= high_water_mark_
                && old_len < high_water_mark_
                && high_water_mark_fn_) {
            loop_->QueueInLoop(std::bind(high_water_mark_fn_, shared_from_this(), old_len + remaining));
        }

        output_buffer_.Append(static_cast<const char*>(data) + nwritten, remaining);

        if (!chan_->IsWritable()) {
            chan_->EnableWriteEvent();
        }
    }
}

void TCPConn::HandleRead() {
    assert(loop_->IsInLoopThread());
    int serrno = 0;
    ssize_t n = input_buffer_.ReadFromFD(chan_->fd(), &serrno);
    if (n > 0) {
        msg_fn_(shared_from_this(), &input_buffer_);
    } else if (n == 0) {
        if (type() == kOutgoing) {
            // This is an outgoing connection, we own it and it's done. so close it
            DLOG_TRACE("fd=%d  We read 0 bytes and close the socket.",fd_);
            status_ = kDisconnecting;
            HandleClose();
        } else {
            // Fix the half-closing problem : https://github.com/chenshuo/muduo/pull/117

            chan_->DisableReadEvent();
            if (close_delay_.IsZero()) {
                DLOG_TRACE("channel (fd=%d) DisableReadEvent. We close this connection immediately",chan_->fd());
                DelayClose();
            } else {
                // This is an incoming connection, we need to preserve the
                // connection for a while so that we can reply to it.
                // And we set a timer to close the connection eventually.
                DLOG_TRACE("channel (fd=%d) DisableReadEvent. And a timer to delay close this TCPConn, time:%fs",chan_->fd(),close_delay_.Seconds());
                delay_close_timer_ = loop_->RunAfter(close_delay_, std::bind(&TCPConn::DelayClose, shared_from_this())); // TODO leave it to user layer close.
            }
        }
    } else {
        if (EVUTIL_ERR_RW_RETRIABLE(serrno)) {
            DLOG_TRACE("errno=%d ",serrno);
        } else {
            DLOG_TRACE("errno=%d We are closing this connection now.",serrno);
            HandleError();
        }
    }
}

void TCPConn::HandleWrite() {
    assert(loop_->IsInLoopThread());
    assert(!chan_->attached() || chan_->IsWritable());

    ssize_t n = ::send(fd_, output_buffer_.data(), output_buffer_.length(), MSG_NOSIGNAL);
    if (n > 0) {
        output_buffer_.Next(n);

        if (output_buffer_.length() == 0) {
            chan_->DisableWriteEvent();

            if (write_complete_fn_) {
                loop_->QueueInLoop(std::bind(write_complete_fn_, shared_from_this()));
            }
        }
    } else {
        int serrno = errno;

        if (EVUTIL_ERR_RW_RETRIABLE(serrno)) {
            LOG_WARN("this=%p TCPConn::HandleWrite errno=%d %s",this,serrno,strerror(serrno).c_str());
        } else {
            HandleError();
        }
    }
}

void TCPConn::DelayClose() {
    assert(loop_->IsInLoopThread());
    DLOG_TRACE("ENTER addr=%s fd=%d status=%s",AddrToString().c_str(),fd_,StatusToString().c_str());
    status_ = kDisconnecting;
    delay_close_timer_.reset();
    HandleClose();
}

void TCPConn::HandleClose() {
    //DLOG_TRACE("ENTER addr=%s fd=%d status=%s",AddrToString().c_str(),fd_,StatusToString().c_str());

    // Avoid multi calling
    if (status_ == kDisconnected) {
    	DLOG_TRACE("may multi call HandleClose");
        return;
    }

    // We call HandleClose() from TCPConn's method, the status_ is kConnected
    // But we call HandleClose() from out of TCPConn's method, the status_ is kDisconnecting
    assert(status_ == kDisconnecting);

    status_ = kDisconnecting;
    assert(loop_->IsInLoopThread());
    DLOG_TRACE("chan:%p DisableAllEvent Close",chan_.get());
    chan_->DisableAllEvent();
    chan_->Close();

    TCPConnPtr conn(shared_from_this());

    if (delay_close_timer_) {
        DLOG_TRACE("loop=%p Cancel the delay closing timer.",this);
        delay_close_timer_->Cancel();
        delay_close_timer_.reset();
    }

    if (conn_fn_) {
        // This callback must be invoked at status kDisconnecting
        // e.g. when the TCPClient disconnects with remote server,
        // the user layer can decide whether to do the reconnection.
        assert(status_ == kDisconnecting);
        conn_fn_(conn);
    }

    if (close_fn_) {
        close_fn_(conn);
    }
    DLOG_TRACE("addr=%s fd=%d status=%s use_count=%d",AddrToString().c_str(),fd_,StatusToString().c_str(),conn.use_count());
    status_ = kDisconnected;
}

void TCPConn::HandleError() {
    DLOG_TRACE("fd=%d status=%s",fd_,StatusToString().c_str());
    status_ = kDisconnecting;
    HandleClose();
}

void TCPConn::OnAttachedToLoop() {
    DLOG_TRACE("TCPConn OnAttachedToLoop");
    assert(loop_->IsInLoopThread());
    status_ = kConnected;
    chan_->EnableReadEvent();

    if (conn_fn_) {
        conn_fn_(shared_from_this());
    }
}

void TCPConn::SetHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t mark) {
    high_water_mark_fn_ = cb;
    high_water_mark_ = mark;
}

void TCPConn::SetTCPNoDelay(bool on) {
    sock::SetTCPNoDelay(fd_, on);
}

std::string TCPConn::StatusToString() const {
    H_CASE_STRING_BIGIN(status_.load());
    H_CASE_STRING(kDisconnected);
    H_CASE_STRING(kConnecting);
    H_CASE_STRING(kConnected);
    H_CASE_STRING(kDisconnecting);
    H_CASE_STRING_END();
}
}
