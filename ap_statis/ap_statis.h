#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <unordered_map>

#include "event_loop.h"
#include "tcp_server.h"
#include "slothjson.h"
#include "json/ap_statis_msg.h"

using namespace std;
using namespace base;
using namespace net;
using namespace slothjson;


#define LOG_LOOP_CHECK_HEAD  "ap_statis"

class MsgString;
typedef std::shared_ptr<MsgString>     MsgStringPtr;

class MacHash_t;
typedef std::shared_ptr<MacHash_t>      MacHashPtr;
typedef std::list<MacHashPtr>           MacHashList;
typedef std::unordered_map<std::string, MacHashPtr> MacHashMap;

class ConnPriv;
typedef std::shared_ptr<ConnPriv>      ConnPrivPtr;

class ApDevHash_t;
typedef std::shared_ptr<ApDevHash_t>      ApDevHashPtr;
typedef std::unordered_map<std::string, ApDevHashPtr> ApDevHashMap;


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

class MacHash_t {
public:
	MacHash_t() {
	}

	~MacHash_t() {
		//APP_DEBUG("destroy DevHashMap");
	}

	string ToApDevKey() {
		return province + Vendor + Model + FirmwareVer;
	}

    std::string MAC;
    std::string Vendor;
    std::string Model;
    std::string FirmwareVer;
    std::string HardwareVer;
    std::string province;
};

class ApDevHash_t {
public:
	ApDevHash_t() {
	}

	~ApDevHash_t() {
		//APP_DEBUG("destroy DevHashMap");
	}

    std::string key;
	uint64_t online;
	uint64_t dev_sum;
};


class ConnPriv  : public std::enable_shared_from_this<ConnPriv> {
public:
    ConnPriv(uint64_t fd,uint64_t id) {
		fd_ = fd;
		id_ = id;
	}

	~ConnPriv() {
		DLOG_TRACE("destroy ConnPriv size:%u",list_.size());		
		std::lock_guard<std::mutex> lock(mutex_);
		for (auto it = list_.begin(); it != list_.end();it++) {
			MacHashPtr m = *it;
			DLOG_TRACE("MAC:%s,ad:%p,use_count:%d",m->MAC.c_str(),m.get(),m.use_count());
			list_.erase(it);	
		}		 
	}

	void Add(MacHashPtr& dev) {
		std::lock_guard<std::mutex> lock(mutex_);
		list_.push_back(dev);
	}

	void UpdateLive(uint64_t live_sec) {
		live_sec_ = live_sec_;
	}

public:
	uint64_t fd_;	
	uint64_t id_;

private:	
	uint64_t live_sec_;	
	bool enable_;
	MacHashList list_;
	std::mutex mutex_;
};

class Server {
public:
    Server(net::EventLoop* loop);   
	~Server();

    void Init();

	void DropLoopSqdbClTimer();
	void GetQosSwitchTimer();
	void CreateLoopSqdbClTimer();

    void OnConnection(const net::TCPConnPtr& conn);
    void OnMessage(const net::TCPConnPtr& conn, net::Buffer* msg);
    void OnMessageProcess(MsgStringPtr &Message);

	bool FindAndNewMac(const string& mac, ConnPrivPtr& ConnPriv,MacHashPtr& macdev) {	
		std::lock_guard<std::mutex> lock(mutex_);
		
		auto it = MacMap_.find(mac);
		if (it == MacMap_.end()) {
			MacHashPtr dev = make_shared<MacHash_t>();

			dev->MAC = mac;	
			MacMap_.insert(std::pair<std::string, MacHashPtr>(dev->MAC, dev));
			ConnPriv->Add(dev);
			
			macdev = dev;
			APP_DEBUG("insert mac:%s size:%u",dev->MAC.c_str(),MacMap_.size());
			return false;
		} else {
			macdev = it->second;
			return true;
		}
	}

	bool FindAndNewApDev(const string& key, ApDevHashPtr& ApDev) {	
		std::lock_guard<std::mutex> lock(ApDevMutex_);
		
		auto it = ApDevMap_.find(key);
		if (it == ApDevMap_.end()) {
			ApDevHashPtr dev = make_shared<ApDevHash_t>();

			dev->key = key;	
			ApDevMap_.insert(std::pair<std::string, ApDevHashPtr>(dev->key, dev));
				
			ApDev = dev;
			APP_DEBUG("insert key:%s size:%u",dev->key.c_str(),ApDevMap_.size());
			return false;
		} else {
			ApDev = it->second;
			return true;
		}
	}

private:
	net::EventLoop* base_loop_;	
	net::TCPServerPtr harborServer_;	
	net::EventLoopThreadPoolPtr serverPool_;	
	net::EventLoopThreadPoolPtr msgPool_;

	std::set<net::TCPConnPtr> conns_;

	std::mutex mutex_;
	MacHashMap MacMap_;

	std::mutex ApDevMutex_;
	ApDevHashMap ApDevMap_;
};


