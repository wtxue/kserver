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
#include "tlv_hb.h"

using namespace std;
using namespace base;
using namespace net;

void Server::CreateLoopSqdbClTimer() {
	LOG_TRACE("CreateLoopSqdbClTimer");

	int vm_size_kb = 0;  
	int rss_size_kb = 0;	
	long long CurCpuTime =  GetCurCpuTime();
	long long TotalCpuTime =  GetTotalCpuTime();
	GetCurMemoryUsage(&vm_size_kb, &rss_size_kb);	
	LOG_DEBUG("CurCpuTime:%lu, TotalCpuTime:%lu, vm_size_kb:%dk, rss_size_kb:%dk", CurCpuTime, TotalCpuTime, vm_size_kb, rss_size_kb);
}

void Server::Init() {
    string host = config_reader::ins()->GetString("kserver", "host", "0.0.0.0:1354");    
    short serverThreadNum = config_reader::ins()->GetNumber("kserver", "threadNum", 2);

	LOG_INFO("serverThreadNum:%d  ///////",serverThreadNum);
    serverPool_.reset(new EventLoopThreadPool(base_loop_, serverThreadNum, "server"));
    LOG_INFO("serverPool_ Start //////"); 
    serverPool_->Start(true);

	LOG_INFO("listen:%s ///////", host.c_str());
    hostServer_.reset(new TCPServer(base_loop_, host, "1354#Server", serverPool_));		
    hostServer_->SetMessageCallback(std::bind(&LengthHeaderCodec::OnMessage, &codec_, std::placeholders::_1, std::placeholders::_2));
    hostServer_->SetConnectionCallback(std::bind(&Server::OnConnection, this, std::placeholders::_1));
    hostServer_->Init();
    hostServer_->Start();

    base_loop_->RunEvery(net::Duration(120.0), std::bind(&Server::CreateLoopSqdbClTimer, this));
}

void Server::OnConnection(const net::TCPConnPtr& conn) {
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

void Server::OnMessage(const net::TCPConnPtr& conn, MsgTlvBoxPtr& Message) {	
	LOG_TRACE("OnMessage");
	uint16_t cmd;
	int idx = 0;

	std::vector<int> lists;
	Message->GetTlv().GetTLVList(lists);
	for (auto &it : lists)		
		LOG_DEBUG("idx:%d cmd:%d",idx++,it);

	Message->GetTlv().GetShortValue(UCPA_MSG, cmd);
	LOG_DEBUG("cmd:%d",cmd);
	if (cmd == UCMSG_LIVE) {
		TlvBox res;
		res.PutShortValue(ACPA_RETURN, ACRET_OK);
		res.PutShortValue(ACPA_MSG, ACMSG_LIVE);
		res.Serialize();
		codec_.Send(Message->GetConn(),res);
	}
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s config.ini\n", argv[0]);
        printf("  exp: %s ./config.ini\n", argv[0]);
        return 0;
    }

	printf("\nconf path: %s\n", argv[1]);
	if (!config_reader::setPath(argv[1])) {
		printf("load path: %s Failed, please check your config\n", argv[1]);
		return 0;
	}

	string log_type = config_reader::ins()->GetString("logInfo", "log_type", "console");
    int    log_level = config_reader::ins()->GetNumber("logInfo", "log_level", 5); /* DEBUG */
	printf("log_type:%s\n",log_type.c_str());
	if (log_type == "file") {
		string log_dir = config_reader::ins()->GetString("logInfo", "log_dir", "./log/");
		string log_name = config_reader::ins()->GetString("logInfo", "log_name", "server");
		printf("log:%s %s\n",log_dir.c_str(),log_name.c_str());
		LOG_INIT(log_dir.c_str(), log_name.c_str(), false, log_level);
	}
	else {	
		LOG_INIT(NULL, NULL, true, log_level);
	}

    net::EventLoop base_loop;  
    Server s(&base_loop);
    s.Init();	
	LOG_INFO("base loop Start /////////"); 
    base_loop.Run();    
    return 0;
}

