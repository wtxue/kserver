#pragma once

#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
#include <iostream>
#include <memory>
#include <functional>
#endif // end of define __cplusplus

#ifndef H_DEBUG_MODE
#define H_DEBUG_MODE
#endif

#ifndef HTTP_CLIENT_SUPPORTS_SSL
#define HTTP_CLIENT_SUPPORTS_SSL
#endif

#include "sys_addrinfo.h"
#include "sys_sockets.h"
#include "sockets.h"
#include "logging.h"


struct event;
namespace net {
    int EventAdd(struct event* ev, const struct timeval* timeout);
    int EventDel(struct event*);
    int GetActiveEventCount();
}
