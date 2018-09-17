#include "net/inner_pre.h"
#include "net/libevent.h"
#include <signal.h>
#include <map>
#include <thread>
#include <mutex>

namespace net {

namespace {
struct OnStartup {
    OnStartup() {
        if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
            LOG_ERROR("SIGPIPE set failed.");
            exit(-1);
        }
        LOG_INFO("ignore SIGPIPE");
    }
    ~OnStartup() {
    }
} __s_onstartup;
}


#ifdef H_DEBUG_MODE
static std::map<struct event*, std::thread::id> evmap;
static std::mutex mutex;
#endif

int EventAdd(struct event* ev, const struct timeval* timeout) {
#ifdef H_DEBUG_MODE
    {
        std::lock_guard<std::mutex> guard(mutex);
        if (evmap.find(ev) == evmap.end()) {
            auto id = std::this_thread::get_id();
            evmap[ev] = id;
        } else {
            LOG_ERROR("EventAdd ev:%p fd:%d event_add twice!",ev,ev->ev_fd);
            assert("event_add twice");
        }
    }
    //LOG_TRACE("EventAdd ev:%p fd:%d user_ptr:%p tid:%lu",ev,ev->ev_fd,ev->ev_arg,std::this_thread::get_id());
#endif
    return event_add(ev, timeout);
}

int EventDel(struct event* ev) {
#ifdef H_DEBUG_MODE
    {
        std::lock_guard<std::mutex> guard(mutex);
        auto it = evmap.find(ev);
        if (it == evmap.end()) {
            LOG_ERROR("EventDel ev:%p fd:%d not exist in event loop, maybe event_del twice.",ev,ev->ev_fd);
            assert("event_del twice");
        } else {
            auto id = std::this_thread::get_id();
            if (id != it->second) {
                LOG_ERROR("EventDel ev:%p fd:%d deleted in different thread.",ev,ev->ev_fd);
                assert(it->second == id);
            }
            evmap.erase(it);
        }
    }
    //LOG_TRACE("EventDel ev:%p fd:%d user_ptr:%p tid:%lu",ev,ev->ev_fd,ev->ev_arg,std::this_thread::get_id());
#endif
    return event_del(ev);
}

int GetActiveEventCount() {
#ifdef H_DEBUG_MODE
	int evmap_size = evmap.size();
	LOG_DEBUG("evmap_size:%d tid:%lu",evmap_size,std::this_thread::get_id());
    return evmap_size;
#else
    return 0;
#endif
}

}
