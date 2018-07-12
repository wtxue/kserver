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
	LOG_TRACE("apLogServer");  
}

apLogServer::~apLogServer() {
}

void apLogServer::CreateLoopSqdbClTimer() {
	LOG_TRACE("CreateLoopSqdbClTimer");
	struct cs_name_t name_;
	sqdbPool* pool = sqdbManager::getIns()->GetDbPool("ap_log");
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

	std::vector<std::string> objs;
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
			LOG_ERROR("decode ap_log_qos_vip_t err");
			break;
		}
		
		LOG_DEBUG("i:%d obj:%s name:%s", i, objs[i].c_str(), name_.Name.c_str());
		snprintf(Name,127,"%s",name_.Name.c_str());
		start = strstr(Name, cl->GetcsNameHead().c_str());
		if (start) {
			start = Name + cl->GetcsNameHead().size() + 1; // exp: ap_log_20180626
			parse_time = base_mktime(start);
			LOG_DEBUG("Name:%s parse_time:%u now_time:%u",Name,parse_time,now_time);
			if (parse_time && (now_time > parse_time) && ((now_time - parse_time) > (7*24*60*60))) {
				LOG_DEBUG("find one dropCollectionSpace:%s",Name);
				pool->dropCollectionSpace(Name);
				return ;					
			}
		}
	}

	int vm_size_kb = 0;  
	int rss_size_kb = 0;	
	long long CurCpuTime =  GetCurCpuTime();
	long long TotalCpuTime =  GetTotalCpuTime();
	GetCurMemoryUsage(&vm_size_kb, &rss_size_kb);	
	LOG_DEBUG("CurCpuTime:%lu, TotalCpuTime:%lu, vm_size_kb:%dk, rss_size_kb:%dk", CurCpuTime, TotalCpuTime, vm_size_kb, rss_size_kb);
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
	sqdbPool* pool = sqdbManager::getIns()->GetDbPool("ap_log");
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
		LOG_ERROR("query err rc:%d",rc);
		return ;
	}

	int qos_wl_get_flag = 0;
	for (i = 0; i < objs.size(); i++) {
		LOG_DEBUG("i:%d obj:%s ",i,objs[i].c_str());
		rc = slothjson::decode(objs[i], config_);
		if (!rc) {
			LOG_ERROR("decode ap_log_config_t err");
			return ;
		}

		LOG_DEBUG("param_name:%s param_value:%u param_time:%u",config_.param_name.c_str(),config_.param_value,config_.param_time);  

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
		#if 1
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
		#else
			
			std::shared_ptr<ap_log_qos_vip_t> vip(new ap_log_qos_vip_t());
			ap_log_qos_vip_t &tmp = std::move(vip);
			rc = slothjson::decode(objs[i], tmp);
			if (!rc) {
				LOG_ERROR("decode ap_log_qos_vip_t err");
				break;
			}
			
			LOG_DEBUG("i:%d obj:%s mac:%s loglev:%d",i,objs[i].c_str(),vip->mac.c_str(),vip->loglev);  
			macVip_[vip->mac] = vip;
		#endif
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
	msgQueuePool_.Init(msgPool_);	  

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
}

