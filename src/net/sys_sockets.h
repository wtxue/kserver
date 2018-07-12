#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/uio.h>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1    /**< invalid socket definition */
#endif


/* True iff e is an error that means a read/write operation can be retried. */
#define EVUTIL_ERR_RW_RETRIABLE(e)              \
    ((e) == EINTR || (e) == EAGAIN)
/* True iff e is an error that means an connect can be retried. */
#define EVUTIL_ERR_CONNECT_RETRIABLE(e)         \
    ((e) == EINTR || (e) == EINPROGRESS)
/* True iff e is an error that means a accept can be retried. */
#define EVUTIL_ERR_ACCEPT_RETRIABLE(e)          \
    ((e) == EINTR || (e) == EAGAIN || (e) == ECONNABORTED)

/* True iff e is an error that means the connection was refused */
#define EVUTIL_ERR_CONNECT_REFUSED(e)                   \
    ((e) == ECONNREFUSED)


#define net_socket_t int
#define signal_number_t net_socket_t

