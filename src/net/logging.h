#pragma once

#include "logger.h"  

#if 1
#define LOG_TRACE(...)     _TRACE(__VA_ARGS__)
#define LOG_DEBUG(...)     _DEBUG(__VA_ARGS__)
#define LOG_INFO(...)      _INFO(__VA_ARGS__)
#define LOG_WARN(...)      _WARN(__VA_ARGS__)
#define LOG_ERROR(...)     _ERROR(__VA_ARGS__)
#define LOG_FATAL(...)     _LOG_FATAL(__VA_ARGS__)
#define DLOG_TRACE(...)    _TRACE(__VA_ARGS__)
#else
#define LOG_TRACE(...)
#define LOG_DEBUG(...)
#define LOG_INFO(...) 
#define LOG_WARN(...)
#define LOG_ERROR(...)
#define LOG_FATAL(...)
#define DLOG_TRACE(...)
#endif

