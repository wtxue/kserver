#pragma once

#include "net/inner_pre.h"
#include "net/udp_server.h"

namespace net {
class Buffer;
class TCPConn;
class TCPServer;
class UDPServer;

typedef std::shared_ptr<TCPServer> TCPServerPtr;
typedef std::shared_ptr<TCPConn> TCPConnPtr;
typedef std::shared_ptr<UDPServer> UDPServerPtr;

typedef std::function<void()> TimerCallback;

// When a connection established, broken down, connecting failed, this callback will be called
// This is called from a work-thread this is not the listening thread probably
typedef std::function<void(const TCPConnPtr&)> ConnectionCallback;


typedef std::function<void(const TCPConnPtr&)> CloseCallback;
typedef std::function<void(const TCPConnPtr&)> WriteCompleteCallback;
typedef std::function<void(const TCPConnPtr&, size_t)> HighWaterMarkCallback;

typedef std::function<void(const TCPConnPtr&, Buffer*)> MessageCallback;

namespace internal {
inline void DefaultConnectionCallback(const TCPConnPtr&) {}
inline void DefaultMessageCallback(const TCPConnPtr&, Buffer*) {}
}

}
