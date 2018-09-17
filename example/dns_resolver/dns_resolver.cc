#include "tcp_server.h"
#include "buffer.h"
#include "tcp_conn.h"
#include "dns_resolver.h"
#include "config_reader.h"

using namespace std;
using namespace base;
using namespace net;

int main(int argc, char* argv[]) {
    std::string host = "qq.com";
    if (argc > 1) {
        host = argv[1];
    }

	// log console 
	LOG_INIT(NULL, NULL, true, DEBUG);

    net::EventLoop loop;

    auto fn_resolved = [&loop, &host](const std::vector <struct in_addr>& addrs) {
        LOG_INFO("Entering fn_resolved");
        for (auto addr : addrs) {
            struct sockaddr_in saddr;
            memset(&saddr, 0, sizeof(saddr));
            saddr.sin_addr = addr;
            LOG_INFO("DNS resolved host:%s ip:%s",host.c_str(), inet_ntoa(saddr.sin_addr));
        }

        loop.RunAfter(net::Duration(0.5), [&loop]() { loop.Stop(); });
    };

    std::shared_ptr<net::DNSResolver> dns_resolver(new net::DNSResolver(&loop , host, net::Duration(1.0), fn_resolved));
    dns_resolver->Start();
    loop.Run();

    return 0;
}

