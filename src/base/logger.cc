#include <errno.h>
#include <unistd.h>//access, getpid
#include <assert.h>//assert
#include <stdarg.h>//va_list
#include <sys/stat.h>//mkdir
#include <sys/syscall.h>//system call
#include <sys/prctl.h> 
#include "logger.h"

#define MEM_USE_LIMIT (1u * 1024 * 1024 * 1024)//3GB
#define LOG_USE_LIMIT (512 * 1024 * 1024)//512MB
#define LOG_LEN_LIMIT (10 * 1024)//10K
#define RELOG_THRESOLD 5
#define BUFF_WAIT_TIME 1

#if 1
#define RLOG_DBG(fmt, args...)                                                                                         \
	do {                                                                                                               \
		fprintf(stderr, "[%s:%d] " fmt, __FUNCTION__, __LINE__, ##args);                                               \
	} while (0)
#else
#define RLOG_DBG(fmt, args...)
#endif

/* 线程私有数据 */
__thread char ThreadName[64] = {0};

ring_log* ring_log::_ins = NULL;

pthread_mutex_t ring_log::_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ring_log::_cond = PTHREAD_COND_INITIALIZER;

pthread_once_t ring_log::_once = PTHREAD_ONCE_INIT;
uint32_t ring_log::_one_buff_len = 30*1024*1024;//30MB

ring_log::ring_log():
    _buff_cnt(3),
    _curr_buf(NULL),
    _prst_buf(NULL),
    _fp(NULL),
    _log_cnt(0),
    _env_ok(false),
    _level(INFO),
    _lst_lts(0),
    _tm() {
    //create double linked list
    cell_buffer* head = new cell_buffer(_one_buff_len);
    if (!head) {
        RLOG_DBG("no space to allocate cell_buffer\n");
        exit(1);
    }
    cell_buffer* current;
    cell_buffer* prev = head;
    for (int i = 1;i < _buff_cnt; ++i) {
        current = new cell_buffer(_one_buff_len);
        if (!current) {
            RLOG_DBG("no space to allocate cell_buffer\n");
            exit(1);
        }
        current->prev = prev;
        prev->next = current;
        prev = current;
    }
    prev->next = head;
    head->prev = prev;

    _curr_buf = head;
    _prst_buf = head;

    _pid = getpid();
}

void ring_log::init_path(const char* log_dir, const char* prog_name, bool is_console, int level) {
	char _bak_log_dir[512] = {0};
	
    pthread_mutex_lock(&_mutex);
    if (is_console) {
	    _is_console = true;
		goto UNLOCK;
	}

    strncpy(_prog_name, prog_name, 128);
    strncpy(_log_dir, log_dir, 512);

    mkdir(_log_dir, 0777);    
    if (access(_log_dir, F_OK | W_OK) == -1) {
        RLOG_DBG("logdir: %s error: %s\n", _log_dir, strerror(errno));
    }
    else {
        _env_ok = true;
    }
    
	snprintf(_bak_log_dir,511,"%s/%s",_log_dir,"bak");
	mkdir(_bak_log_dir, 0777); 
	RLOG_DBG("mkdir: %s\n",_bak_log_dir);
	
    if (level > TRACE)
        level = TRACE;
    if (level < FATAL)
        level = FATAL;

UNLOCK:       
    _level = level;
    pthread_mutex_unlock(&_mutex);
}

int ring_log::set_level(int level)	{ 
	if (level > TRACE || level < 0)
		return -1;
		
	pthread_mutex_lock(&_mutex);
	_level = level; 		
	pthread_mutex_unlock(&_mutex);
	return 0; 
}

void ring_log::persist() {
    while (true) {
        //check if _prst_buf need to be persist
        pthread_mutex_lock(&_mutex);
        if (_prst_buf->status == cell_buffer::FREE) {
            struct timespec tsp;
            struct timeval now;
            gettimeofday(&now, NULL);
            tsp.tv_sec = now.tv_sec;
            tsp.tv_nsec = now.tv_usec * 1000;//nanoseconds
            tsp.tv_sec += BUFF_WAIT_TIME;//wait for 1 seconds
            pthread_cond_timedwait(&_cond, &_mutex, &tsp);
        }
        if (_prst_buf->empty()) {
            //give up, go to next turn
            pthread_mutex_unlock(&_mutex);
            continue;
        }

        if (_prst_buf->status == cell_buffer::FREE) {
            assert(_curr_buf == _prst_buf);//to test
            _curr_buf->status = cell_buffer::FULL;
            _curr_buf = _curr_buf->next;
        }

        int year = _tm.year, mon = _tm.mon, day = _tm.day;
        pthread_mutex_unlock(&_mutex);

        //decision which file to write
        if (!decis_file(year, mon, day))
            continue;
        //write
        _prst_buf->persist(_fp);
        fflush(_fp);

        pthread_mutex_lock(&_mutex);
        _prst_buf->clear();
        _prst_buf = _prst_buf->next;
        pthread_mutex_unlock(&_mutex);
    }
}

char *ring_log::GetThreadName() {
	if (!ThreadName[0]) {
		prctl(PR_GET_NAME, ThreadName);
		//RLOG_DBG("\n///////////////////////////////////////////ThreadName:%s\n",ThreadName);
	}
	return ThreadName;
}

void ring_log::try_append(const char* lvl, const char* format, ...) {
    long ws;
    uint64_t curr_sec = _tm.get_curr_time(&ws);
    if (_lst_lts && curr_sec - _lst_lts < RELOG_THRESOLD)
        return ;

    char log_line[LOG_LEN_LIMIT];
    int prev_len = snprintf(log_line, LOG_LEN_LIMIT, "%s[%s.%06ld][%s]", lvl, _tm.utc_fmt, ws, GetThreadName());

    va_list arg_ptr;
    va_start(arg_ptr, format);

    //TO OPTIMIZE IN THE FUTURE: performance too low here!
    int main_len = vsnprintf(log_line + prev_len, LOG_LEN_LIMIT - prev_len, format, arg_ptr);

    va_end(arg_ptr);

    if (_is_console) {
		fprintf(stderr,"%s",log_line);
		fflush(stderr);
		return;
    }

    uint32_t len = prev_len + main_len;

    _lst_lts = 0;
    bool tell_back = false;

    pthread_mutex_lock(&_mutex);
    if (_curr_buf->status == cell_buffer::FREE && _curr_buf->avail_len() >= len) {
        _curr_buf->append(log_line, len);
    }
    else {
        //1. _curr_buf->status = cell_buffer::FREE but _curr_buf->avail_len() < len
        //2. _curr_buf->status = cell_buffer::FULL
        if (_curr_buf->status == cell_buffer::FREE) {
            _curr_buf->status = cell_buffer::FULL;//set to FULL
            cell_buffer* next_buf = _curr_buf->next;
            //tell backend thread
             tell_back = true;

            //it suggest that this buffer is under the persist job
            if (next_buf->status == cell_buffer::FULL) {
                //if mem use < MEM_USE_LIMIT, allocate new cell_buffer
                if (_one_buff_len * (_buff_cnt + 1) > MEM_USE_LIMIT) {
                    RLOG_DBG("no more log space can use\n");
                    _curr_buf = next_buf;
                    _lst_lts = curr_sec;
                }
                else {
                    cell_buffer* new_buffer = new cell_buffer(_one_buff_len);
                    _buff_cnt += 1;
                    new_buffer->prev = _curr_buf;
                    _curr_buf->next = new_buffer;
                    new_buffer->next = next_buf;
                    next_buf->prev = new_buffer;
                    _curr_buf = new_buffer;
                }
            }
            else {
                //next buffer is free, we can use it
                _curr_buf = next_buf;
            }
            if (!_lst_lts)
                _curr_buf->append(log_line, len);
        }
        else { //_curr_buf->status == cell_buffer::FULL, assert persist is on here too!
            _lst_lts = curr_sec;
        }
    }
    pthread_mutex_unlock(&_mutex);
    if (tell_back) {
        pthread_cond_signal(&_cond);
    }
}

bool ring_log::decis_file(int year, int mon, int day) {
	char _cmdline[1024] = {0};
	
    if (!_env_ok) {
        if (_fp)
            fclose(_fp);
        _fp = NULL;
        return false;
    }
    if (!_fp) {
    	snprintf(_cmdline,1023,"rm -f %s/bak/%s.*; mv -f %s/%s.* %s/bak/",_log_dir,_prog_name,_log_dir,_prog_name,_log_dir);
    	system(_cmdline);
    	RLOG_DBG("cmd:%s\n",_cmdline);
    	
        _year = year, _mon = mon, _day = day;
        char log_path[1024] = {};
        sprintf(log_path, "%s/%s.%d%02d%02d.%u.log", _log_dir, _prog_name, _year, _mon, _day, _pid);
        _fp = fopen(log_path, "w");
        if (_fp)
            _log_cnt += 1;
    }
    else if (_day != day) {
        fclose(_fp);

		snprintf(_cmdline,1023,"rm -f %s/bak/%s.*; mv -f %s/%s.* %s/bak/",_log_dir,_prog_name,_log_dir,_prog_name,_log_dir);
    	system(_cmdline);
    	RLOG_DBG("cmd:%s\n",_cmdline);
    	
        char log_path[1024] = {};
        _year = year, _mon = mon, _day = day;
        sprintf(log_path, "%s/%s.%d%02d%02d.%u.log", _log_dir, _prog_name, _year, _mon, _day, _pid);
        _fp = fopen(log_path, "w");
        if (_fp)
            _log_cnt = 1;
    }
    else if (ftell(_fp) >= LOG_USE_LIMIT) {
        fclose(_fp);
        char old_path[1024] = {};
        char new_path[1024] = {};
        //mv xxx.log.[i] xxx.log.[i + 1]
        for (int i = _log_cnt - 1;i > 0; --i) {
            sprintf(old_path, "%s/%s.%d%02d%02d.%u.log.%d", _log_dir, _prog_name, _year, _mon, _day, _pid, i);
            sprintf(new_path, "%s/%s.%d%02d%02d.%u.log.%d", _log_dir, _prog_name, _year, _mon, _day, _pid, i + 1);
            rename(old_path, new_path);
        }
        //mv xxx.log xxx.log.1
        sprintf(old_path, "%s/%s.%d%02d%02d.%u.log", _log_dir, _prog_name, _year, _mon, _day, _pid);
        sprintf(new_path, "%s/%s.%d%02d%02d.%u.log.1", _log_dir, _prog_name, _year, _mon, _day, _pid);
        rename(old_path, new_path);
        _fp = fopen(old_path, "w");
        if (_fp)
            _log_cnt += 1;
    }
    return _fp != NULL;
}

void* be_thdo(void* args) {
	args = args;	
    prctl(PR_SET_NAME, "logger");
    ring_log::ins()->persist();
    return NULL;
}
