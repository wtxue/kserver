#pragma once

#include "tcp_conn.h"
#include "buffer.h"
#include "event_loop.h"
#include "tcp_server.h"
#include "tcp_callbacks.h"
#include "tlv_box.h"

using namespace std;
using namespace base;
using namespace net;

namespace base {

#define TLV_MARK "TLVC"


class MsgTlvBox;
typedef std::shared_ptr<MsgTlvBox> MsgTlvBoxPtr;

class MsgTlvBox  : public std::enable_shared_from_this<MsgTlvBox> {
public:
    MsgTlvBox(net::EventLoop* loop, const net::TCPConnPtr& conn) 
		: loop_(loop) 
		, conn_(conn) {	
		DLOG_TRACE("create MsgTlvBox conn name:%s",conn->name().c_str());
	}

	MsgTlvBox() { 
		DLOG_TRACE("create MsgTlvBox ");
	}

	~MsgTlvBox() {
		DLOG_TRACE("destroy MsgTlvBox ");
	}

	void SetLoop(net::EventLoop* loop,const net::TCPConnPtr& conn) {
		loop_ = loop;
		conn_ = conn;
	}

    base::TlvBox& GetTlv()    {
        return TlvBox_;
    }

    net::TCPConnPtr& GetConn()    {
        return conn_;
    }

private:
	net::EventLoop* loop_;
	net::TCPConnPtr conn_;
	base::TlvBox TlvBox_;
};

class LengthHeaderCodec {
public:
    typedef std::function<void(const net::TCPConnPtr&, MsgTlvBoxPtr& Message)> TlvMessageCallback;

    explicit LengthHeaderCodec(const TlvMessageCallback& cb)
        : messageCallback_(cb) {}

    void OnMessage(const net::TCPConnPtr& conn, net::Buffer* buf) {
        while (buf->size() >= (kHeaderTlvMarkLen + kHeaderTlvHeadLen)) {
            buf->Skip(kHeaderTlvMarkLen);
            const int32_t len = buf->PeekInt32();
            if (len > 65536 || len < 0) {
                LOG_ERROR("Invalid length:%d", len);
                conn->Close();
                break;
            }

            LOG_DEBUG("PeekInt32 len:0x%04x",len);
            if (buf->size() >= len + kHeaderTlvHeadLen) {
                buf->Skip(kHeaderTlvHeadLen);
                MsgTlvBoxPtr Message(new MsgTlvBox());	
                Message->GetTlv().Parse((const unsigned char*)(buf->data()), len);
				buf->Skip(len);
                messageCallback_(conn, Message);
                break;
            } else {
                break;
            }
        }
    }

    void Send(net::TCPConnPtr& conn, const TlvBox& message) {
        net::Buffer buf;
        buf.Append(message.GetSerializedBuffer(), message.GetSerializedBytes());
        buf.PrependInt32(message.GetSerializedBytes());
        buf.Prepend(TLV_MARK,kHeaderTlvMarkLen);
        conn->Send(&buf);
    }

private:
    TlvMessageCallback messageCallback_;
    const static size_t kHeaderTlvMarkLen = 4;  /* TLV_MARK "TLVC" */
    const static size_t kHeaderTlvHeadLen = 4;  
};


}
