#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>

#include "tcp_server.h"
#include "buffer.h"
#include "tcp_conn.h"
#include "timestamp.h"
#include "misc.h"
#include "md5.h"
#include "manager.h"

#include "base64.h"
#include "config_reader.h"
#include "db_base.h"

#include "slothjson.h"
#include "json/dispatcher_msg.h"

using namespace std;
using namespace base;
using namespace net;
using namespace slothjson;
using namespace sqdb;


CDevModel* CDevModel::m_pIns = NULL;

static bool DevSnMd5Check(const string& CheckSN, const DevHashMapPtr& dev) {
	MD5 md5_;
	string md5_string;

	md5_.update(dev->rnd.c_str(), dev->rnd.length());
	md5_.update(dev->sn.c_str(), dev->sn.length());
	md5_.finalize();
	md5_string = md5_.hexdigest();
	LOG_TRACE("sn md5_string:%s",md5_string.c_str());
	if (!strcasecmp(md5_string.c_str(),CheckSN.c_str())) {
		LOG_TRACE("sn md5 verify ok",md5_string.c_str());
		return true;
	}

	LOG_DEBUG("CheckSN:%s,md5:%s,rnd:%s,sn:%s",CheckSN.c_str(),md5_string.c_str(),dev->rnd.c_str(),dev->sn.c_str());
	md5_.init();
	md5_.update(dev->rnd.c_str(), dev->rnd.length());
	md5_.update(dev->ssn.c_str(), dev->ssn.length());
	md5_.finalize();
	md5_string = md5_.hexdigest();
	LOG_TRACE("ssn md5_string:%s",md5_string.c_str());
	if (!strcasecmp(md5_string.c_str(),CheckSN.c_str())) {
		LOG_TRACE("ssn md5 verify ok");
		return true;
	}

	LOG_DEBUG("CheckSN:%s,md5:%s,rnd:%s,ssn:%s",CheckSN.c_str(),md5_string.c_str(),dev->rnd.c_str(),dev->ssn.c_str());
	if (dev->ssid1.length() >= 4) {
		md5_.init();
		md5_.update(dev->rnd.c_str(), dev->rnd.length());
		md5_.update(dev->sn.c_str(), dev->sn.length());
		md5_.update(dev->ssid1.c_str(), dev->ssid1.length());
		md5_.update(dev->ssid1pass.c_str(), dev->ssid1pass.length());
		md5_.update(dev->adminpass.c_str(), dev->adminpass.length());
		md5_.finalize();
		md5_string = md5_.hexdigest();
		LOG_TRACE("sn ssid1 ssid1pass adminpass md5_string:%s",md5_string.c_str());
		if (!strcasecmp(md5_string.c_str(),CheckSN.c_str())) {
			LOG_TRACE("sn ssid1 ssid1pass adminpass md5 verify ok");
			return true;
		}

		LOG_DEBUG("sn:%s,ssid1:%s,ssid1pass:%s,adminpass:%s",
				dev->sn.c_str(),dev->ssid1.c_str(),dev->ssid1pass.c_str(),dev->adminpass.c_str());
		md5_.init();
		md5_.update(dev->rnd.c_str(), dev->rnd.length());
		md5_.update(dev->ssn.c_str(), dev->ssn.length());
		md5_.update(dev->ssid1.c_str(), dev->ssid1.length());
		md5_.update(dev->ssid1pass.c_str(), dev->ssid1pass.length());
		md5_.update(dev->adminpass.c_str(), dev->adminpass.length());
		md5_.finalize();
		md5_string = md5_.hexdigest();
		LOG_TRACE("ssn ssid1 ssid1pass adminpass md5_string:%s",md5_string.c_str());
		if (!strcasecmp(md5_string.c_str(),CheckSN.c_str())) {
			LOG_TRACE("ssn ssid1 ssid1pass adminpass md5 verify ok");
			return true;
		}
		LOG_DEBUG("ssn:%s,ssid1:%s,ssid1pass:%s,adminpass:%s",
				dev->ssn.c_str(),dev->ssid1.c_str(),dev->ssid1pass.c_str(),dev->adminpass.c_str());
	}

	return false;
}

static int create_id_num(string &idnum, int len) {
	struct timeval tv;
	struct tm tm;
	int i = 0;
	char buf_[64] = {0};
	int len_ = 0;	
	base::TrueRandom Random;
	
	if (len < DEV_LOGID_SIZE){
		LOG_ERROR("len too small!!(len = %d)\n", len);
		return -1;
	}

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &tm);
	len_ = sprintf(buf_, "%04u%02u%02u%02u%02u%02u%06lu",tm.tm_year+1900, tm.tm_mon+1, 
		tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec);

	for (i = 0; i < 2; i++) {
		len_ += snprintf(buf_ + len_, len-len_,"%03u", (Random.NextUInt32()%1000));
	}

	idnum = buf_;
	return 0;
}

