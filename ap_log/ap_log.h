#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>

#include "event_loop.h"
#include "tcp_server.h"
#include "slothjson.h"
#include "json/ap_log_msg.h"
#include "json/ap_log_db.h"

using namespace std;
using namespace base;
using namespace net;
using namespace slothjson;

namespace ap_log {

class MsgString;
typedef std::shared_ptr<MsgString> MsgStringPtr;

class MsgString  : public std::enable_shared_from_this<MsgString> {
public:
    MsgString(EventLoop* loop, const net::TCPConnPtr& conn, const char *start, const char *end) 
		: loop_(loop) 
		, conn_(conn)
		, msg_(start, end) {	
		DLOG_TRACE("create MsgString conn name:%s",conn->name().c_str());
	}

	~MsgString() {
		DLOG_TRACE("destroy MsgString ");
	}

    std::string& GetMsg()    {
        return msg_;
    }

    net::TCPConnPtr& GetConn()    {
        return conn_;
    }

private:
	net::EventLoop* loop_;
	net::TCPConnPtr conn_;
	std::string msg_;
};


class apLogServer {
public:
    apLogServer(net::EventLoop* loop);   
	~apLogServer();

    void Init();

	void DropLoopSqdbClTimer();
	void GetQosSwitchTimer();
	void CreateLoopSqdbClTimer();

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

    void OnConnection(const net::TCPConnPtr& conn);
    void OnMessage(const net::TCPConnPtr& conn, net::Buffer* msg);
    void OnMessageProcess(MsgStringPtr &Message);
	
	void Send(const net::TCPConnPtr& conn,const std::string &res);
	void Send(uint64_t conn_id,const std::string &res);

private:
	net::EventLoop* base_loop_;	
	net::TCPServerPtr apiServer_;
	net::TCPServerPtr harborServer_;	
	net::EventLoopThreadPoolPtr serverPool_;	
	net::EventLoopThreadPoolPtr msgPool_;

    uint64_t next_conn_id_;
    typedef std::map<uint64_t, TCPConnPtr> ConnectionMap;
    ConnectionMap conns_;

	typedef std::map<string, std::shared_ptr<ap_log_qos_vip_t>> MacVipMap;	
	std::mutex mutex_;
	MacVipMap macVip_;
	uint64_t config_deviceLogStatus_;
    uint64_t config_qos_wl_time_;
};

}

