#include <iostream> 
#include <errno.h>
#include <linux/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h> 
#include <ctype.h>
#include <limits.h>
#include <time.h>

#include "logger.h"
#include "misc.h"

#if 1
#define MISC_DBG(...)     _DEBUG(__VA_ARGS__)
#else
#define MISC_DBG(...)
#endif

namespace base {

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

int b2str(const char* buf, const int bufLen, char *str, const int strLen, bool capital) {
	const char* format = capital ? "%02X" : "%02x";
	int index = 0;
	
	for (int i = 0; i < bufLen && index < strLen - 2; i++, index += 2) 	{
		snprintf((str + index), (strLen - index - 1), format, (unsigned char)buf[i]);
	}
	str[index] = '\0';
	return index;
}

bool strIsUpperHex(const char *str, const int strLen) {
	int len = strLen ? strLen : (int)strlen(str);
	for (int i = 0; i < len; i++) {
		if (!(isdigit(str[i]) || (str[i] >= 'A' && str[i] <= 'F'))) {
			return false;
		}
	}

	return true;
}

int string2ll(const char *s, size_t slen, long long *value) {
    const char *p = s;
    size_t plen = 0;
    int negative = 0;
    unsigned long long v = 0;

    if (plen == slen)
        return 0;

    /* Special case: first and only digit is 0. */
    if (slen == 1 && p[0] == '0') {
        if (value != NULL) *value = 0;
        return 1;
    }

    if (p[0] == '-') {
        negative = 1;
        p++; plen++;

        /* Abort on only a negative sign. */
        if (plen == slen)
            return 0;
    }

    /* First digit should be 1-9, otherwise the string should just be 0. */
    if (p[0] >= '1' && p[0] <= '9') {
        v = p[0]-'0';
        p++; plen++;
    } else if (p[0] == '0' && slen == 1) {
        *value = 0;
        return 1;
    } 	
#if 0	
	else {
        return 0;
    }
#endif

    while (plen < slen && p[0] >= '0' && p[0] <= '9') {
        if (v > (ULLONG_MAX / 10)) /* Overflow. */
            return 0;
        v *= 10;

        if (v > (ULLONG_MAX - (p[0]-'0'))) /* Overflow. */
            return 0;
        v += p[0]-'0';

        p++; plen++;
    }

    /* Return if not all bytes were used. */
    if (plen < slen)
        return 0;

    if (negative) {
        if (v > ((unsigned long long)(-(LLONG_MIN+1))+1)) /* Overflow. */
            return 0;
        if (value != NULL) *value = -v;
    } else {
        if (v > LLONG_MAX) /* Overflow. */
            return 0;
        if (value != NULL) *value = v;
    }
    return 1;
}

int string2l(const char *s, size_t slen, long *lval) {
    long long llval;

    if (!string2ll(s,slen,&llval))
        return 0;

    if (llval < LONG_MIN || llval > LONG_MAX)
        return 0;

    *lval = (long)llval;
    return 1;
}

uint64_t base_mktime(char *stime) {
	struct tm info;
	long year = 0;
	long mon  = 0;
	long day  = 0;
	long hour = 0;
	time_t time = 0;
	char *s = stime;

	if (!string2l(s, 4, &year))
		return 0;
	
	s += 4;
	if (!string2l(s, 2, &mon))
		return 0;
	
	s += 2;
	if (!string2l(s, 2, &day))
		return 0;
	
	s += 2;
	if (!string2l(s, 2, &hour))
		hour = 0;

	//MISC_DBG("year:%d mon:%d day:%d hour:%d\n",year,mon,day,hour);
	memset(&info,0,sizeof(info));
	info.tm_year = year - 1900;
	info.tm_mon = mon - 1;
	info.tm_mday = day;
	info.tm_hour = hour;
	
	time = mktime(&info);
	if (time <= 0)
		return 0;
	else
		return time;
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

bool TrueRandom::NextBytesBinStr(std::string & binStr, size_t size) {
	int binSize = size/2 + 1;
	int strSize = size + 2;

	
	//MISC_DBG("binSize:%d strSize:%d",binSize,strSize);
	char buf[binSize];	
	char strBuf[strSize];
	
    if (static_cast<int32_t>(binSize) != read(m_fd, buf, binSize)) {
    	MISC_DBG("read rand data err fd:%d",m_fd);
    	return false;
    }

	b2str(buf, binSize, strBuf, strSize, 1);
	binStr = strBuf;
	return true;
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

