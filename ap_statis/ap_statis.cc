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
#include "ap_statis.h"

using namespace std;
using namespace base;
using namespace net;
using namespace slothjson;
using namespace sqdb;


Server::Server(net::EventLoop* loop) {
	base_loop_ = loop;
}

Server::~Server() {
}

void Server::DropLoopSqdbClTimer() {

}

void Server::CreateLoopSqdbClTimer() {
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

void Server::GetQosSwitchTimer() {

}

void Server::Init() {
    string harbor = config_reader::ins()->GetString("kserver", "harbor", "0.0.0.0:7073");
    
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

	LOG_INFO("listen:%s /////////", harbor.c_str());
    harborServer_.reset(new TCPServer(base_loop_, harbor, "Harbor#Server", serverPool_));
    harborServer_->SetMessageCallback(std::bind(&Server::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    harborServer_->SetConnectionCallback(std::bind(&Server::OnConnection, this, std::placeholders::_1));
    bool ret = harborServer_->Init();
    if (ret == false)
    	exit(-1);    	
    harborServer_->Start();

	//LOG_INFO("RunEvery GetQosSwitchTimer 60s /////////");
	GetQosSwitchTimer();
    base_loop_->RunEvery(net::Duration(60.0), std::bind(&Server::GetQosSwitchTimer, this));
    base_loop_->RunEvery(net::Duration(120.0), std::bind(&Server::CreateLoopSqdbClTimer, this));    
    base_loop_->RunEvery(net::Duration(600.0), std::bind(&Server::DropLoopSqdbClTimer, this));
}

void Server::OnConnection(const net::TCPConnPtr& conn) {
    if (conn->IsConnected()) {    	
        LOG_INFO("Accept a new  connection %s from %s",conn->name().c_str(),conn->remote_addr().c_str());        
        conn->SetTCPNoDelay(true);         
		ConnPrivPtr cp = make_shared<ConnPriv>(conn->id(),conn->fd());
        conn->set_context(net::Any(cp));     
        conns_.insert(conn);
    } else {
		LOG_INFO("Disconnected %s from %s",conn->name().c_str(),conn->remote_addr().c_str());		
        ConnPrivPtr cp = net::any_cast<ConnPrivPtr>(conn->context());        
        conn->set_context(net::Any());        
		conns_.erase(conn);
    }    
}

void Server::OnMessageProcess(MsgStringPtr &Message) {
	LOG_TRACE("OnMessageProcess");	
	slothjson::cmd_t cmd;
	string requestBase64;
	string RPCMethod;
	bool rc = false;
	
	DLOG_TRACE("Message:%p,use_count:%d,conn.use_count:%d", Message.get(), Message.use_count(),Message->GetConn().use_count());
	rc = Base64::Decode(Message->GetMsg(), &requestBase64);
	if (!rc) {
		LOG_ERROR("Base64 Decode err:%s",Message->GetMsg().c_str());
		return ;
	}

	//LOG_INFO("requestBase64:%d|%s",requestBase64.size(),requestBase64.c_str());
	rc = slothjson::decode(requestBase64, cmd);	
	if (cmd.json_has_RPCMethod()) {
		RPCMethod = cmd.RPCMethod;
	}
	if (cmd.json_has_Ack()) {
		RPCMethod = cmd.Ack;
	}

	if (!RPCMethod.size()) {
		LOG_ERROR("no RPCMethod,requestBase64:%s",requestBase64.c_str());
		return;
	}
	
	if (RPCMethod == "Heartbeat") {
		slothjson::Heartbeat_t heart;
		string res; 		
		string base64_res;
	
		if (!cmd.json_has_ID()) {
			LOG_ERROR("Heartbeat but no ID,requestBase64:%s",requestBase64.c_str());
			return;
		}
		
		heart.Result = 0;
		heart.ID = cmd.ID;
	
		rc = slothjson::encode<false, slothjson::Heartbeat_t>(heart, res);	
		if (!rc) {
			LOG_ERROR("decode Heartbeat_t err");
			return ;
		}	

		ConnPrivPtr cp = net::any_cast<ConnPrivPtr>(Message->GetConn()->context());
		APP_DEBUG("Heartbeat fd:%u id:%u",cp->fd_,cp->id_);
		
		rc = Base64::Encode(res, &base64_res);
		base64_res = base64_res + "\r\n";			
		Message->GetConn()->Send(base64_res);
		LOG_TRACE("Addr:%s,res:%s",Message->GetConn()->AddrToString().c_str(),res.c_str());			
		return ;
	}
}

void Server::OnMessage(const net::TCPConnPtr& conn, net::Buffer* msg) {	
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

		loop->RunInLoop(std::bind(&Server::OnMessageProcess, this, Message));
		DLOG_TRACE("Message:%p,use_count:%d,conn.use_count:%d", Message.get(), Message.use_count(),conn.use_count());
	}
}


