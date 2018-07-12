#include "net/inner_pre.h"

#include "sync_udp_client.h"
#include "net/libevent.h"
#include "net/sockets.h"

namespace net {
namespace udp {
namespace sync {
UDPClient::UDPClient() {
    sockfd_ = INVALID_SOCKET;
    memset(&remote_addr_, 0, sizeof(remote_addr_));
}

UDPClient::~UDPClient(void) {
	DLOG_TRACE("destroy udp Client");
    Close();
}

bool UDPClient::Connect(const struct sockaddr_in& addr) {
    memcpy(&remote_addr_, &addr, sizeof(remote_addr_));
    return Connect();
}

bool UDPClient::Connect(const char* host, int port) {
    char buf[32] = {};
    snprintf(buf, sizeof buf, "%s:%d", host, port);
    return Connect(buf);
}

bool UDPClient::Connect(const struct sockaddr_storage& addr) {
    memcpy(&remote_addr_, &addr, sizeof(remote_addr_));
    return Connect();
}

bool UDPClient::Connect(const char* addr/*host:port*/) {
    remote_addr_ = sock::ParseFromIPPort(addr);
    return Connect();
}

bool UDPClient::Connect(const struct sockaddr& addr) {
    memcpy(&remote_addr_, &addr, sizeof(remote_addr_));
    return Connect();
}

bool UDPClient::Connect() {
    sockfd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    sock::SetReuseAddr(sockfd_);

    struct sockaddr* addr = reinterpret_cast<struct sockaddr*>(&remote_addr_);
    socklen_t addrlen = sizeof(*addr);
    int ret = ::connect(sockfd_, addr, addrlen);

    if (ret != 0) {
        LOG_ERROR("Failed to connect to remote %s, errno:%d %s",sock::ToIPPort(&remote_addr_).c_str(),errno,strerror(errno));
        Close();
        return false;
    }

    connected_ = true;
    return true;
}

void UDPClient::Close() {
    EVUTIL_CLOSESOCKET(sockfd_);
}


std::string UDPClient::DoRequest(const std::string& data, uint32_t timeout_ms) {
    if (!Send(data)) {
        int eno = errno;
        LOG_ERROR("sent failed, errno:%d %s, dlen:%d",eno,strerror(eno),data.size());
        return "";
    }

    sock::SetTimeout(sockfd_, timeout_ms);

    size_t buf_size = 1472; // The UDP max payload size
    MessagePtr msg(new Message(sockfd_, buf_size));
    socklen_t addrLen = sizeof(struct sockaddr);
    int readn = ::recvfrom(sockfd_, msg->WriteBegin(), buf_size, 0, msg->mutable_remote_addr(), &addrLen);
    int err = errno;
    if (readn >= 0) {
        msg->WriteBytes(readn);
        return std::string(msg->data(), msg->size());
    } else {
        LOG_ERROR("errno:%d %s, recvfrom return -1",err,strerror(err));
    }

    return "";
}

std::string UDPClient::DoRequest(const std::string& remote_ip, int port, const std::string& udp_package_data, uint32_t timeout_ms) {
    UDPClient c;
    if (!c.Connect(remote_ip.data(), port)) {
        return "";
    }

    return c.DoRequest(udp_package_data, timeout_ms);
}

bool UDPClient::Send(const char* msg, size_t len) {
    if (connected_) {
        int sentn = ::send(sockfd(), msg, len, 0);
        return sentn == len;
    }

    struct sockaddr* addr = reinterpret_cast<struct sockaddr*>(&remote_addr_);
    socklen_t addrlen = sizeof(*addr);
    int sentn = ::sendto(sockfd(),
                         msg, len, 0,
                         addr,
                         addrlen);
    return sentn > 0;
}

bool UDPClient::Send(const std::string& msg) {
    return Send(msg.data(), msg.size());
}

bool UDPClient::Send(const std::string& msg, const struct sockaddr_in& addr) {
    return UDPClient::Send(msg.data(), msg.size(), addr);
}


bool UDPClient::Send(const char* msg, size_t len, const struct sockaddr_in& addr) {
    UDPClient c;
    if (!c.Connect(addr)) {
        return false;
    }

    return c.Send(msg, len);
}

bool UDPClient::Send(const MessagePtr& msg) {
    return UDPClient::Send(msg->data(), msg->size(), *reinterpret_cast<const struct sockaddr_in*>(msg->remote_addr()));
}

bool UDPClient::Send(const Message* msg) {
    return UDPClient::Send(msg->data(), msg->size(), *reinterpret_cast<const struct sockaddr_in*>(msg->remote_addr()));
}

}
}
}


