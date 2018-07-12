#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>

#include "event_loop.h"

#include "thread_dispatch_policy.h"

#include <tcp_server.h>
#include "slothjson.h"
#include "json/ap_log_msg.h"
#include "json/ap_log_db.h"


using namespace std;
using namespace base;
using namespace net;
using namespace slothjson;


namespace ap_log {

class MsgBuffer;
class EventLoopQueue;

typedef std::shared_ptr<MsgBuffer> MsgBufferPtr;
typedef std::shared_ptr<EventLoopQueue> EventLoopQueuePtr;
typedef std::function<void(const MsgBufferPtr&)> MsgProcessCallback;

class EventLoopQueue {
public:
	EventLoopQueue(uint32_t id)
		: id_(id) {		
		DLOG_TRACE("create EventLoopQueue id:%d",id);
	}

	~EventLoopQueue() {
		DLOG_TRACE("destroy EventLoopQueue");
	}

    void PopOne(MsgBufferPtr& item)   {		
		std::unique_lock<std::mutex> mlock(mutex_);
		if (!queue_.empty()) {	
			item = queue_.front();			
			queue_.pop();	
			DLOG_TRACE("pendingCount_:%d after swap queue_.size:%d",pendingCount_,queue_.size());		
			pendingCount_--;
		}
		else {
			LOG_ERROR("maybe bug");
		}
    }

    void PopAll(std::queue<MsgBufferPtr>& msgs)   {
		std::lock_guard<std::mutex> mlock(mutex_);		
		if (!queue_.empty()) {	
			std::swap(msgs, queue_);
			DLOG_TRACE("pendingCount_:%d after swap queue_.size:%d all.size:%d",pendingCount_,queue_.size(),msgs.size());			
			pendingCount_ = 0;			
		}
		else {
			LOG_ERROR("maybe bug");
		}
    }

	void Push(const MsgBufferPtr& item) {
		std::lock_guard<std::mutex> lock(mutex_);
		queue_.push(item);
		pendingCount_++;
		Count_++;
	}

	void Push(MsgBufferPtr&& item) {
		std::lock_guard<std::mutex> lock(mutex_);
		queue_.push(std::move(item));
		pendingCount_++;
		Count_++;
	}

	uint64_t PendingCount() {
		std::lock_guard<std::mutex> lock(mutex_);
		return pendingCount_;
	}

private:
	uint32_t id_;

	std::mutex mutex_;
	std::queue<MsgBufferPtr> queue_;

	uint64_t pendingCount_;
	uint64_t Count_;
	//std::atomic<long> pendingCount_  = { 0 };
	//std::atomic<long> Count_  = { 0 };
};


class EventLoopQueuePool : public ThreadDispatchPolicy {
public:	
	EventLoopQueuePool() {
		DLOG_TRACE("create EventLoopQueuePool");
	}
		
	~EventLoopQueuePool() {
		DLOG_TRACE("destroy EventLoopQueuePool");
	}

	void Init(net::EventLoopThreadPoolPtr tpool) {
		DLOG_TRACE("EventLoopQueuePool Init");
		msgPool_ = tpool;
		DLOG_TRACE("thread_num:%d",msgPool_->thread_num());
		for (uint32_t i = 0; i < msgPool_->thread_num(); ++i) {
			net::EventLoop* loop_ = msgPool_->GetNextLoopWithHash(i);			
			EventLoopQueuePtr Queue(new EventLoopQueue(i));			
			//DLOG_TRACE("new Queue:%p use_count:%d id:%d",Queue.get(),Queue.use_count(),i);	
			msgPoolQueue_.push_back(Queue);
			//DLOG_TRACE("new Queue:%p use_count:%d id:%d",Queue.get(),Queue.use_count(),i);			
			//msgPoolQueue_.emplace_back(EventLoopQueue<MsgBufferPtr>(loop_,i));		
			loop_->set_context(net::Any(msgPoolQueue_[i]));			
			//DLOG_TRACE("new Queue:%p use_count:%d id:%d",Queue.get(),Queue.use_count(),i);	
		}	
	}