CDevModel* CDevModel::getIns() {
    if(m_pIns == NULL) {
        m_pIns = new CDevModel();
    }
    return m_pIns;
}	

int32_t CDevModel::GetFindMacVip(const string& mac) { 
	std::lock_guard<std::mutex> lock(MacVipMutex_);
	if (!mac.size()) {
		return config_deviceLogStatus_;
	}
	
	auto it = macVip_.find(mac);
	if (it == macVip_.end()) {
		LOG_TRACE("not find mac:%s,flag:%d",mac.c_str(),config_deviceLogStatus_);
		return config_deviceLogStatus_;
	} else {
		LOG_TRACE("find mac:%s,flag:%d",mac.c_str(),it->second.loglev);
		return it->second.loglev;
	}
}

uint64_t CDevModel::GetPlatcodebyMacdev(const string& mac) { 
	std::lock_guard<std::mutex> lock(DevConfigMutex_);
	
	auto it = DevConfig_.find(mac);
	if (it == DevConfig_.end()) {
		return 0;
	} else {
		return it->second;
	}
}

uint64_t CDevModel::GetIdPerByWeight() { 
	std::lock_guard<std::mutex> lock(IdPerMutex_);
	int sum_weight = 0;
	uint64_t plat_id = 0;
	int sum_percent = 0;
	int r;	
	base::TrueRandom Random;
	size_t i;
	
	sum_weight = IdPer_.size();
	if (sum_weight <= 0) {
		return 0;
	}

	r = Random.NextUInt32()%sum_weight;
	sum_percent = 0;
	LOG_DEBUG("rnd:%d",r);
	for (i = 0; i < IdPer_.size(); i++) {
		if (r < (IdPer_[i].per + sum_percent)) {
			plat_id = IdPer_[i].ID;
			break;
		}
		sum_percent += IdPer_[i].per;
	}

	if (plat_id == 0) {
		LOG_ERROR("no such plat_id");
	}

	LOG_DEBUG("plat_id:%u",plat_id);
	return plat_id;
}

bool CDevModel::GetOpSvcAddrByPlanID(uint64_t PlanID, OpSvcAddr_t& addr) { 
	std::lock_guard<std::mutex> lock(OpSvcAddrMutex_);
	
	auto it = providesId_.find((int32_t)PlanID);
	if (it == providesId_.end()) {
		return false;
	} else {
		addr = it->second;
		return true;
	}
}

bool CDevModel::GetOpSvcAddrByOther(OpSvcAddr_t& addr) {
	static int	 other_port[] = {60002,60012,60022,60032,60042,60052};
	static const char *other_ip[]   = {"101.95.50.81","101.95.50.82"};
	
	base::TrueRandom Random;
	int port_size = sizeof(other_port)/sizeof(other_port[0]);
	int ip_size = sizeof(other_ip)/sizeof(other_ip[0]); 

	addr.svr_addr = other_ip[Random.NextUInt32()%ip_size];
	addr.svr_port = other_port[Random.NextUInt32()%port_size];	
	return true;
}


void CDevModel::GetOpSvcAddrTimer() {
	std::vector<OpSvcAddr_t> objs;
	if (!getOpSvcAddr(objs)) {
		LOG_ERROR("getOpSvcAddr failed");
		return ;
	}

	std::lock_guard<std::mutex> lock(OpSvcAddrMutex_);
	providesId_.clear();
	for (size_t j = 0; j < objs.size(); j++) {
		LOG_DEBUG("svr_provides_id:%d svr_addr:%s svr_port:%d",objs[j].svr_provides_id,objs[j].svr_addr.c_str(),objs[j].svr_port);
		providesId_[objs[j].svr_provides_id] = objs[j];
	}	
	LOG_DEBUG("after providesId_.size:%d",providesId_.size());
}

