#pragma once


#include <atomic>
#include <vector>
#include "event_loop_thread.h"

using namespace std;

namespace net {

class EventLoopThread;
class EventLoopThreadPool;

typedef std::shared_ptr<EventLoopThreadPool> EventLoopThreadPoolPtr;
typedef std::shared_ptr<EventLoopThread>     EventLoopThreadPtr;

class EventLoopThreadPool : public ServerStatus {
public:
    typedef std::function<void()> DoneCallback;

    EventLoopThreadPool(EventLoop* base_loop, uint32_t threadNum);
	
    EventLoopThreadPool(EventLoop* base_loop, uint32_t threadNum, const std::string& name);
	
    ~EventLoopThreadPool();

    bool Start(bool wait_thread_started = false);

    void Stop(bool wait_thread_exited = false);
    void Stop(DoneCallback fn);

    // @brief Join all the working thread. If you forget to call this method,
    // it will be invoked automatically in the destruct method ~EventLoopThreadPool().
    // @note DO NOT call this method from any of the working thread.
    void Join();

    // @brief Reinitialize some data fields after a fork
    void AfterFork();
public:
    EventLoop* GetNextLoop();
    EventLoop* GetNextLoopWithHash(uint64_t hash);
    uint32_t thread_num() const;
	std::string& GetName()   {
		return name_;
	}

private:
    void Stop(bool wait_thread_exit, DoneCallback fn);
    void OnThreadStarted(uint32_t count);
    void OnThreadExited(uint32_t count);

private:
    EventLoop* base_loop_;
	std::string name_;
    uint32_t thread_num_ = 0;
    std::atomic<int64_t> next_ = { 0 };

    DoneCallback stopped_cb_;
    std::vector<EventLoopThreadPtr> threads_;
};
}
