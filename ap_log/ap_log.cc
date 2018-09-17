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

#include "base64.h"
#include "config_reader.h"
#include "db_base.h"
#include "ap_log.h"

#include "slothjson.h"
#include "json/ap_log_msg.h"
#include "json/ap_log_db.h"

using namespace std;
using namespace base;
using namespace net;
using namespace slothjson;
using namespace sqdb;

namespace ap_log {

apLogServer::apLogServer(net::EventLoop* loop) {
	base_loop_ = loop;
    next_conn_id_ = 0;
	config_deviceLogStatus_ = 2;
    config_qos_wl_time_ = 0;
}

apLogServer::~apLogServer() {
}

void apLogServer::DropLoopSqdbClTimer() {
	std::vector<std::string> objs;	
	slothjson::cs_name_t name_;

	LOG_TRACE("DropLoopSqdbClTimer");
	sqdbPool* pool = sqdbManager::getIns()->GetDbPool();
	if (!pool) {	
		LOG_ERROR("get pool err");
		return ;
	}
	
	int rc = pool->GetList(SDB_LIST_COLLECTIONSPACES, objs);
	if (rc <= 0) {
		LOG_ERROR("GetList err rc:%d",rc);
		return ;
	}

	char *start;
	uint64_t parse_time = 0;
	char Name[128] = {0};
	uint64_t now_time = net::Timestamp::Now().Unix();
	for (size_t i = 0; i < objs.size(); i++) {
		rc = slothjson::decode(objs[i], name_);
		if (!rc) {
			LOG_ERROR("decode cl name err");
			break;
		}
		
		LOG_TRACE("i:%d obj:%s name:%s", i, objs[i].c_str(), name_.Name.c_str());
		snprintf(Name,127,"%s",name_.Name.c_str());
		start = strstr(Name, LOG_LOOP_CHECK_HEAD); //csNameHead
		if (start) {
			start = Name + strlen(LOG_LOOP_CHECK_HEAD) + 1; // exp: xxxxxxx_20180626
			parse_time = base_mktime(start);
			LOG_TRACE("Name:%s parse_time:%u now_time:%u",Name,parse_time,now_time);
			if (parse_time && (now_time > parse_time) && ((now_time - parse_time) > (7*24*60*60))) {
				LOG_DEBUG("find one dropCollectionSpace:%s",Name);
				pool->dropCollectionSpace(Name);
				return ;					
			}
		}
	}
}

void apLogServer::CreateLoopSqdbClTimer() {
	LOG_TRACE("CreateLoopSqdbClTimer");
	sqdbPool* pool = sqdbManager::getIns()->GetDbPool();
	if (!pool) {	
		LOG_ERROR("get pool err");
		return ;
	}

	sqdbCl* cl = pool->GetDbCl("ap_log.ap_log_dev");
	if (!cl) {	
		LOG_ERROR("get ap_log.ap_log_dev err");
		return ;
	}
	
	cl->UpdateLoopCsName();
}

void apLogServer::GetQosSwitchTimer() {
	string condition;
	std::vector<std::string> objs;
	ap_log_config_t config_;
	ap_log_qos_vip_t vip_;
	uint64_t deviceLogStatus_;
    uint64_t qos_wl_time_;
	int rc;
	size_t i;

	LOG_TRACE("GetQosSwitchTimer");
	sqdbPool* pool = sqdbManager::getIns()->GetDbPool();
	if (!pool) {	
		LOG_ERROR("get pool err");
		return ;
	}

	sqdbCl* cl = pool->GetDbCl("ap_config.config");
	if (!cl) {
		LOG_ERROR("GetDbCl err");
		return ;
	}

	rc = cl->query(condition,objs);
	if (rc <= 0) {
		if (rc < 0)
			LOG_ERROR("query err rc:%d",rc);
		return ;
	}

	int qos_wl_get_flag = 0;
	for (i = 0; i < objs.size(); i++) {
		LOG_TRACE("i:%d obj:%s ",i,objs[i].c_str());
		rc = slothjson::decode(objs[i], config_);
		if (!rc) {
			LOG_ERROR("decode ap_log_config_t err");
			return ;
		}

		LOG_TRACE("param_name:%s param_value:%u param_time:%u",config_.param_name.c_str(),config_.param_value,config_.param_time);
		if (config_.param_name == "deviceLogStatus") {			
			LOG_TRACE("deviceLogStatus param_value:%u",config_.param_value);  
			deviceLogStatus_ = config_.param_value;
		}

		if (config_.param_name == "qos_wl_version") {
			LOG_TRACE("qos_wl_version param_value:%u",config_.param_value);  
			qos_wl_time_ = config_.param_value;
		}
	}

	{
		std::lock_guard<std::mutex> lock1(mutex_);
		if (config_deviceLogStatus_ != deviceLogStatus_) {
			LOG_DEBUG("deviceLogStatus change to:%u",deviceLogStatus_);
			config_deviceLogStatus_ = deviceLogStatus_;
		}
		if (config_qos_wl_time_ != qos_wl_time_) {
			LOG_DEBUG("qos_wl_time change to:%u",qos_wl_time_);
			config_qos_wl_time_ = qos_wl_time_;
			qos_wl_get_flag = 1;
		}
	}

	if (!qos_wl_get_flag) 
		return ;
		
	cl = pool->GetDbCl("ap_config.log_qos_vip");
	if (!cl) {
		LOG_ERROR("GetDbCl ap_config.log_qos_vip err");
		return ;
	}

	objs.clear();
	rc = cl->query(condition,objs);
	if (rc <= 0) {
		LOG_ERROR("query err rc:%d",rc);
		return ;
	}

	{
		LOG_DEBUG("log_qos_vip size:%d",rc);  
		std::lock_guard<std::mutex> lock2(mutex_);
		macVip_.clear();
		
		LOG_DEBUG("brfore map macVip_ size:%d",macVip_.size());  
		for (i = 0; i < objs.size(); i++) {
			rc = slothjson::decode(objs[i], vip_);
			if (!rc) {
				LOG_ERROR("decode ap_log_qos_vip_t err");
				break;
			}

			std::shared_ptr<ap_log_qos_vip_t> vip(new ap_log_qos_vip_t());
			//LOG_DEBUG("i:%d obj:%s mac:%s loglev:%d",i,objs[i].c_str(),vip_.mac.c_str(),vip_.loglev);  
			vip->mac = vip_.mac;
			vip->addtime = vip_.addtime;
			vip->info = vip_.info;
			vip->loglev = vip_.loglev;
			macVip_[vip_.mac] = vip;
		}		
		LOG_DEBUG("after map macVip_ size:%d",macVip_.size());  
	}
}

void apLogServer::Init() {
    string addr = config_reader::ins()->GetString("kserver", "addr", "0.0.0.0:8000");
    string harbor = config_reader::ins()->GetString("kserver", "harbor", "0.0.0.0:8001");
    
    short serverThreadNum = config_reader::ins()->GetNumber("kserver", "threadNum", 4);
    //short maxConns = config_reader::ins()->GetNumber("kserver", "maxConns", 1024);
	short msgThreadNum = config_reader::ins()->GetNumber("msg", "threadNum", 4);


	LOG_INFO("serverThreadNum:%d  ///////",serverThreadNum);
    serverPool_.reset(new EventLoopThreadPool(base_loop_, serverThreadNum, "server"));
    LOG_INFO("serverPool_ Start //////"); 
    serverPool_->Start(true);

	LOG_INFO("msgThreadNum:%d  ///////",msgThreadNum);  
    msgPool_.reset(new EventLoopThreadPool(base_loop_, msgThreadNum, "msg"));
    LOG_INFO("msgPool_ Start //////"); 
    msgPool_->Start(true);     

	LOG_INFO("listen:%s ///////", addr.c_str());
    apiServer_.reset(new TCPServer(base_loop_, addr, "Api#Server", serverPool_));
    apiServer_->SetMessageCallback(std::bind(&apLogServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    apiServer_->SetConnectionCallback(std::bind(&apLogServer::OnConnection, this, std::placeholders::_1));
    apiServer_->Init();
    apiServer_->Start();

	LOG_INFO("listen:%s /////////", harbor.c_str());
    harborServer_.reset(new TCPServer(base_loop_, harbor, "Harbor#Server", serverPool_));
    harborServer_->SetMessageCallback(std::bind(&apLogServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    harborServer_->SetConnectionCallback(std::bind(&apLogServer::OnConnection, this, std::placeholders::_1));
    harborServer_->Init();
    harborServer_->Start();

	//LOG_INFO("RunEvery GetQosSwitchTimer 60s /////////");
	GetQosSwitchTimer();
    base_loop_->RunEvery(net::Duration(60.0), std::bind(&apLogServer::GetQosSwitchTimer, this));
    base_loop_->RunEvery(net::Duration(120.0), std::bind(&apLogServer::CreateLoopSqdbClTimer, this));    
    base_loop_->RunEvery(net::Duration(600.0), std::bind(&apLogServer::DropLoopSqdbClTimer, this));
}

void apLogServer::Send(const net::TCPConnPtr& conn,const std::string &res) {
	return conn->Send(res);
}

void apLogServer::Send(uint64_t conn_id,const std::string &res) {
	ConnectionMap::iterator it = conns_.find(conn_id);
	if (it == conns_.end()) {
		LOG_ERROR("not find conn_id:%u",conn_id);
	} else {
		Send(it->second,res);
	}
}

void apLogServer::OnConnection(const net::TCPConnPtr& conn) {
    if (conn->IsConnected()) {    	
		next_conn_id_++;		
        LOG_INFO("Accept a new (conn_id_:%u) connection %s from %s",next_conn_id_,conn->name().c_str(),conn->remote_addr().c_str());
        conn->SetTCPNoDelay(true); 
        //conns_.insert(conn);
        conn->set_context(net::Any(next_conn_id_));
		conns_[next_conn_id_] = conn;
    } else {
		//conns_.erase(conn);
        uint64_t conn_id_ = net::any_cast<uint64_t>(conn->context());        
		LOG_INFO("Disconnected (conn_id_:%u) %s from %s",conn_id_,conn->name().c_str(),conn->remote_addr().c_str());		
        conn->set_context(net::Any());
		conns_.erase(conn_id_);
    }    
}

void apLogServer::OnMessageProcess(MsgStringPtr &Message) {
	LOG_TRACE("OnMessageProcess");
	slothjson::ap_log_cmd_t ap_log_cmd;
	uint64_t macVipFlag = 0;
	string requestBase64;	
	bool rc = false;
	
	DLOG_TRACE("Message:%p,use_count:%d,conn.use_count:%d", Message.get(), Message.use_count(),Message->GetConn().use_count());
	rc = Base64::Decode(Message->GetMsg(), &requestBase64);
	if (!rc) {
		LOG_ERROR("Base64 Decode err:%s",Message->GetMsg().c_str());
		return ;
	}
	
	rc = slothjson::decode(requestBase64, ap_log_cmd);	
	if (!ap_log_cmd.json_has_RPCMethod()) {
		LOG_ERROR("no RPCMethod,requestBase64:%s",requestBase64.c_str());
		return ;
	}

	if (ap_log_cmd.RPCMethod == "Heartbeat") {
		slothjson::ap_log_heart_t heart;
		string res; 		
		string base64_res;
	
		heart.Result = 0;
		heart.ID = ap_log_cmd.ID;
	
		rc = slothjson::encode<false, slothjson::ap_log_heart_t>(heart, res);	
		if (!rc) {
			LOG_ERROR("decode ap_log_cmd_t err");
			return ;
		}	
		
		rc = Base64::Encode(res, &base64_res);
		base64_res = base64_res + "\r\n";			
		Message->GetConn()->Send(base64_res);
		LOG_TRACE("Addr:%s,res:%s",Message->GetConn()->AddrToString().c_str(),res.c_str());			
		return ;
	}

	if (ap_log_cmd.RPCMethod != "log") {
		LOG_TRACE("RPCMethod:%s not log",ap_log_cmd.RPCMethod.c_str());
		return ;
	}
	
	slothjson::ap_log_dev_t log_db;
	string db_obj; 		
	macVipFlag = GetFindMacVip(ap_log_cmd.params.MAC);
	if (!macVipFlag) {
		LOG_TRACE("mac:%s macVipFlag:%d skip",ap_log_cmd.params.MAC.c_str(),macVipFlag);
		return ;
	}
	
	log_db.RPCMethod = ap_log_cmd.params.RPCMethod;
	log_db.ID = ap_log_cmd.ID;
	log_db.mac = ap_log_cmd.params.MAC;
	if (!ap_log_cmd.params.server.size())
		log_db.skip_server();
	else
		log_db.server = ap_log_cmd.params.server;
	log_db.remote = ap_log_cmd.params.remote;
	log_db.start_time = ap_log_cmd.params.start_time;
	log_db.end_time = ap_log_cmd.params.end_time;
	log_db.direction = ap_log_cmd.params.direction;
	log_db.Result = ap_log_cmd.params.Result;
	log_db.in = ap_log_cmd.params.in;
	log_db.out = ap_log_cmd.params.out;
	log_db.client_ip = Message->GetConn()->remote_addr();
	log_db.addtime = net::Timestamp::Now().Unix();			

	rc = slothjson::encode<false, slothjson::ap_log_dev_t>(log_db, db_obj);	
	if (!rc) {
		LOG_ERROR("encode ap_log_db_t err");
		return ;
	}

	LOG_TRACE("before insert");
	sqdbPool* pool = sqdbManager::getIns()->GetDbPool();
	if (!pool) {
		LOG_ERROR("get pool err");
		return ;
	}
	
	// ap_log_20180702 skip time
	sqdbCl* cl = pool->GetDbCl("ap_log.ap_log_dev");
	if (!cl) {
		LOG_ERROR("get cl ap_log.ap_log_dev err");
		return ;
	}
	cl->insert(db_obj);		
	LOG_TRACE("after insert ok:%s",db_obj.c_str());	
	//LOG_TRACE("after insert ok:%s",log_db.mac.c_str());	
}

void apLogServer::OnMessage(const net::TCPConnPtr& conn, net::Buffer* msg) {	
	while (msg->size() >= 100) {
		const char* crlf = msg->FindCRLF();
		if (!crlf) 
			break;

		net::EventLoop* loop = msgPool_->GetNextLoop();
		if (!loop) {
			LOG_ERROR("loop IsEmpty"); 
			continue ;
		}

		MsgStringPtr Message(new MsgString(loop, conn, msg->data(), crlf));	
		msg->Retrieve(crlf + 2);	

		loop->RunInLoop(std::bind(&apLogServer::OnMessageProcess, this, Message));
		DLOG_TRACE("Message:%p,use_count:%d,conn.use_count:%d", Message.get(), Message.use_count(),conn.use_count());
	}
}

}
