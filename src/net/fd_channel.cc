#include "net/inner_pre.h"

#include <string.h>

#include "net/fd_channel.h"
#include "net/libevent.h"
#include "net/event_loop.h"

namespace net {
static_assert(FdChannel::kReadable == EV_READ, "");
static_assert(FdChannel::kWritable == EV_WRITE, "");

FdChannel::FdChannel(EventLoop* l, net_socket_t f, bool r, bool w)
    : loop_(l), attached_(false), event_(nullptr), fd_(f) {
    assert(fd_ > 0);
    events_ = (r ? kReadable : 0) | (w ? kWritable : 0);
    event_ = new event;
    
    DLOG_TRACE("create fd:%d event_:%p events_:%s", fd_, event_, EventsToString().c_str());
    memset(event_, 0, sizeof(struct event));
}

FdChannel::~FdChannel() {
	DLOG_TRACE("destroy fd:%d", fd_);
    assert(event_ == nullptr);
}

void FdChannel::Close() {
    DLOG_TRACE("close chan fd:%d", fd_);
    assert(event_);
    if (event_) {
        assert(!attached_);
        if (attached_) {
            EventDel(event_);
        }

		DLOG_TRACE("delete event_:%p", event_);
        delete (event_);
        event_ = nullptr;
    }
    read_fn_ = ReadEventCallback();
    write_fn_ = EventCallback();
}

void FdChannel::AttachToLoop() {
    assert(!IsNoneEvent());
    assert(loop_->IsInLoopThread());

    if (attached_) {
        // FdChannel::Update may be called many times
        // So doing this can avoid event_add will be called more than once.
        DetachFromLoop();
    }

    assert(!attached_);
    ::event_set(event_, fd_, events_ | EV_PERSIST,
                &FdChannel::HandleEvent, this);
    ::event_base_set(loop_->event_base(), event_);
    DLOG_TRACE("add fd:%d Events:%s event_:%p evbase_:%p",fd_,EventsToString().c_str(),event_,loop_->event_base());
    if (EventAdd(event_, nullptr) == 0) {
        DLOG_TRACE("fd:%d add watching ok",fd_);
        attached_ = true;
    } else {
        LOG_ERROR("this:%p fd:%d with event:%s attach to event loop failed",this,fd_,EventsToString().c_str());
    }
}

void FdChannel::EnableReadEvent() {
    int events = events_;
    events_ |= kReadable;

    if (events_ != events) {
        Update();
    }
}

void FdChannel::EnableWriteEvent() {
    int events = events_;
    events_ |= kWritable;

    if (events_ != events) {
        Update();
    }
}

void FdChannel::DisableReadEvent() {
    int events = events_;
    events_ &= (~kReadable);

    if (events_ != events) {
        Update();
    }
}

void FdChannel::DisableWriteEvent() {
    int events = events_;
    events_ &= (~kWritable);
    if (events_ != events) {
        Update();
    }
}

void FdChannel::DisableAllEvent() {
    if (events_ == kNone) {
        return;
    }

    events_ = kNone;
    Update();
}

void FdChannel::DetachFromLoop() {
    assert(loop_->IsInLoopThread());
    assert(attached_);

    DLOG_TRACE("del evevt fd_:%d",fd_);
    if (EventDel(event_) == 0) {
        attached_ = false;
        DLOG_TRACE("fd:%d del detach from event_loop ok",fd_);
    } else {
        LOG_ERROR("DetachFromLoop this:%p fd:%d  with event:%s detach from event loop failed",this,fd_,EventsToString().c_str());
    }
}

void FdChannel::Update() {
    assert(loop_->IsInLoopThread());

    if (IsNoneEvent()) {
        DetachFromLoop();
    } else {
        AttachToLoop();
    }
}

std::string FdChannel::EventsToString() const {
    std::string s;

    if (events_ & kReadable) {
        s = "kReadable";
    }

    if (events_ & kWritable) {
        if (!s.empty()) {
            s += "|";
        }

        s += "kWritable";
    }

    return s;
}

void FdChannel::HandleEvent(net_socket_t sockfd, short which, void* v) {
    FdChannel* c = (FdChannel*)v;
    c->HandleEvent(sockfd, which);
}

void FdChannel::HandleEvent(net_socket_t sockfd, short which) {
    assert(sockfd == fd_);

    if ((which & kReadable) && read_fn_) {
        read_fn_();
    }

    if ((which & kWritable) && write_fn_) {
		DLOG_TRACE("sockfd:%d Events:%s",sockfd,EventsToString().c_str());
        write_fn_();
    }
}
}
