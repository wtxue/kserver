#include <errno.h>
#include <linux/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h> 
#include <ctype.h>
#include "misc.h"

#if 0
#define MISC_DBG(fmt, args...)                                                                                         \
	do {                                                                                                               \
		fprintf(stderr, "[%s:%d] " fmt, __FUNCTION__, __LINE__, ##args);                                               \
	} while (0)
#else
#define MISC_DBG(fmt, args...)
#endif

namespace base {
namespace misc {

// /proc/pid/stat字段定义
struct pid_stat_fields {
    int64_t pid;
    char comm[PATH_MAX];
    char state;
    int64_t ppid;
    int64_t pgrp;
    int64_t session;
    int64_t tty_nr;
    int64_t tpgid;
    int64_t flags;
    int64_t minflt;
    int64_t cminflt;
    int64_t majflt;
    int64_t cmajflt;
    int64_t utime;
    int64_t stime;
    int64_t cutime;
    int64_t cstime;
    // ...
};

// /proc/stat/cpu信息字段定义
struct cpu_stat_fields {
    char cpu_label[16];
    int64_t user;
    int64_t nice;
    int64_t system;
    int64_t idle;
    int64_t iowait;
    int64_t irq;
    int64_t softirq;
    // ...
};

long long GetCurCpuTime() {
    char file_name[64] = { 0 };
    snprintf(file_name, sizeof(file_name), "/proc/%d/stat", getpid());
    
    FILE* pid_stat = fopen(file_name, "r");
    if (!pid_stat) {
        return -1;
    }

    pid_stat_fields result;
    int ret = fscanf(pid_stat, "%ld %s %c %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
        &result.pid, result.comm, &result.state, &result.ppid, &result.pgrp, &result.session,
        &result.tty_nr, &result.tpgid, &result.flags, &result.minflt, &result.cminflt,
        &result.majflt, &result.cmajflt, &result.utime, &result.stime, &result.cutime, &result.cstime);

    fclose(pid_stat);

    if (ret <= 0) {
        return -1;
    }

    return result.utime + result.stime + result.cutime + result.cstime;
}

long long GetTotalCpuTime() {
    char file_name[] = "/proc/stat";
    FILE* stat = fopen(file_name, "r");
    if (!stat) {
        return -1;
    }

    cpu_stat_fields result;
    int ret = fscanf(stat, "%s %ld %ld %ld %ld %ld %ld %ld",
        result.cpu_label, &result.user, &result.nice, &result.system, &result.idle,
        &result.iowait, &result.irq, &result.softirq);

    fclose(stat);

    if (ret <= 0) {
        return -1;
    }

    return result.user + result.nice + result.system + result.idle +
        result.iowait + result.irq + result.softirq;
}

float CalculateCurCpuUseage(long long cur_cpu_time_start, long long cur_cpu_time_stop,
    long long total_cpu_time_start, long long total_cpu_time_stop) {
    long long cpu_result = total_cpu_time_stop - total_cpu_time_start;
    if (cpu_result <= 0) {
        return 0;
    }

    return (sysconf(_SC_NPROCESSORS_ONLN) * 100.0f *(cur_cpu_time_stop - cur_cpu_time_start)) / cpu_result;
}

int GetCurMemoryUsage(int* vm_size_kb, int* rss_size_kb) {
    if (!vm_size_kb || !rss_size_kb) {
        return -1;
    }

    char file_name[64] = { 0 };
    snprintf(file_name, sizeof(file_name), "/proc/%d/status", getpid());
    
    FILE* pid_status = fopen(file_name, "r");
    if (!pid_status) {
        return -1;
    }

    int ret = 0;
    char line[256] = { 0 };
    char tmp[32]   = { 0 };
    fseek(pid_status, 0, SEEK_SET);
    for (int i = 0; i < 16; i++) {
        if (fgets(line, sizeof(line), pid_status) == NULL) {
            ret = -2;
            break;
        }

        if (strstr(line, "VmSize") != NULL) {
            sscanf(line, "%s %d", tmp, vm_size_kb);
        } else if (strstr(line, "VmRSS") != NULL) {
            sscanf(line, "%s %d", tmp, rss_size_kb);
        }
    }

    fclose(pid_status);

    return ret;
}

int b2str(const char* buf, const int bufLen, char *str, const int strLen, bool capital)
{
	const char* format = capital ? "%02X" : "%02x";
	int index = 0;
	for (int i = 0; i < bufLen && index < strLen - 2; i++, index += 2)
	{
		sprintf(str + index, format, (unsigned char)buf[i]);
	}
	str[index] = '\0';
	return index;
}

bool strIsUpperHex(const char *str, const int strLen)
{
	int len = strLen ? strLen : (int)strlen(str);
	for (int i = 0; i < len; i++)
	{
		if (!(isdigit(str[i]) || (str[i] >= 'A' && str[i] <= 'F')))
		{
			return false;
		}
	}

	return true;
}

TrueRandom::TrueRandom()
    : m_fd(-1) {
    m_fd = open("/dev/urandom", O_RDONLY, 0);
    if (m_fd < 0) {
        MISC_DBG("err m_fd:%d",m_fd);
        //assert(m_fd < 0);
    }
}

TrueRandom::~TrueRandom() {
    close(m_fd);
    m_fd = -1;
}

bool TrueRandom::NextBytes(void* buffer, size_t size) {
    return read(m_fd, buffer, size) == static_cast<int32_t>(size);
}

uint32_t TrueRandom::NextUInt32() {
    uint32_t random = -1;
    NextBytes(&random, sizeof(random));
    return random;
}

uint32_t TrueRandom::NextUInt32(uint32_t max_random) {
    return NextUInt32() % max_random;
}

}
}