void apLogServer::OnMessageProcess(net::EventLoop* loop) {
	//LOG_TRACE("OnMessageProcess");
	uint64_t macVipFlag = 0;
	EventLoopQueuePtr Queue;
	std::queue<MsgBufferPtr> msgs;
	slothjson::ap_log_cmd_t* msg;
	
	if (!msgQueuePool_.GetEventLoopQueue(loop, Queue)) {
		LOG_DEBUG("Queue IsEmpty"); 
		return ;
	}

	Queue->PopAll(msgs);
	LOG_DEBUG("msgs size:%d",msgs.size());
    while (!msgs.empty()) {
        MsgBufferPtr buffer = msgs.front();
        msgs.pop();

		//DLOG_TRACE("buffer:%p use_count:%d conn_id_:%d",buffer.get(),buffer.use_count(),buffer->GetId());	
		buffer->GetMsg(msg);
		if (!msg->json_has_RPCMethod()) {
			LOG_DEBUG("no RPCMethod");
			continue ;
		}

		if (msg->RPCMethod != "log") {
			LOG_DEBUG("RPCMethod:%s not log",msg->RPCMethod.c_str());
			continue ;
		}
		
		slothjson::ap_log_db_t log_db;
		string db_obj; 		
		macVipFlag= GetFindMacVip(msg->params.MAC);
		if (!macVipFlag) {
			LOG_DEBUG("mac:%s macVipFlag:%d skip",msg->params.MAC.c_str(),macVipFlag);
			continue ;
		}
		
		log_db.RPCMethod = msg->params.RPCMethod;
		log_db.ID = msg->ID;
		log_db.mac = msg->params.MAC;
		if (!msg->params.server.size())
			log_db.skip_server();
		else
			log_db.server = msg->params.server;
		log_db.remote = msg->params.remote;
		log_db.start_time = msg->params.start_time;
		log_db.end_time = msg->params.end_time;
		log_db.direction = msg->params.direction;
		log_db.Result = msg->params.Result;
		log_db.in = msg->params.in;
		log_db.out = msg->params.out;
		log_db.client_ip = buffer->GetConnName();
		log_db.addtime = net::Timestamp::Now().Unix();			

		int rc = slothjson::encode<false, slothjson::ap_log_db_t>(log_db, db_obj);	
		if (!rc) {
			LOG_DEBUG("decode ap_log_db_t err");
			continue ;
		}
	
		LOG_DEBUG("brfore insert");
		sqdbPool* pool = sqdbManager::getIns()->GetDbPool("ap_log");
		if (!pool) {
			LOG_ERROR("get pool err");
			continue ;
		}
		
		// ap_log_20180702 skip time
		sqdbCl* cl = pool->GetDbCl("ap_log.ap_log_dev");
		if (!cl) {
			LOG_ERROR("get cl ap_log.ap_log_dev err");
			continue ;
		}
		cl->insert(db_obj);		
		//LOG_DEBUG("after insert ok:%s",db_obj.c_str());	
		LOG_DEBUG("after insert ok:%s",log_db.mac.c_str());	
    }
}

void apLogServer::Send(const net::TCPConnPtr& conn,const std::string &res) {
	return conn->Send(res);
}

