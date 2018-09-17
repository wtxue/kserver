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

#include "IPLocator.h"
#include "mysqlPool.h"

#include "slothjson.h"
#include "json/dispatcher_msg.h"

using namespace std;
using namespace base;
using namespace net;
using namespace slothjson;


class DevHashMap_t;
typedef std::shared_ptr<DevHashMap_t> DevHashMapPtr;
typedef std::weak_ptr<DevHashMap_t> WeakDevHashMapPtr;
typedef std::list<WeakDevHashMapPtr> WeakDevHashMapList;
typedef std::list<WeakDevHashMapPtr>::iterator WeakDevHashMapItar;


typedef std::unordered_map<std::string, DevHashMapPtr> DevHashMap;
//typedef std::map<std::string, DevHashMapPtr> DevHashMap;


#define DEV_LOGID_SIZE  32
#define DEV_RDM_SIZE    32
#define DEV_RND_TIMEOUT 130
#define LOG_LOOP_CHECK_HEAD  "dist"

class DevHashMap_t {
public:
	DevHashMap_t() {
	}

	~DevHashMap_t() {
		//APP_DEBUG("destroy DevHashMap");
	}

    std::string MAC;
    std::string Vendor;
    std::string Model;
    std::string FirmwareVer;
    std::string HardwareVer;
    std::string IPAddr;
    std::string PlatformID;
    std::string remote;
    std::string rnd;

    std::string sn;
    std::string ssn;
    std::string ssid1;
    std::string ssid1pass;
    std::string adminpass;
    uint64_t dev_id;
    uint64_t plan_id;
	int Prov_id;
	int City_id;

    uint64_t recv_pkts;
    uint64_t last_time;
	WeakDevHashMapItar pos;
};


class Server {
public:
    Server(net::EventLoop* loop) 
		: base_loop_(loop)
		, next_conn_id_(0) {
		statOk.store(0);
		statOkE8c.store(0);
		statErrSn.store(0);
		statErrMd5.store(0);	
	}
		 
	~Server() {
	}

    void Init();

	string GetHostIp() {
		return host_;
	}

	void StatLoopTimer();
	void UpdateLogLoopTimer();
	void LogLoopTimer();
	void CreateLoopTimer();

	void UDPMessage(EventLoop* loop, UDPMessagePtr& msg); 
	void UDPMessageProcess(UDPMessagePtr& msg);
	void ProcessBootFirst(UDPMessagePtr& msg, slothjson::RPCMethod_t& method);
	void ProcessRegisterFirst(UDPMessagePtr& msg, slothjson::RPCMethod_t& method);
	void WriteLog(const string& log);

	bool FindAndNewDev(const string& mac, DevHashMapPtr& macdev) {	
		std::lock_guard<std::mutex> lock(mutex_);
		
		auto it = devMap_.find(mac);
		if (it == devMap_.end()) {
			DevHashMapPtr dev = make_shared<DevHashMap_t>();

			dev->MAC = mac;	
			devMap_.insert(std::pair<std::string, DevHashMapPtr>(dev->MAC, dev));
		
		    DevHashMapList_.push_back(dev);
		    dev->pos = --DevHashMapList_.end();	
			
			macdev = dev;
			//APP_DEBUG("insert mac:%s size:%u",mac.c_str(),devMap_.size());
			return false;
		} else {
			macdev = it->second;
			DevHashMapList_.splice(DevHashMapList_.end(), DevHashMapList_, macdev->pos);
			if (macdev->pos != --DevHashMapList_.end())
				APP_DEBUG("maybe bug");
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
			DevHashMapList_.splice(DevHashMapList_.end(), DevHashMapList_, macdev->pos);
			if (macdev->pos != --DevHashMapList_.end())
				APP_DEBUG("maybe bug");
			return true;
		}
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

	int64_t idleSeconds_ = (10*60);
	WeakDevHashMapList DevHashMapList_;

	std::atomic<uint64_t> statOk;	
	std::atomic<uint64_t> statOkE8c;
	std::atomic<uint64_t> statErrSn;	
	std::atomic<uint64_t> statErrMd5;
};


class CDevModel {
public:
    static CDevModel* getIns();
    CDevModel() {
		config_deviceLogStatus_ = 0;
		config_qos_wl_time_ = 0;
		config_DevConfig_time_ = 0;
		config_PlatConfig_time_ = 0;
		all_per_ = 0;
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
	uint32_t all_per_;
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