void CDevModel::GetNetcoreConfigTimer() {
	int isNeedUpdateMac = 0;	
	uint64_t config_DevConfig_time_tmp = 0;
	
	int isNeedUpdatePlatConfig = 0;		
	std::string plat_id_tmp;
	uint64_t config_PlatConfig_time_tmp = 0;
	
	int isNeedUpdateDeviceLogStatus = 0;
	long DeviceLogStatus_tmp = 0;
	
	int isNeedUpdateQosVersion = 0;
	uint64_t qos_wl_time_tmp = 0;
	size_t i = 0;
	
	std::vector<NetcoreConfig_t> objs;
	getNetcoreConfig(objs);

	for (i = 0; i < objs.size(); i++) {
		if (objs[i].param_name == "plat_json") {
			//[{"ID":70000,"per":1},{"ID":60000,"per":1},{"ID":10000,"per":1}] 
			if (config_PlatConfig_time_ != objs[i].param_time) {
				plat_id_tmp = objs[i].param_value; 
				config_PlatConfig_time_tmp = objs[i].param_time; 
				isNeedUpdatePlatConfig = 1;
			}
		}

		if (objs[i].param_name == "netcore_disbymac") {
			if (config_DevConfig_time_ != objs[i].param_time) {
				config_DevConfig_time_tmp = objs[i].param_time;			
 				isNeedUpdateMac = 1;
			}
		}

		if (objs[i].param_name == "deviceLogStatus") {	
			string2l(objs[i].param_value.c_str(), objs[i].param_value.size(), &DeviceLogStatus_tmp);
			if (config_deviceLogStatus_ != DeviceLogStatus_tmp) {
				//LOG_DEBUG("DeviceLogStatus_tmp:%u",DeviceLogStatus_tmp);  
				isNeedUpdateDeviceLogStatus = 1;
			}
		}

		if (objs[i].param_name == "qos_wl_version") {
			if (config_qos_wl_time_ != objs[i].param_time) {
				qos_wl_time_tmp = objs[i].param_time;
				isNeedUpdateQosVersion = 1;
			}
		}
	}

	if (isNeedUpdatePlatConfig) {
		std::vector<slothjson::PlatIdPer_t> perArr;
		if (!slothjson::decode(plat_id_tmp, perArr)) {
			LOG_ERROR("decode id per arr failed");
		}
		else {
			LOG_DEBUG("config_PlatConfig_time_ change to:%u",config_PlatConfig_time_tmp);
			std::lock_guard<std::mutex> lock1(IdPerMutex_);		
			config_PlatConfig_time_ = config_PlatConfig_time_tmp; 	
			IdPer_.clear();
			for (i = 0; i < perArr.size(); i++) {
				LOG_DEBUG("k:%d ID:%u per:%d",i,perArr[i].ID,perArr[i].per);
				IdPer_.push_back(perArr[i]);
			}
		}
	}

	if (isNeedUpdateMac) {
		std::vector<DevConfig_t> dev_objs;
		if (!getNetcoreDisbymac(dev_objs)) {
			LOG_ERROR("getNetcoreDisbymac failed");
		}
		else {
			LOG_DEBUG("config_DevConfig_time_ change to:%u",config_DevConfig_time_tmp);
			std::lock_guard<std::mutex> lock2(DevConfigMutex_);
			config_DevConfig_time_ = config_DevConfig_time_tmp; 			
			DevConfig_.clear();
			for (i = 0; i < dev_objs.size(); i++) {
				LOG_DEBUG("i:%d MAC:%s platcode:%d",i,dev_objs[i].MAC.c_str(),dev_objs[i].platcode);
				DevConfig_[dev_objs[i].MAC] = dev_objs[i].platcode;
			}
		}
	}

	if (isNeedUpdateDeviceLogStatus) {
		std::lock_guard<std::mutex> lock3(MacVipMutex_);
		LOG_DEBUG("deviceLogStatus change to:%u",DeviceLogStatus_tmp);
		config_deviceLogStatus_ = DeviceLogStatus_tmp;
	}

	if (isNeedUpdateQosVersion) {
		std::vector<DispatcherVip_t> vip_objs;
		if (!getMacQosConfig(vip_objs)) {
			LOG_ERROR("getNetcoreDisbymac failed");
		}
		else {
			std::lock_guard<std::mutex> lock3(MacVipMutex_);
			LOG_DEBUG("qos_wl_time change to:%u",qos_wl_time_tmp);
			config_qos_wl_time_ = qos_wl_time_tmp;
			macVip_.clear();
			for (i = 0; i < vip_objs.size(); i++) {
				LOG_DEBUG("i:%d mac:%s loglev:%d",i,vip_objs[i].mac.c_str(),vip_objs[i].loglev);
				macVip_.insert(make_pair(vip_objs[i].mac, vip_objs[i]));
			}			
			LOG_DEBUG("after macVip_.size:%d",macVip_.size());
		}
	}
}

bool CDevModel::getNetcoreDisbymac(std::vector<DevConfig_t> &conf) {
    bool bRet = false;
	CDBManager* pDBManager = CDBManager::getIns();
	CDBConn* pConn = pDBManager->GetDBConn();
	if (!pConn) {
		LOG_ERROR("get conn err"); 
		return bRet;
	}	
	
	string strSql;	  
	strSql = "select MAC,platcode from netcore_disbymac;";	
	CResultSetPtr resSet = pConn->ExecuteQuery(strSql);
	if (resSet) {
		while (resSet->Next()) {
			DevConfig_t devConf;
			devConf.MAC = resSet->GetString("MAC");
			devConf.platcode = resSet->GetInt("platcode");
			conf.push_back(devConf);
			bRet = true;
		}
	}	
	
	pDBManager->RelDBConn(pConn);
	return bRet;
}