void apLogServer::Send(uint64_t conn_id,const std::string &res) {
	ConnectionMap::iterator it = conns_.find(conn_id);
	if (it == conns_.end()) {
		LOG_TRACE("not find conn_id:%u",conn_id);
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

void apLogServer::OnMessage(const net::TCPConnPtr& conn, net::Buffer* msg) {	
	string requestBase64;	
	bool rc = false;
	
	//LOG_DEBUG("conn name:%s size:%d",conn->name().c_str(),msg->size());	
	while (msg->size() >= 100) {
		const char* crlf = msg->FindCRLF();
		if (!crlf) 
			break;
		string request(msg->data(), crlf);
		msg->Retrieve(crlf + 2);

		rc = Base64::Decode(request, &requestBase64);
		if (!rc) {
			LOG_ERROR("Base64 Decode err:%s",request.c_str());
			continue ;
		}

		//LOG_DEBUG("requestBase64:%d,%s",requestBase64.size(),requestBase64.c_str());
		// todo
		EventLoop* loop = msgQueuePool_.GetNextMsgLoop(0);
		if (!loop) {
			LOG_DEBUG("loop IsEmpty"); 
			continue ;
		}

		EventLoopQueuePtr Queue;
		if (!msgQueuePool_.GetEventLoopQueue(loop, Queue)) {
			LOG_DEBUG("Queue IsEmpty"); 
			continue ;
		}

		//DLOG_TRACE("new Queue:%p use_count:%d",Queue.get(),Queue.use_count());	
		if (conn->context().IsEmpty()) {
			LOG_DEBUG("conn_id_ IsEmpty"); 
			continue ;
		}

		// only not one server in thread pool
		uint64_t conn_id_ = net::any_cast<uint64_t>(conn->context());			
		MsgBufferPtr buffer(new MsgBuffer(loop,conn_id_,conn->remote_addr()));	
		//LOG_DEBUG("///////////////////////conn_id_:%u id:%u fd:%d",conn_id_,conn->id(),conn->fd()); 		
		
		//DLOG_TRACE("buffer:%p use_count:%d ",buffer.get(),buffer.use_count());	
		rc = slothjson::decode(requestBase64, buffer->GetMsg());	

		slothjson::ap_log_cmd_t* msg;
		buffer->GetMsg(msg);
		if (!msg->json_has_RPCMethod()) {
			LOG_DEBUG("no RPCMethod:%s",requestBase64.c_str());
			continue ;
		}

		if (msg->RPCMethod == "Heartbeat") {
			slothjson::ap_log_heart_t heart;
			string res; 		
			string base64_res;

			heart.Result = 0;
			heart.ID = msg->ID;

			rc = slothjson::encode<false, slothjson::ap_log_heart_t>(heart, res);	
			if (!rc) {
				LOG_ERROR("decode ap_log_cmd_t err");
				return ;
			}
		
			
			LOG_DEBUG("res:%s",res.c_str());			
			rc = Base64::Encode(res, &base64_res);
			base64_res = base64_res + "\r\n";			
			//LOG_DEBUG("base64_res:%s",base64_res.c_str());	
			Send(conn,base64_res);			
			//buffer.reset();
		}
		else {
			//Queue->Push(std::move(buffer));
			Queue->Push(buffer);			
			//DLOG_TRACE("buffer:%p use_count:%d ",buffer.get(),buffer.use_count());	
			loop->RunInLoop(std::bind(&apLogServer::OnMessageProcess, this, loop));
		}

#if 0
		slothjson::ap_log_cmd_t obj_cmd;
		
		rc = slothjson::decode(requestBase64, obj_cmd);	
		if (!rc) {
			LOG_DEBUG("decode ap_log_cmd_t err");
			return ;
		}
			
		if (obj_cmd.json_has_RPCMethod()) {
			LOG_DEBUG("RPCMethod:%s",obj_cmd.RPCMethod.c_str());
		}
		if (obj_cmd.json_has_ID()) {
			LOG_DEBUG("ID:%s",obj_cmd.ID.c_str());
		}
		if (obj_cmd.json_has_params()) {
			LOG_DEBUG("params.RPCMethod:%s",obj_cmd.params.RPCMethod.c_str());
			LOG_DEBUG("params.MAC:%s",obj_cmd.params.MAC.c_str());
			LOG_DEBUG("params.DevID:%s",obj_cmd.params.DevID.c_str());
			LOG_DEBUG("params.direction:%d",obj_cmd.params.direction);
			LOG_DEBUG("params.Result:%d",obj_cmd.params.Result);
			LOG_DEBUG("params.in:%s",obj_cmd.params.in.c_str());
			LOG_DEBUG("params.out:%s",obj_cmd.params.out.c_str());
			if (obj_cmd.params.json_has_in()) {
				slothjson::params_in_t obj_in;
				rc = slothjson::decode(obj_cmd.params.in, obj_in);
				if (!rc)
					LOG_DEBUG("prase in err");
				else {
					LOG_DEBUG("in.RPCMethod:%s",obj_in.RPCMethod.c_str());
					LOG_DEBUG("in.Time:%s",obj_in.Time.c_str());
					LOG_DEBUG("in.Message:%s",obj_in.Message.c_str());
				}	 
			}
		}

		if (obj_cmd.RPCMethod == "Heartbeat") {
			slothjson::ap_log_heart_t heart;
			string res;			
			string base64_res;

			heart.Result = 0;
			heart.ID = obj_cmd.ID;

			rc = slothjson::encode<false, slothjson::ap_log_heart_t>(heart, res);	
			if (!rc) {
				LOG_DEBUG("decode ap_log_cmd_t err");
				return ;
			}
		
			
			LOG_DEBUG("res:%s",res.c_str());			
			rc = Base64::Encode(res, &base64_res);
			base64_res = base64_res + "\r\n";			
			LOG_DEBUG("base64_res:%s",base64_res.c_str());	
			Send(conn,base64_res);
		}
		
#endif
	}
}

}
