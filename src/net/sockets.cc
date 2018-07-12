#include "inner_pre.h"

#include "libevent.h"
#include "sockets.h"
#include "duration.h"

namespace net {

static const std::string empty_string;

std::string strerror(int e) {
    char buf[2048] = {};
    #if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
    int rc = strerror_r(e, buf, sizeof(buf) - 1); // XSI-compliant
    if (rc == 0) {
        return std::string(buf);
    }
    #else
    const char* s = strerror_r(e, buf, sizeof(buf) - 1); // GNU-specific
    if (s) {
        return std::string(s);
    }
    #endif

    return empty_string;
}

namespace sock {
net_socket_t CreateNonblockingSocket() {
    int serrno = 0;

    /* Create listen socket */
    net_socket_t fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        serrno = errno;
        LOG_ERROR("socket error:%s",strerror(serrno).c_str());
        return INVALID_SOCKET;
    }

    if (evutil_make_socket_nonblocking(fd) < 0) {
        goto out;
    }

    if (fcntl(fd, F_SETFD, 1) == -1) {
        serrno = errno;
        LOG_FATAL("fcntl(F_SETFD):%s",strerror(serrno).c_str());
        goto out;
    }

    DLOG_TRACE("socket fd:%d",fd);
    SetKeepAlive(fd, true);
    SetReuseAddr(fd);
    SetReusePort(fd);
    return fd;
out:
    EVUTIL_CLOSESOCKET(fd);
    return INVALID_SOCKET;
}

net_socket_t CreateUDPServer(const std::string& addr) {
    net_socket_t fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        int serrno = errno;
        LOG_ERROR("socket error:%s ",strerror(serrno).c_str());
        return INVALID_SOCKET;
    }
    SetReuseAddr(fd);
    SetReusePort(fd);

    struct sockaddr_storage addr_ss = ParseFromIPPort(addr.c_str());
    if (::bind(fd, (struct sockaddr*)&addr_ss, sizeof(struct sockaddr))) {
        int serrno = errno;
        LOG_ERROR("socket bind:%s error:%d %s",addr.c_str(),serrno,strerror(serrno).c_str());
        return INVALID_SOCKET;
    }

    return fd;
}

bool ParseFromIPPort(const char* address, struct sockaddr_storage& ss) {
    memset(&ss, 0, sizeof(ss));
    std::string host;
    int port;
    if (!SplitHostPort(address, host, port)) {
        return false;
    }

    short family = AF_INET;
    auto index = host.find(':');
    if (index != std::string::npos) {
        family = AF_INET6;
    }

    struct sockaddr_in* addr = sockaddr_in_cast(&ss);
    int rc = ::evutil_inet_pton(family, host.data(), &addr->sin_addr);
    if (rc == 0) {
        LOG_INFO("ParseFromIPPort evutil_inet_pton (AF_INET '%s', ...) rc=0. is not a valid IP address. Maybe it is a hostname.",host.data());
        return false;
    } else if (rc < 0) {
        int serrno = errno;
        if (serrno == 0) {
            LOG_INFO("['%s'] is not a IP address. Maybe it is a hostname.",host.data());
        } else {
            LOG_WARN("ParseFromIPPort evutil_inet_pton (AF_INET, '%s', ...) failed:%s ",host.data(),strerror(serrno).c_str());
        }
        return false;
    }

    addr->sin_family = family;
    addr->sin_port = htons(port);

    return true;
}

bool SplitHostPort(const char* address, std::string& host, int& port) {
    std::string a = address;
    if (a.empty()) {
        return false;
    }

    size_t index = a.rfind(':');
    if (index == std::string::npos) {
        LOG_ERROR("Address specified error <%s>. Cannot find ':'",address);
        return false;
    }

    if (index == a.size() - 1) {
        return false;
    }

    port = std::atoi(&a[index + 1]);

    host = std::string(address, index);
    if (host[0] == '[') {
        if (*host.rbegin() != ']') {
            LOG_ERROR("Address specified error <%s>. '[' ']' is not pair.",address);
            return false;
        }

        // trim the leading '[' and trail ']'
        host = std::string(host.data() + 1, host.size() - 2);
    }

    // Compatible with "fe80::886a:49f3:20f3:add2]:80"
    if (*host.rbegin() == ']') {
        // trim the trail ']'
        host = std::string(host.data(), host.size() - 1);
    }

    return true;
}

struct sockaddr_storage GetLocalAddr(net_socket_t sockfd) {
    struct sockaddr_storage laddr;
    memset(&laddr, 0, sizeof laddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof laddr);
    if (::getsockname(sockfd, sockaddr_cast(&laddr), &addrlen) < 0) {
        LOG_ERROR("GetLocalAddr:%s",strerror(errno).c_str());
        memset(&laddr, 0, sizeof laddr);
    }

    return laddr;
}