bool CDevModel::getNetcoreConfig(std::vector<NetcoreConfig_t> &conf) {
    bool bRet = false;
	CDBManager* pDBManager = CDBManager::getIns();
	CDBConn* pConn = pDBManager->GetDBConn();
	if (!pConn) {
		LOG_ERROR("get conn err"); 
		return bRet;
	}	
	
	string strSql;	  
	strSql = "select * from netcore_config;";	
	CResultSetPtr resSet = pConn->ExecuteQuery(strSql);
	if (resSet) {
		while (resSet->Next()) {
			NetcoreConfig_t Conf;			
			resSet->GetString("param_name",Conf.param_name);
			resSet->GetString("param_value",Conf.param_value);
			Conf.param_time = resSet->GetLong("param_time");
			conf.push_back(Conf);
			bRet = true;
		}
	}	
	
	pDBManager->RelDBConn(pConn);
	return bRet;
}

bool CDevModel::getMacQosConfig(std::vector<DispatcherVip_t> &conf) {
    bool bRet = false;
	CDBManager* pDBManager = CDBManager::getIns();
	CDBConn* pConn = pDBManager->GetDBConn();
	if (!pConn) {
		LOG_ERROR("get conn err"); 
		return bRet;
	}	
	
	string strSql;	  
	strSql = "select * from netcore_logqos_vip;";	
	CResultSetPtr resSet = pConn->ExecuteQuery(strSql);
	if (resSet) {
		while (resSet->Next()) {
			DispatcherVip_t vip;			
			resSet->GetString("mac",vip.mac);
			resSet->GetString("info",vip.info);
			vip.addtime = resSet->GetLong("addtime");
			vip.loglev = resSet->GetInt("loglev");
			conf.push_back(vip);
			bRet = true;
		}
	}	
	
	pDBManager->RelDBConn(pConn);
	return bRet;
}


bool CDevModel::getOpSvcAddr(std::vector<OpSvcAddr_t> &conf) {
    bool bRet = false;
	CDBManager* pDBManager = CDBManager::getIns();
	CDBConn* pConn = pDBManager->GetDBConn();
	if (!pConn) {
		LOG_ERROR("get conn err"); 
		return bRet;
	}	
	
	string strSql;	  
	strSql = "select * from op_svc_addr where INF_TYPE='10';";	
	CResultSetPtr resSet = pConn->ExecuteQuery(strSql);
	if (resSet) {
		while (resSet->Next()) {
			OpSvcAddr_t addr;			
			addr.svr_addr_id = resSet->GetInt("SVC_ADDR_ID");			
			addr.svr_provides_id = resSet->GetInt("SVC_PROVIDERS_ID");
			resSet->GetString("SERVER_ADDR",addr.svr_addr);
			addr.svr_port = resSet->GetInt("SERVER_PORT");
			conf.push_back(addr);
			bRet = true;
		}
	}	
	
	pDBManager->RelDBConn(pConn);
	return bRet;
}


bool CDevModel::getDevSn(const DevHashMapPtr &dev) {
    bool bRet = false;
	CDBManager* pDBManager = CDBManager::getIns();
	CDBConn* pConn = pDBManager->GetDBConn();
	if (!pConn) {
		LOG_ERROR("get conn err"); 
		return bRet;
	}	

	string strSql;	  
	//strSql = "select DEVICE_SN,DEF_SSIDNAME,DEF_SSIDPASS,ADMIN_PASS,DEVICE_SSN,DEVICE_INST_ID,svc_providers_id from gateway_device_inst where MAC_ADDRESS="
	strSql = "select * from gateway_device_inst where MAC_ADDRESS=";
	strSql += "'" + dev->MAC + "'";
	
	CResultSetPtr resSet = pConn->ExecuteQuery(strSql);
	if (resSet) {
		while (resSet->Next()) {
			resSet->GetString("DEVICE_SN",dev->sn);
			resSet->GetString("DEF_SSIDNAME",dev->ssid1);			
			resSet->GetString("DEF_SSIDPASS",dev->ssid1pass);			
			resSet->GetString("ADMIN_PASS",dev->adminpass);			
			resSet->GetString("DEVICE_SSN",dev->ssn);			
		
			dev->dev_id = resSet->GetLong("DEVICE_INST_ID");			
			dev->plan_id = resSet->GetLong("svc_providers_id");			
			bRet = true;
		}
	}	

	LOG_TRACE("RelDBConn");
	pDBManager->RelDBConn(pConn);	
    return bRet;
}

bool CDevModel::getDevAccessRec(const string& mac,uint64_t *plan_id) {
    bool bRet = false;
	CDBManager* pDBManager = CDBManager::getIns();
	CDBConn* pConn = pDBManager->GetDBConn();
	if (!pConn) {
		LOG_ERROR("get conn err"); 
		return bRet;
	}	

	string strSql;	  
	strSql = "select SVC_PROVIDERS_ID from gw_device_access_rec where MAC_ADDRESS='" + mac + "'";
	uint64_t tmp_plan_id = 0;
	
	CResultSetPtr resSet = pConn->ExecuteQuery(strSql);
	if (resSet) {
		while (resSet->Next()) {
			tmp_plan_id = resSet->GetLong("SVC_PROVIDERS_ID");	
			if (plan_id) {
				LOG_TRACE("plan_id:%u",tmp_plan_id); 
				*plan_id = tmp_plan_id;
			}
			bRet = true;
		}
	}	

	LOG_TRACE("RelDBConn");
	pDBManager->RelDBConn(pConn);	
    return bRet;
}

