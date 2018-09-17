#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#include "codec.h"
#include "event_loop.h"
#include "tcp_server.h"
#include "CommDef.h"

using namespace std;
using namespace base;
using namespace net;


class Server {
public:
    Server(net::EventLoop* loop) 
		: base_loop_(loop)
		, next_conn_id_(0)
		, codec_(std::bind(&Server::OnMessage, this, std::placeholders::_1, std::placeholders::_2)) {
		LOG_TRACE("create Server");  
	}
		 
	~Server() {
		DLOG_TRACE("destroy Server ");
	}

    void Init();
	void CreateLoopSqdbClTimer();
    void OnConnection(const net::TCPConnPtr& conn);
    void OnMessage(const net::TCPConnPtr& conn, MsgTlvBoxPtr& Message);

private:
	net::EventLoop* base_loop_;	
	net::TCPServerPtr hostServer_;
	net::EventLoopThreadPoolPtr serverPool_;	

    uint64_t next_conn_id_;
    typedef std::map<uint64_t, TCPConnPtr> ConnectionMap;
    ConnectionMap conns_;

	LengthHeaderCodec codec_;
};


