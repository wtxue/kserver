#pragma once

#include "net/sys_addrinfo.h"
#include "net/sys_sockets.h"

#include <string.h>

namespace net {

class Duration;

std::string strerror(int e);

namespace sock {

net_socket_t CreateNonblockingSocket();
net_socket_t CreateUDPServer(const std::string& addr);
void SetKeepAlive(net_socket_t fd, bool on);
void SetReuseAddr(net_socket_t fd);
void SetReusePort(net_socket_t fd);
void SetTCPNoDelay(net_socket_t fd, bool on);
void SetTimeout(net_socket_t fd, uint32_t timeout_ms);
void SetTimeout(net_socket_t fd, const Duration& timeout);
std::string ToIPPort(const struct sockaddr_storage* ss);
std::string ToIPPort(const struct sockaddr* ss);
std::string ToIPPort(const struct sockaddr_in* ss);
std::string ToIP(const struct sockaddr* ss);


// @brief Parse a literal network address and return an internet protocol family address
// @param[in] address - A network address of the form "host:port" or "[host]:port"
// @return bool - false if parse failed.
bool ParseFromIPPort(const char* address, struct sockaddr_storage& ss);

inline struct sockaddr_storage ParseFromIPPort(const char* address) {
    struct sockaddr_storage ss;
    bool rc = ParseFromIPPort(address, ss);
    if (rc) {
        return ss;
    } else {
        memset(&ss, 0, sizeof(ss));
        return ss;
    }
}

// @brief Splits a network address of the form "host:port" or "[host]:port"
//  into host and port. A literal address or host name for IPv6
// must be enclosed in square brackets, as in "[::1]:80" or "[ipv6-host]:80"
// @param[in] address - A network address of the form "host:port" or "[ipv6-host]:port"
// @param[out] host -
// @param[out] port - the port in local machine byte order
// @return bool - false if the network address is invalid format
bool SplitHostPort(const char* address, std::string& host, int& port);

struct sockaddr_storage GetLocalAddr(net_socket_t sockfd);

inline bool IsZeroAddress(const struct sockaddr_storage* ss) {
    const char* p = reinterpret_cast<const char*>(ss);
    for (size_t i = 0; i < sizeof(*ss); ++i) {
        if (p[i] != 0) {
            return false;
        }
    }
    return true;
}

template<typename To, typename From>
inline To implicit_cast(From const& f) {
    return f;
}

inline const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr) {
    return static_cast<const struct sockaddr*>(net::sock::implicit_cast<const void*>(addr));
}

inline struct sockaddr* sockaddr_cast(struct sockaddr_in* addr) {
    return static_cast<struct sockaddr*>(net::sock::implicit_cast<void*>(addr));
}

inline struct sockaddr* sockaddr_cast(struct sockaddr_storage* addr) {
    return static_cast<struct sockaddr*>(net::sock::implicit_cast<void*>(addr));
}

inline const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr) {
    return static_cast<const struct sockaddr_in*>(net::sock::implicit_cast<const void*>(addr));
}

inline struct sockaddr_in* sockaddr_in_cast(struct sockaddr* addr) {
    return static_cast<struct sockaddr_in*>(net::sock::implicit_cast<void*>(addr));
}

inline struct sockaddr_in* sockaddr_in_cast(struct sockaddr_storage* addr) {
    return static_cast<struct sockaddr_in*>(net::sock::implicit_cast<void*>(addr));
}

inline struct sockaddr_in6* sockaddr_in6_cast(struct sockaddr_storage* addr) {
    return static_cast<struct sockaddr_in6*>(net::sock::implicit_cast<void*>(addr));
}

inline const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr_storage* addr) {
    return static_cast<const struct sockaddr_in*>(net::sock::implicit_cast<const void*>(addr));
}

inline const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr_storage* addr) {
    return static_cast<const struct sockaddr_in6*>(net::sock::implicit_cast<const void*>(addr));
}

inline const struct sockaddr_storage* sockaddr_storage_cast(const struct sockaddr* addr) {
    return static_cast<const struct sockaddr_storage*>(net::sock::implicit_cast<const void*>(addr));
}

inline const struct sockaddr_storage* sockaddr_storage_cast(const struct sockaddr_in* addr) {
    return static_cast<const struct sockaddr_storage*>(net::sock::implicit_cast<const void*>(addr));
}

inline const struct sockaddr_storage* sockaddr_storage_cast(const struct sockaddr_in6* addr) {
    return static_cast<const struct sockaddr_storage*>(net::sock::implicit_cast<const void*>(addr));
}

}

}