bool CDevModel::updateDevAccessRec(const DevHashMapPtr &dev,uint64_t plan_id,int up_or_insert) {
    bool bRet = false;
	CDBManager* pDBManager = CDBManager::getIns();
	CDBConn* pConn = pDBManager->GetDBConn();
	if (!pConn) {
		LOG_ERROR("get conn err"); 
		return bRet;
	}

	char cmd[2048] = {0};
	char access_time[128] = {0};
	
	uint64_t NowTime = net::Timestamp::Now().Unix();	
	struct tm lt;	
	localtime_r((time_t*)&NowTime, &lt); 
	snprintf(access_time,sizeof(access_time)-1, "%d-%02d-%02d %02d:%02d:%02d",
		(lt.tm_year+1900), (lt.tm_mon + 1), lt.tm_mday,lt.tm_hour, lt.tm_min, lt.tm_sec);

	if (1 == up_or_insert) {		
		snprintf(cmd,sizeof(cmd)-1,"insert into gw_device_access_rec (%s,%s,%s,%s,%s,%s,%s,%s,%s,%s) "
			"values('%lu','%lu','%s','%s','%s','%s','%s','%s','%s','%s');",
			"DEVICE_INST_ID","SVC_PROVIDERS_ID","ACCESS_TIME","update_time","ACCESS_IP","FIRMWARE_VER","HardwareVer","Vendor","Model","mac_address",
			dev->dev_id,plan_id,access_time,access_time,dev->remote.c_str(),
			dev->FirmwareVer.c_str(),dev->HardwareVer.c_str(),dev->Vendor.c_str(),dev->Model.c_str(),dev->MAC.c_str());
	}
	else if (2 == up_or_insert) {
		snprintf(cmd,sizeof(cmd)-1,"update gw_device_access_rec set update_time='%s',SVC_PROVIDERS_ID='%lu',ACCESS_IP='%s' where mac_address='%s'",
			access_time,plan_id,dev->remote.c_str(),dev->MAC.c_str());	
	}
	else {
		LOG_DEBUG("bug fix code\n");
	}

	LOG_TRACE("cmd:%s",cmd); 
	pConn->ExecuteUpdate(cmd);

	LOG_TRACE("RelDBConn");
	pDBManager->RelDBConn(pConn);	
    return bRet;
}

void Server::CreateLoopSqdbClTimer() {
	LOG_TRACE("CreateLoopSqdbClTimer");

	int vm_size_kb = 0;  
	int rss_size_kb = 0;	
	long long CurCpuTime =  GetCurCpuTime();
	long long TotalCpuTime =  GetTotalCpuTime();
	GetCurMemoryUsage(&vm_size_kb, &rss_size_kb);	
	LOG_DEBUG("CurCpuTime:%lu, TotalCpuTime:%lu, vm_size_kb:%dk, rss_size_kb:%dk", CurCpuTime, TotalCpuTime, vm_size_kb, rss_size_kb);
}

