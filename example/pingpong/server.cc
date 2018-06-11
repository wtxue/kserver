#include "tcp_server.h"
#include "buffer.h"
#include "tcp_conn.h"
#include "dns_resolver.h"

#include "base64.h"
#include "config_reader.h"

using namespace base;
using namespace net;


#if 1
void OnConnection(const net::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        LOG_INFO("Accept a new connection from %s",conn->remote_addr().c_str());
        conn->SetTCPNoDelay(true);
    } else {
		LOG_INFO("Disconnected from %s",conn->remote_addr().c_str());
    }    
}

void OnMessage(const net::TCPConnPtr& conn, net::Buffer* msg) {
#if 0
	LOG_DEBUG("conn name:%s size:%d",conn->name().c_str(),msg->size());
    std::string s = msg->NextAllString();
    LOG_INFO("Received a message %d[%s]",s.size(),s.c_str());
    conn->Send(s);

    if (s == "quit" || s == "exit") {
	    LOG_INFO("conn close");
        conn->Close();
    }    
#else
	LOG_DEBUG("conn name:%s size:%d",conn->name().c_str(),msg->size());
	size_t len = msg->size();
	while (len >= 50) {
		const char* crlf = msg->FindCRLF();
		if (!crlf) 
			break;
		string request(msg->data(), crlf);
		msg->Retrieve(crlf + 2);

		string requestBase64;
		bool bret = Base64::Decode(request, &requestBase64);
		if (bret == true) {
			//LOG_DEBUG("requestBase64:%d,%s",requestBase64.size(),requestBase64.c_str());
		}
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
    std::string addr = config_reader::ins()->GetString("kserver", "addr", "0.0.0.0:8000");
    std::string harbor = config_reader::ins()->GetString("kserver", "harbor", "0.0.0.0:8001");
    
    short threadNum = config_reader::ins()->GetNumber("kserver", "threadNum", 4);
    //short maxConns = config_reader::ins()->GetNumber("kserver", "maxConns", 1024);

	std::string log_type = config_reader::ins()->GetString("logInfo", "log_type", "file");
	printf("log_type:%s\n",log_type.c_str());
	if (log_type == "file") {
		std::string log_dir = config_reader::ins()->GetString("logInfo", "log_dir", "./log/");
		std::string log_name = config_reader::ins()->GetString("logInfo", "log_name", "server");
		printf("log:%s %s\n",log_dir.c_str(),log_name.c_str());
		LOG_INIT(log_dir.c_str(), log_name.c_str(), false, TRACE);
	}
	else {	
		LOG_INIT(NULL, NULL, true, TRACE);
	}

#if 0
	LOG_INFO("listen:%s %s threadNum:%d", addr.c_str(),harbor.c_str(),threadNum);  
    evpp::EventLoop loop;
    evpp::TCPServer server(&loop, addr, "Server", threadNum);
    server.SetMessageCallback(&OnMessage);
    server.SetConnectionCallback(&OnConnection);
    server.Init();
    server.Start();
    loop.Run();

#else
	LOG_INFO("threadNum:%d /////////////////////////////////////////////////////////////////",threadNum);  
    net::EventLoop loop;    
    std::shared_ptr<EventLoopThreadPool> tpool(new EventLoopThreadPool(&loop, threadNum));
    LOG_INFO("tpool Start ///////////////////////////////////////////////////////////////////"); 
    tpool->Start(true);

	LOG_INFO("listen:%s //////////////////////////////////////////////////////////////////////", addr.c_str());
    net::TCPServer server(&loop, addr, "8000#Server", tpool);
    server.SetMessageCallback(&OnMessage);
    server.SetConnectionCallback(&OnConnection);
    server.Init();
    server.Start();

	LOG_INFO("listen:%s ///////////////////////////////////////////////////////////////////////", harbor.c_str());
    net::TCPServer s_harbor(&loop, harbor, "8001#Server", tpool);
    s_harbor.SetMessageCallback(&OnMessage);
    s_harbor.SetConnectionCallback(&OnConnection);
    s_harbor.Init();
    s_harbor.Start();
    
    LOG_INFO("main loop Start ///////////////////////////////////////////////////////////////////////////"); 
    loop.Run();

#if 0
    std::vector<std::shared_ptr<evpp::TCPServer>> tcp_servers;
    for (uint32_t i = 0; i < threadNum; i++) {
        evpp::EventLoop* next = tpool.GetNextLoop();
        std::shared_ptr<evpp::TCPServer> s(new evpp::TCPServer(next, addr, std::to_string(i) + "#server", 0));
        s->SetMessageCallback(&OnMessage);
        s->Init();
        s->Start();
        tcp_servers.push_back(s);
    }
#endif

#endif
    
    return 0;
}
#endif

#if 0
int main(int argc, char* argv[]) {
    std::string host = "qq.com";
    if (argc > 1) {
        host = argv[1];
    }

	// log  
	LOG_INIT(NULL, NULL, true, TRACE);

    evpp::EventLoop loop;

    auto fn_resolved = [&loop, &host](const std::vector <struct in_addr>& addrs) {
        LOG_INFO("Entering fn_resolved");
        for (auto addr : addrs) {
            struct sockaddr_in saddr;
            memset(&saddr, 0, sizeof(saddr));
            saddr.sin_addr = addr;
            LOG_INFO("DNS resolved host:%s ip:%s",host.c_str(), inet_ntoa(saddr.sin_addr));
        }

        loop.RunAfter(evpp::Duration(0.5), [&loop]() { loop.Stop(); });
    };

    std::shared_ptr<evpp::DNSResolver> dns_resolver(new evpp::DNSResolver(&loop , host, evpp::Duration(1.0), fn_resolved));
    dns_resolver->Start();
    loop.Run();

    return 0;
}
#endif

