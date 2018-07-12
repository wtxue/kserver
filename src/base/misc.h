#ifndef __MISC_H__
#define __MISC_H__

namespace base {

/// @brief 获取当前进程的cpu使用时间
long long GetCurCpuTime();

/// @brief 获取整个系统的cpu使用时间
long long GetTotalCpuTime();

/// @brief 计算进程的cpu使用率
/// @param cur_cpu_time_start 当前进程cpu使用时间段-start
/// @param cur_cpu_time_stop 当前进程cpu使用时间段-stop
/// @param total_cpu_time_start 整个系统cpu使用时间段-start
/// @param total_cpu_time_start 整个系统cpu使用时间段-stop
float CalculateCurCpuUseage(long long cur_cpu_time_start, long long cur_cpu_time_stop,
    long long total_cpu_time_start, long long total_cpu_time_stop);



/// @brief 获取进程当前内存使用情况
/// @param vm_size_kb 输出参数，虚存，单位为K
/// @param rss_size_kb 输出参数，物理内存，单位为K
int GetCurMemoryUsage(int* vm_size_kb, int* rss_size_kb);

int b2str(const char* buf, const int bufLen, char *str, const int strLen, bool capital);

bool strIsUpperHex(const char *str, const int strLen);

int string2ll(const char *s, size_t slen, long long *value);

int string2l(const char *s, size_t slen, long *lval);

uint64_t base_mktime(char *stime);
	
class TrueRandom {

public:
    TrueRandom();
    ~TrueRandom();

    uint32_t NextUInt32();

    uint32_t NextUInt32(uint32_t max_random);

    bool NextBytes(void* buffer, size_t size);

	bool NextBytesBinStr(std::string & binStr, size_t size);

private:
    int32_t m_fd;
}; // class TrueRandom


#if 0
//time_interval.h
#pragma once

#include <iostream>
#include <memory>
#include <string>
#ifdef GCC
#include <sys/time.h>
#else
#include <ctime>
#endif // GCC

class TimeInterval
{
public:
    TimeInterval(const std::string& d) : detail(d)
    {
        init();
    }

    TimeInterval()
    {
        init();
    }

    ~TimeInterval()
    {
#ifdef GCC
        gettimeofday(&end, NULL);
        std::cout << detail 
            << 1000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000 
            << " ms" << endl;
#else
        end = clock();
        std::cout << detail 
            << (double)(end - start) << " ms" << std::endl;
#endif // GCC
    }

protected:
    void init() {
#ifdef GCC
        gettimeofday(&start, NULL);
#else
        start = clock();
#endif // GCC
    }
private:
    std::string detail;
#ifdef GCC
    timeval start, end;
#else
    clock_t start, end;
#endif // GCC
};

#define TIME_INTERVAL_SCOPE(d)   std::shared_ptr<TimeInterval> time_interval_scope_begin = std::make_shared<TimeInterval>(d)
#endif
}

#endif