void Server::ProcessBootFirst(UDPMessagePtr& msg, const string& requst, slothjson::RPCMethod_t& method) {
	DevHashMapPtr macdev;
	bool rc = false;
	CDevModel* model = CDevModel::getIns();

	slothjson::BootFirstAck_t BootFirstAck;
	std::string BootFirstAck_obj;
	
	slothjson::ResultAck_t ack;
	std::string ack_obj;

	net::Timestamp time_start = net::Timestamp::Now();
	uint64_t nowTime = time_start.Unix();
	
	rc = FindAndNewDev(method.MAC, macdev);
	if (rc == false) {
		macdev->recv_pkts = 1;
	}
	else {
		if (macdev->last_time != nowTime) {
			macdev->recv_pkts = 0;
		} 
		else if (++macdev->recv_pkts > 3) {
			LOG_ERROR("mac:%s pakcet overload!!", method.MAC.c_str());
			ack.Result = -1;
			ack.Ack = "pakcet overload";
			ack.skip_Ack();
			rc = slothjson::encode<false, slothjson::ResultAck_t>(ack, ack_obj);
			if (!rc) {
				LOG_ERROR("encode ResultAck failed");
				return ;
			}
			
			SendMessage(msg->sockfd(), msg->remote_addr(), ack_obj);
			return ;
		}
	}
	
	macdev->MAC = method.MAC;
	macdev->Vendor = method.Vendor;
	macdev->Model = method.MAC;
	macdev->FirmwareVer = method.FirmwareVer;
	macdev->HardwareVer = method.HardwareVer;
	macdev->IPAddr = method.IPAddr;
	macdev->PlatformID = method.PlatformID;
	macdev->remote = sock::ToIPPort(msg->remote_addr());
	if (nowTime - macdev->last_time > DEV_RND_TIMEOUT) {
		base::TrueRandom Random;
		Random.NextBytesBinStr(macdev->rnd, DEV_RDM_SIZE);
		LOG_DEBUG("MAC:%s create rnd:%s",macdev->MAC.c_str(),macdev->rnd.c_str());
	}
	macdev->last_time = nowTime;
	
	BootFirstAck.Result = 0;
	BootFirstAck.ChallengeCode = macdev->rnd;	
	BootFirstAck.Interval = 60;
	BootFirstAck.ServerIP = macdev->IPAddr;
	rc = slothjson::encode<false, slothjson::BootFirstAck_t>(BootFirstAck, BootFirstAck_obj);
	if (!rc) {
		LOG_ERROR("encode BootFirstAck failed");
		return ;
	}
	
	SendMessage(msg->sockfd(), msg->remote_addr(), BootFirstAck_obj);	
	LOG_TRACE("send ok BootFirstAck_obj:%s",BootFirstAck_obj.c_str());

	int macVipFlag = model->GetFindMacVip(macdev->MAC);
	if (!macVipFlag) {
		LOG_DEBUG("mac:%s macVipFlag:%d skip log",macdev->MAC.c_str(),macVipFlag);
		return ;
	}

	net::Timestamp time_end = net::Timestamp::Now();
	slothjson::DispatcherLog_t log;
	string log_obj;

	create_id_num(log.idnum,DEV_LOGID_SIZE);
	log.direction = 3;   //dev to ser
	log.step = 1;
	log.result = 0;
	log.mac = macdev->MAC;
	log.action = "BootFirst";
	log.hostip = GetHostIp();
	log.begintime = time_start.Unix();
	log.begintime_ms = time_start.UnixMs() - time_start.Unix() * 1000;
	log.addtime = time_end.Unix();
	log.addtime_ms = time_end.UnixMs() - time_end.Unix() * 1000;
	log.costtime = time_end.UnixMs() - time_start.UnixMs();
	log.indata = requst;
	log.outdata = BootFirstAck_obj;
	log.userip = macdev->remote;
	rc = slothjson::encode<false, slothjson::DispatcherLog_t>(log, log_obj);
	if (!rc) {
		LOG_ERROR("encode BootFirstAck failed");
		return ;
	}
	
	LOG_TRACE("before insert");
	sqdbPool* pool = sqdbManager::getIns()->GetDbPool();
	if (!pool) {
		LOG_ERROR("get pool err");
		return ;
	}
	
	sqdbCl* cl = pool->GetDbCl("CSpace.manager_log");
	if (!cl) {
		LOG_ERROR("get CSpace.manager_log err");
		return ;
	}
	cl->insert(log_obj);		
	LOG_TRACE("after insert ok:%s",log_obj.c_str());	
}

