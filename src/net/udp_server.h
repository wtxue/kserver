#pragma once

#include "net/inner_pre.h"
#include "net/thread_dispatch_policy.h"
#include "event_loop_thread_pool.h"
#include "udp_message.h"

#include <thread>

namespace net {

class EventLoop;

class UDPServer: public ThreadDispatchPolicy {
public:
    typedef std::function<void(EventLoop*, UDPMessagePtr&)> UDPMessageHandler;
public:
    UDPServer();
    ~UDPServer();

    bool Init(int port);
    bool Init(const std::vector<int>& ports);
	bool Init(const std::string& HostPort);  
	bool Init(const std::vector<std::string>& HostPorts);

    bool Start();
    void Stop(bool wait_thread_exit);

    void Pause();
    void Continue();

    // @brief Reinitialize some data fields after a fork
    void AfterFork();

    bool IsRunning() const;
    bool IsStopped() const;

    void SetMessageHandler(UDPMessageHandler handler) {
        message_handler_ = handler;
    }

    void SetEventLoopThreadPool(const EventLoopThreadPoolPtr& pool) {
        tpool_ = pool;
    }

    void set_recv_buf_size(size_t v) {
        recv_buf_size_ = v;
    }

private:
    class RecvThread;
    typedef std::shared_ptr<RecvThread> RecvThreadPtr;
    std::vector<RecvThreadPtr> recv_threads_;

    UDPMessageHandler   message_handler_;

    // The worker thread pool, used to process UDP package
    // This data field is not owned by UDPServer,
    // it is set by outer application layer.
    EventLoopThreadPoolPtr tpool_;

    // The buffer size used to receive an UDP package.
    // The minimum size is 1472, maximum size is 65535. Default : 1472
    // We can increase this size to receive a larger UDP package
    size_t recv_buf_size_;
private:
    void RecvingLoop(RecvThread* th);
};


}

