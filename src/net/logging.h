#pragma once

#include "net/platform_config.h"
#include "rlog.h"  

#if 1
#define LOG_TRACE(...)     _LOG_HEAD("[KTRACE]",__VA_ARGS__)
#define LOG_DEBUG(...)     _LOG_HEAD("[KDEBUG]",__VA_ARGS__)
#define LOG_INFO(...)      _LOG_HEAD("[K INFO]",__VA_ARGS__)
#define LOG_WARN(...)      _LOG_HEAD("[K WARN]",__VA_ARGS__)
#define LOG_ERROR(...)     _LOG_HEAD("[KERROR]",__VA_ARGS__)
#define LOG_FATAL(...)     _LOG_HEAD("[KFATAL]",__VA_ARGS__)
#define DLOG_TRACE(...)    _LOG_HEAD("[DTRACE]",__VA_ARGS__)
#define DLOG_WARN(...)     _LOG_HEAD("[D WARN]",__VA_ARGS__)
#else
#define LOG_TRACE(...)
#define LOG_DEBUG(...)
#define LOG_INFO(...) 
#define LOG_WARN(...)
#define LOG_ERROR(...)
#define LOG_FATAL(...)
#define DLOG_TRACE(...)
#define DLOG_WARN(...)
#endif