void Server::ProcessRegisterFirst(UDPMessagePtr& msg, const string& requst, slothjson::RPCMethod_t& method) {
	DevHashMapPtr macdev;
	int err_ack = 0;
	string err_desc;
	bool rc = false;

	uint64_t id = 0;
	uint64_t Platcode = 0;
	
	uint64_t gw_device_access_rec_plan_id = 0;
	int UpdateInsertFlag = 0;	

	int macVipFlag = 0;

	OpSvcAddr_t SvcAddr;
	RegisterFirstAck_t RegisterFirstAck;
	std::string RegisterFirstAck_obj;

	slothjson::ResultAck_t ack;
	std::string ack_obj;
	
	CDevModel* model = CDevModel::getIns();

	net::Timestamp time_start = net::Timestamp::Now();
	net::Timestamp time_end;
	slothjson::DispatcherLog_t log;
	string log_obj;
	sqdbPool* pool = nullptr;
	sqdbCl* cl = nullptr;

	if (!FindDev(method.MAC, macdev)) {
		LOG_ERROR("not find mac:%s,requst:%s",method.MAC.c_str(),requst.c_str());			
		err_ack = -2;
		err_desc = "not find dev";
		goto ERROR;
	}
	
	if (macdev->PlatformID.size()) {			
		LOG_TRACE("MAC:%s,PlatformID:%s",macdev->MAC.c_str(),macdev->PlatformID.c_str());
		if (!strcasecmp(macdev->PlatformID.c_str(),"NULL") || !strcasecmp(macdev->PlatformID.c_str(),"OTHER")) {
			model->GetOpSvcAddrByOther(SvcAddr);
			LOG_DEBUG("MAC:%s svr_addr:%s svr_port:%d",macdev->MAC.c_str(),SvcAddr.svr_addr.c_str(),SvcAddr.svr_port);
			goto RegisterFirst;			
		}
	}
	
	model->getDevSn(macdev);		
	LOG_TRACE("MAC:%s,CheckSN:%s,sn:%s,ssn:%s,dev_id:%lu,plan_id:%lu",
			macdev->MAC.c_str(),method.CheckSN.c_str(),macdev->sn.c_str(),macdev->ssn.c_str(),macdev->dev_id,macdev->plan_id);

	if (!DevSnMd5Check(method.CheckSN,macdev)) {
		LOG_ERROR("mac:%s DevSnMd5Check failed",method.MAC.c_str());			
		err_ack = -2;
		err_desc = "md5 check failed";
		goto ERROR;
	}

	id = macdev->plan_id;
	Platcode = model->GetPlatcodebyMacdev(macdev->MAC);
	if (Platcode > 0 && Platcode != id) {
		LOG_DEBUG("MAC:%s,id change to:%u get by Platcode",macdev->MAC.c_str(),Platcode);
		id = Platcode;
	}

	if (!model->getDevAccessRec(macdev->MAC,&gw_device_access_rec_plan_id)) {
		UpdateInsertFlag = 1;  // insert
		LOG_DEBUG("MAC:%s need insert gw_device_access_rec",macdev->MAC.c_str());
	}
	else {
		LOG_TRACE("getDevAccessRec ok MAC:%s plan_id:%u",macdev->MAC.c_str(),gw_device_access_rec_plan_id); 
		if (id != CNVGC_DB_NO_RECORD && id == gw_device_access_rec_plan_id) {
			UpdateInsertFlag = 0;
			LOG_TRACE("MAC:%s same getDevAccessRec plan_id, skip",macdev->MAC.c_str());
		}
		else {
			LOG_DEBUG("MAC:%s not same to getDevAccessRec plan_id, need Update",macdev->MAC.c_str());
			id = gw_device_access_rec_plan_id;
			UpdateInsertFlag = 2;  
		}
	}
	
	if (id == CNVGC_DB_NO_RECORD) {
		id = model->GetIdPerByWeight();
		LOG_DEBUG("MAC:%s GetIdPerByWeight id to:%u",macdev->MAC.c_str(),id);
	}
	
	if (!model->GetOpSvcAddrByPlanID(id,SvcAddr)) {
		LOG_ERROR("mac:%s no SvcAddr by id:%d", method.MAC.c_str(),id);
		err_ack = -2;
		err_desc = "no SvcAddr by id";		
		goto ERROR;
	}
	
	if (UpdateInsertFlag) {
		model->updateDevAccessRec(macdev, id, UpdateInsertFlag);
	}

RegisterFirst:
	RegisterFirstAck.Result = 0;
	RegisterFirstAck.ServerAddr = SvcAddr.svr_addr;
	RegisterFirstAck.Interval = 60;
	RegisterFirstAck.ServerPort = SvcAddr.svr_port;
	RegisterFirstAck.ServerIP = macdev->IPAddr;
	rc = slothjson::encode<false, slothjson::RegisterFirstAck_t>(RegisterFirstAck, RegisterFirstAck_obj);
	if (!rc) {
		LOG_ERROR("encode RegisterFirstAck failed");
		return ;
	}
	
	SendMessage(msg->sockfd(), msg->remote_addr(), RegisterFirstAck_obj);
	DLOG_TRACE("RegisterFirstAck_obj:%s",RegisterFirstAck_obj.c_str());

	macVipFlag = model->GetFindMacVip(macdev->MAC);
	if (!macVipFlag) {
		LOG_DEBUG("mac:%s macVipFlag:%d skip log",macdev->MAC.c_str(),macVipFlag);
		return ;
	}

	time_end = net::Timestamp::Now();	 
	create_id_num(log.idnum,DEV_LOGID_SIZE);
	log.direction = 3;   //dev to ser
	log.step = 1;
	log.result = 0;
	log.mac = macdev->MAC;
	log.action = "RegisterFirst";
	log.hostip = GetHostIp();
	log.begintime = time_start.Unix();
	log.begintime_ms = time_start.UnixMs() - time_start.Unix() * 1000;
	log.addtime = time_end.Unix();
	log.addtime_ms = time_end.UnixMs() - time_end.Unix() * 1000;
	log.costtime = time_end.UnixMs() - time_start.UnixMs();
	log.indata = requst;
	log.outdata = RegisterFirstAck_obj;
	log.userip = macdev->remote;
	rc = slothjson::encode<false, slothjson::DispatcherLog_t>(log, log_obj);
	if (!rc) {
		LOG_ERROR("encode BootFirstAck failed");
		return ;
	}
	
	LOG_TRACE("before insert");
	pool = sqdbManager::getIns()->GetDbPool();
	if (!pool) {
		LOG_ERROR("get pool err");
		return ;
	}
	
	cl = pool->GetDbCl("CSpace.manager_log");
	if (!cl) {
		LOG_ERROR("get CSpace.manager_log err");
		return ;
	}
	cl->insert(log_obj);		
	LOG_TRACE("after insert ok:%s",log_obj.c_str());	
	return ;

ERROR:
	ack.Result = err_ack;
	ack.Ack = err_desc;
	ack.skip_Ack();
	rc = slothjson::encode<false, slothjson::ResultAck_t>(ack, ack_obj);
	if (!rc) {
		LOG_ERROR("encode ResultAck failed");
		return ;
	}
	
	SendMessage(msg->sockfd(), msg->remote_addr(), ack_obj);
}