    bool GetEventLoopQueue(net::EventLoop* loop, EventLoopQueuePtr& queue) {
		if (loop->context().IsEmpty()) {
			LOG_DEBUG("loop context IsEmpty"); 
			return false; 
		}
		else {			 
	        queue = net::any_cast<EventLoopQueuePtr>(loop->context());
			return true;
		}
    }

	EventLoop* GetNextMsgLoop(uint64_t hash) {
	    if (IsRoundRobin()) {
	        return msgPool_->GetNextLoop();
	    } else {
	        return msgPool_->GetNextLoopWithHash(hash);
	    }
	}

private:	
	net::EventLoopThreadPoolPtr msgPool_;	
    std::vector<EventLoopQueuePtr> msgPoolQueue_;
};

class MsgBuffer  : public std::enable_shared_from_this<MsgBuffer> {
public:
    MsgBuffer(EventLoop* loop, uint64_t conn_id) 
		: loop_(loop) 
		, conn_id_(conn_id) {	
		DLOG_TRACE("create MsgBuffer conn_id:%d",conn_id);
	}

    MsgBuffer(EventLoop* loop, uint64_t conn_id, const string& name) 
		: loop_(loop) 
		, conn_id_(conn_id)
		, conn_name_(name){	
		DLOG_TRACE("create MsgBuffer conn_id:%d",conn_id);
	}

	~MsgBuffer() {
		//DLOG_TRACE("destroy MsgBuffer ");
	}

	void SetMsgProcessCallback(MsgProcessCallback cb) {
		msg_cb_ = cb;
	}

	uint64_t GetId() {
		return conn_id_;
	}

    const std::string& GetConnName()    {
        return conn_name_;
    }

	slothjson::ap_log_cmd_t& GetMsg() {
		return msg_;
	}

	void GetMsg(slothjson::ap_log_cmd_t*& msg) {
		msg = &msg_;
	}

private:
    net::EventLoop* loop_;
    uint64_t conn_id_;
	std::string conn_name_;
	
	MsgProcessCallback msg_cb_; 
    //net::TCPServer* tcp_server_;
	
	
    net::Duration timeout_;

    //net::InvokeTimerPtr timer_;

    //bool  timer_canceled_;
    //bool  con_timer_canceled_;

	slothjson::ap_log_cmd_t msg_;
};


class apLogServer {
public:
    apLogServer(net::EventLoop* loop);   
	~apLogServer();

    void Init();

	void GetQosSwitchTimer();
	void CreateLoopSqdbClTimer();

	TCPConnPtr& GetConn(uint64_t conn_id) {
		ConnectionMap::iterator it = conns_.find(conn_id);
		if (it == conns_.end()) {
			LOG_TRACE("not find conn_id:%u",conn_id);
		} else {
			return it->second;
		}
	}

	uint64_t GetFindMacVip(const string& mac) {	
		std::lock_guard<std::mutex> lock(mutex_);
		if (!mac.size()) {
			return config_deviceLogStatus_;
		}
		
		MacVipMap::iterator it = macVip_.find(mac);
		if (it == macVip_.end()) {
			//LOG_TRACE("not find mac:%s",mac.c_str());
			return config_deviceLogStatus_;
		} else {
			return it->second->loglev;
		}
	}

private:
    void OnConnection(const net::TCPConnPtr& conn);
    void OnMessage(const net::TCPConnPtr& conn, net::Buffer* msg);

    void OnMessageProcess(net::EventLoop* loop);
	
	void Send(const net::TCPConnPtr& conn,const std::string &res);
	void Send(uint64_t conn_id,const std::string &res);


	net::EventLoop* base_loop_;
	
	net::TCPServerPtr apiServer_;
	net::TCPServerPtr harborServer_;
	
	net::EventLoopThreadPoolPtr serverPool_;
	
	net::EventLoopThreadPoolPtr msgPool_;
	EventLoopQueuePool msgQueuePool_;


    uint64_t next_conn_id_ = 0;
    typedef std::map<uint64_t, TCPConnPtr> ConnectionMap;
    ConnectionMap conns_;

	typedef std::map<string, std::shared_ptr<ap_log_qos_vip_t>> MacVipMap;	
	std::mutex mutex_;
	MacVipMap macVip_;
	uint64_t config_deviceLogStatus_;
    uint64_t config_qos_wl_time_;
};

}

