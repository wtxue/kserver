#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <list>
#include <unordered_map>

#include <time.h>
#include "timestamp.h"
#include "event_loop.h"
#include "udp_server.h"
#include "tcp_server.h"
#include "tcp_callbacks.h"

#include "mysqlPool.h"

#include "slothjson.h"
#include "json/dispatcher_msg.h"

using namespace std;
using namespace base;
using namespace net;
using namespace slothjson;

struct DevMap_t;
typedef std::shared_ptr<slothjson::DevMap_t> DevHashMapPtr;
typedef std::unordered_map<std::string, DevHashMapPtr> DevHashMap;
//typedef std::map<std::string, DevHashMapPtr> DevHashMap;

#define DEV_LOGID_SIZE 32
#define DEV_RDM_SIZE 32
#define CNVGC_DB_NO_RECORD      10
#define DEV_RND_TIMEOUT        130

class Server {
public:
    Server(net::EventLoop* loop) 
		: base_loop_(loop)
		, next_conn_id_(0) {
		LOG_TRACE("create apLogServer");  
	}
		 
	~Server() {
		DLOG_TRACE("destroy apLogServer ");
	}

    void Init();

	string GetHostIp() {
		return host_;
	}

	void CreateLoopSqdbClTimer();

	void UDPMessage(EventLoop* loop, UDPMessagePtr& msg); 
	void UDPMessageProcess(UDPMessagePtr& msg);
	void ProcessBootFirst(UDPMessagePtr& msg, const string& requst, slothjson::RPCMethod_t& method);
	void ProcessRegisterFirst(UDPMessagePtr& msg, const string& requst, slothjson::RPCMethod_t& method);


	bool FindAndNewDev(const string& mac, DevHashMapPtr& macdev) {	
		std::lock_guard<std::mutex> lock(mutex_);
		
		auto it = devMap_.find(mac);
		if (it == devMap_.end()) {
			DevHashMapPtr new_macdev(new slothjson::DevMap_t());
			devMap_.insert(std::pair<std::string, DevHashMapPtr>(mac, new_macdev));
			macdev = new_macdev;
			LOG_DEBUG("insert mac:%s size:%u",mac.c_str(),devMap_.size());
			return false;
		} else {
			macdev = it->second;			
			return true;
		}
	}
	
	bool FindDev(const string& mac, DevHashMapPtr& macdev) {	
		std::lock_guard<std::mutex> lock(mutex_);
		
		auto it = devMap_.find(mac);
		if (it == devMap_.end()) {
			return false;
		} else {
			macdev =  it->second;
			return true;
		}
	}

	void EraseDev(const string& mac) {	
		std::lock_guard<std::mutex> lock(mutex_);		
		devMap_.erase(mac);
	}

	uint64_t GetDevMapNum() {	
		std::lock_guard<std::mutex> lock(mutex_);		
		return devMap_.size();
	}


private:
	net::EventLoop* base_loop_;	
	net::UDPServerPtr UDPhostServer_;
	net::EventLoopThreadPoolPtr serverPool_;	
	net::EventLoopThreadPoolPtr msgPool_;

	string host_;
	
    uint64_t next_conn_id_;
    typedef std::map<uint64_t, TCPConnPtr> ConnectionMap;
    ConnectionMap conns_;


	DevHashMap devMap_;
	std::mutex mutex_;
};


class CDevModel {
public:
    static CDevModel* getIns();
    CDevModel() {
		config_deviceLogStatus_ = 2;
		config_qos_wl_time_ = 0;
		config_DevConfig_time_ = 0;
		config_PlatConfig_time_ = 0;
		LOG_DEBUG("create CDevModel default config_deviceLogStatus_:%u",config_deviceLogStatus_);
    }
    ~CDevModel() {
    }
	bool getDevSn(const DevHashMapPtr &dev);
	bool getMacQosConfig(std::vector<DispatcherVip_t> &conf);
	bool getNetcoreDisbymac(std::vector<DevConfig_t> &conf);
	bool getNetcoreConfig(std::vector<NetcoreConfig_t> &conf);
	bool getOpSvcAddr(std::vector<OpSvcAddr_t> &conf);
	bool getDevAccessRec(const string& mac,uint64_t *plan_id);
	bool updateDevAccessRec(const DevHashMapPtr &dev,uint64_t plan_id,int up_or_insert);
	
	void GetNetcoreConfigTimer();
	void GetOpSvcAddrTimer();

	int32_t GetFindMacVip(const string& mac);
	uint64_t GetPlatcodebyMacdev(const string& mac);
	uint64_t GetIdPerByWeight();
	bool GetOpSvcAddrByPlanID(uint64_t PlanID, OpSvcAddr_t& addr);
	bool GetOpSvcAddrByOther(OpSvcAddr_t& addr);


private:
    static CDevModel* m_pIns;	

	// key:mac val:platcode
	std::map<string, uint64_t> DevConfig_;
	uint64_t config_DevConfig_time_;	
	std::mutex DevConfigMutex_;

	// key:ID(plat_id) val:per   exp:"ID":70000,"per":1
	std::vector<PlatIdPer_t> IdPer_;
	uint64_t config_PlatConfig_time_;
	std::mutex IdPerMutex_;

	// key:svr_provides_id   val:SvcAddr
	std::map<int32_t, OpSvcAddr_t> providesId_;	
	std::mutex OpSvcAddrMutex_;

	// key:mac val:vip
	typedef std::map<string, DispatcherVip_t> MacVipMap;	
	MacVipMap macVip_;
	int32_t  config_deviceLogStatus_;
	uint64_t config_qos_wl_time_;
	std::mutex MacVipMutex_;
};