void Server::UDPMessageProcess(UDPMessagePtr& msg) {	
	LOG_TRACE("UDPMessageProcess");	
	slothjson::RPCMethod_t method;
	bool rc = false;
	
    std::string requst = msg->NextAllString();
	DLOG_TRACE("requst:%d %s",requst.size(),requst.c_str());
	if (!slothjson::decode(requst, method)) {
		LOG_ERROR("decode method failed,requst:%s",requst.c_str());
		return ;
	}
	
	if (!method.json_has_RPCMethod()) {
		LOG_ERROR("no RPCMethod,requst:%s",requst.c_str());
		return ;
	}

	if (method.RPCMethod == "BootFirst") {		
		ProcessBootFirst(msg,requst,method);
	}
	else if (method.RPCMethod == "RegisterFirst") {
		ProcessRegisterFirst(msg,requst,method);
	}
	else {
		LOG_ERROR("err RPCMethod:%s",method.RPCMethod.c_str());
		slothjson::ResultAck_t ack;
		std::string ack_obj;
		ack.Result = -1;
		//ack.Ack = "RPCMethod err";
		ack.skip_Ack();
		rc = slothjson::encode<false, slothjson::ResultAck_t>(ack, ack_obj);
		if (!rc) {
			LOG_ERROR("encode ResultAck failed");
			return ;
		}		
		SendMessage(msg->sockfd(), msg->remote_addr(), ack_obj);
	}
}


void Server::UDPMessage(EventLoop* loop, UDPMessagePtr& msg) {
    DLOG_TRACE("UDPMessage remote_ipport:%s",sock::ToIPPort(msg->remote_addr()).c_str());
	net::EventLoop* msg_loop = msgPool_->GetNextLoop();
	if (!msg_loop) {
		LOG_ERROR("loop IsEmpty"); 
		return ;
	}	

	msg_loop->RunInLoop(std::bind(&Server::UDPMessageProcess, this, msg));
	DLOG_TRACE("Message:%p,use_count:%d", msg.get(), msg.use_count());
}

void Server::Init() {
    string host = config_reader::ins()->GetString("kserver", "host", "0.0.0.0"); 
    long  port = config_reader::ins()->GetNumber("kserver", "port", 12112);
    short serverThreadNum = config_reader::ins()->GetNumber("kserver", "threadNum", 2);
    
    //short maxConns = config_reader::ins()->GetNumber("kserver", "maxConns", 1024);
	short msgThreadNum = config_reader::ins()->GetNumber("msg", "threadNum", 2);


	LOG_INFO("serverThreadNum:%d  ///////",serverThreadNum);
    serverPool_.reset(new EventLoopThreadPool(base_loop_, serverThreadNum, "server"));
    LOG_INFO("serverPool_ Start //////"); 
    serverPool_->Start(true);

	LOG_INFO("msgThreadNum:%d  ///////",msgThreadNum);  
    msgPool_.reset(new EventLoopThreadPool(base_loop_, msgThreadNum, "msg"));
    LOG_INFO("msgPool_ Start //////"); 
    msgPool_->Start(true);   


	LOG_INFO("bind udp addr:%s %d ///////", host.c_str(), port);
    UDPhostServer_.reset(new UDPServer());	
	UDPhostServer_->SetEventLoopThreadPool(serverPool_);
	UDPhostServer_->SetMessageHandler(std::bind(&Server::UDPMessage, this, std::placeholders::_1, std::placeholders::_2));
    UDPhostServer_->Init(host,port);
    UDPhostServer_->Start();
    host_ = host;

	//LOG_INFO("RunEvery GetQosSwitchTimer 60s /////////");
	//GetQosSwitchTimer();
    //base_loop_->RunEvery(net::Duration(60.0), std::bind(&apLogServer::GetQosSwitchTimer, this));
    //base_loop_->RunEvery(net::Duration(120.0), std::bind(&apLogServer::CreateLoopSqdbClTimer, this));
    
	CDevModel* model = CDevModel::getIns();
	model->GetNetcoreConfigTimer();
	base_loop_->RunEvery(net::Duration(80.0), std::bind(&CDevModel::GetNetcoreConfigTimer, model));	
	model->GetOpSvcAddrTimer();
	base_loop_->RunEvery(net::Duration(120.0), std::bind(&CDevModel::GetOpSvcAddrTimer, model));
}