std::string ToIPPort(const struct sockaddr_storage* ss) {
    std::string saddr;
    int port = 0;

    if (ss->ss_family == AF_INET) {
        struct sockaddr_in* addr4 = const_cast<struct sockaddr_in*>(sockaddr_in_cast(ss));
        char buf[INET_ADDRSTRLEN] = {};
        const char* addr = ::evutil_inet_ntop(ss->ss_family, &addr4->sin_addr, buf, INET_ADDRSTRLEN);

        if (addr) {
            saddr = addr;
        }

        port = ntohs(addr4->sin_port);
    } else if (ss->ss_family == AF_INET6) {
        struct sockaddr_in6* addr6 = const_cast<struct sockaddr_in6*>(sockaddr_in6_cast(ss));
        char buf[INET6_ADDRSTRLEN] = {};
        const char* addr = ::evutil_inet_ntop(ss->ss_family, &addr6->sin6_addr, buf, INET6_ADDRSTRLEN);

        if (addr) {
            saddr = std::string("[") + addr + "]";
        }

        port = ntohs(addr6->sin6_port);
    } else {
        LOG_ERROR("unknown socket family connected");
        return empty_string;
    }

    if (!saddr.empty()) {
        saddr.append(":", 1).append(std::to_string(port));
    }

    return saddr;
}

std::string ToIPPort(const struct sockaddr* ss) {
    return ToIPPort(sockaddr_storage_cast(ss));
}

std::string ToIPPort(const struct sockaddr_in* ss) {
    return ToIPPort(sockaddr_storage_cast(ss));
}

std::string ToIP(const struct sockaddr* s) {
    auto ss = sockaddr_storage_cast(s);
    if (ss->ss_family == AF_INET) {
        struct sockaddr_in* addr4 = const_cast<struct sockaddr_in*>(sockaddr_in_cast(ss));
        char buf[INET_ADDRSTRLEN] = {};
        const char* addr = ::evutil_inet_ntop(ss->ss_family, &addr4->sin_addr, buf, INET_ADDRSTRLEN);
        if (addr) {
            return std::string(addr);
        }
    } else if (ss->ss_family == AF_INET6) {
        struct sockaddr_in6* addr6 = const_cast<struct sockaddr_in6*>(sockaddr_in6_cast(ss));
        char buf[INET6_ADDRSTRLEN] = {};
        const char* addr = ::evutil_inet_ntop(ss->ss_family, &addr6->sin6_addr, buf, INET6_ADDRSTRLEN);
        if (addr) {
            return std::string(addr);
        }
    } else {
        LOG_ERROR("unknown socket family connected ss_family:%d",ss->ss_family);
    }

    return empty_string;
}

void SetTimeout(net_socket_t fd, uint32_t timeout_ms) {
    DLOG_TRACE("fd:%d timeout_ms:%d",fd,timeout_ms);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    int ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    assert(ret == 0);
    if (ret != 0) {
        int serrno = errno;
        LOG_ERROR("setsockopt SO_RCVTIMEO ERROR:%d %s",serrno, strerror(serrno).c_str());
    }
}

void SetTimeout(net_socket_t fd, const Duration& timeout) {
    SetTimeout(fd, (uint32_t)(timeout.Milliseconds()));
}

void SetKeepAlive(net_socket_t fd, bool on) {
    int optval = on ? 1 : 0;
    DLOG_TRACE("SetKeepAlive fd:%d optval:%d",fd,optval);
    int rc = ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
                          reinterpret_cast<const char*>(&optval), static_cast<socklen_t>(sizeof optval));
    if (rc != 0) {
        int serrno = errno;
        LOG_ERROR("setsockopt(SO_KEEPALIVE) failed, errno=:%d %s",serrno, strerror(serrno).c_str());
    }
}

void SetReuseAddr(net_socket_t fd) {
    int optval = 1;
    DLOG_TRACE("SetReuseAddr fd:%d optval:%d",fd,optval);
    int rc = ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                          reinterpret_cast<const char*>(&optval), static_cast<socklen_t>(sizeof optval));
    if (rc != 0) {
        int serrno = errno;
        LOG_ERROR("setsockopt(SO_REUSEADDR) failed, errno=:%d %s",serrno, strerror(serrno).c_str());
    }
}

void SetReusePort(net_socket_t fd) {
#ifdef SO_REUSEPORT
    int optval = 1;
    DLOG_TRACE("SetReusePort fd:%d optval:%d",fd,optval);
    int rc = ::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
                          reinterpret_cast<const char*>(&optval), static_cast<socklen_t>(sizeof optval));
    LOG_INFO("setsockopt SO_REUSEPORT fd=%d rc=%d",fd,rc);
    if (rc != 0) {
        int serrno = errno;
        LOG_ERROR("setsockopt(SO_REUSEPORT) failed, errno=:%d %s",serrno, strerror(serrno).c_str());
    }
#endif
}


void SetTCPNoDelay(net_socket_t fd, bool on) {
    int optval = on ? 1 : 0;
    DLOG_TRACE("SetTCPNoDelay fd:%d optval:%d",fd,optval);
    int rc = ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                          reinterpret_cast<const char*>(&optval), static_cast<socklen_t>(sizeof optval));
    if (rc != 0) {
        int serrno = errno;
        LOG_ERROR("setsockopt(TCP_NODELAY) failed, errno=:%d %s",serrno, strerror(serrno).c_str());
    }
}

}
}

