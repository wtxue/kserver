#pragma once

#include "inner_pre.h"

#if defined(HTTP_CLIENT_SUPPORTS_SSL)

#include <openssl/ssl.h>

namespace net {
namespace httpc {
bool InitSSL();
void CleanSSL();
SSL_CTX* GetSSLCtx();
} // httpc
} // net

#endif
