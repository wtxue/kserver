#ifndef __MISC_H__
#define __MISC_H__

namespace base {
namespace misc {

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

	
class TrueRandom {
    DECLARE_UNCOPYABLE(TrueRandom);

public:
    TrueRandom();
    ~TrueRandom();

    uint32_t NextUInt32();

    uint32_t NextUInt32(uint32_t max_random);

    bool NextBytes(void* buffer, size_t size);

private:
    int32_t m_fd;
}; // class TrueRandom

} 
}
#endif