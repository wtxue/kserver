#include "tcp_server.h"
#include "buffer.h"
#include "tcp_conn.h"
#include "config_reader.h"

using namespace std;
using namespace base;
using namespace net;


void OnConnection(const net::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        LOG_INFO("Accept a new connection from %s",conn->remote_addr().c_str());
        conn->SetTCPNoDelay(true);
    } else {
		LOG_INFO("Disconnected from %s",conn->remote_addr().c_str());
    }  
}

void OnMessage(const net::TCPConnPtr& conn, net::Buffer* msg) {
	//LOG_DEBUG("conn name:%s size:%d",conn->name().c_str(),msg->size());
#if 1  
    conn->Send(msg);
#else
    string s = msg->NextAllString();
    LOG_INFO("Received a message %d[%s]",s.size(),s.c_str());
    conn->Send(s);

    if (s == "quit" || s == "exit") {
	    LOG_INFO("conn close");
        conn->Close();
    }
#endif    
}

int main(int argc, char* argv[]) {
    if (argc != 1 && argc != 3) {
        printf("Usage: %s config\n", argv[0]);
        printf("  e.g: %s conf.ini\n", argv[0]);
        return 0;
    }

    config_reader::setPath("myconf.ini");
    std::string addr = config_reader::ins()->GetString("kserver", "addr", "0.0.0.0:8088"); 
    short threadNum = config_reader::ins()->GetNumber("kserver", "threadNum", 4);

	string log_type = config_reader::ins()->GetString("logInfo", "log_type", "file");
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

	LOG_INFO("listen:%s threadNum:%d", addr.c_str(),threadNum);  
    net::EventLoop loop;
    net::TCPServer server(&loop, addr, "Server", threadNum);
    server.SetMessageCallback(&OnMessage);
    server.SetConnectionCallback(&OnConnection);
    server.Init();
    server.Start();
    loop.Run();    
    return 0;
}




