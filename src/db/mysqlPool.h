#ifndef _MYSQL_POOL_H_
#define _MYSQL_POOL_H_

#include <list>
#include <memory>
#include <iostream>

#include <mysql.h>
#include <pthread.h>

#include "logger.h"
#include "misc.h"
#include "config_reader.h"


#if 1
#define SQL_TRACE(...)     _TRACE(__VA_ARGS__)
#define SQL_DEBUG(...)     _DEBUG(__VA_ARGS__)
#define SQL_ERROR(...)     _ERROR(__VA_ARGS__)
#else
#define SQL_TRACE(...)
#define SQL_DEBUG(...)
#define SQL_ERROR(...) 
#endif

using namespace std;
using namespace base;

#define MAX_ESCAPE_STRING_LEN	10240

class CResultSet;
typedef std::shared_ptr<CResultSet> CResultSetPtr;


class CThreadNotify {
public:
	CThreadNotify() {
		pthread_mutexattr_init(&m_mutexattr);
		pthread_mutexattr_settype(&m_mutexattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&m_mutex, &m_mutexattr);		
		pthread_cond_init(&m_cond, NULL);
	}
	
	~CThreadNotify() {
		pthread_mutexattr_destroy(&m_mutexattr);
		pthread_mutex_destroy(&m_mutex);		
		pthread_cond_destroy(&m_cond);
	}
	
	void Lock() { pthread_mutex_lock(&m_mutex); }
	void Unlock() { pthread_mutex_unlock(&m_mutex); }
	void Wait() { pthread_cond_wait(&m_cond, &m_mutex); }
	void Signal() { pthread_cond_signal(&m_cond); }
private:
	pthread_mutex_t 	m_mutex;
	pthread_mutexattr_t	m_mutexattr;    
	pthread_cond_t 		m_cond;
};


class CResultSet {
public:
	CResultSet(MYSQL_RES* res);
	virtual ~CResultSet();

	bool Next();
	int GetInt(const char* key);
	uint64_t GetLong(const char* key);
	char* GetString(const char* key);
	bool GetString(const char* key, string &val);
	
private:
	int _GetIndex(const char* key);

	MYSQL_RES* 			m_res;
	MYSQL_ROW			m_row;
	map<string, int>	m_key_map;
};

/*
 * TODO:
 * 用MySQL的prepare statement接口来防止SQL注入
 */
class CPrepareStatement {
public:
	CPrepareStatement();
	virtual ~CPrepareStatement();

	bool Init(MYSQL* mysql, string& sql);

	void SetParam(uint32_t index, int& value);
	void SetParam(uint32_t index, uint32_t& value);
    void SetParam(uint32_t index, string& value);
    void SetParam(uint32_t index, const string& value);

	bool ExecuteUpdate();
	uint32_t GetInsertId();
private:
	MYSQL_STMT*	m_stmt;
	MYSQL_BIND*	m_param_bind;
	uint32_t	m_param_cnt;
};

class CDBPool;

class CDBConn {
public:
	CDBConn(CDBPool* pDBPool);
	virtual ~CDBConn();
	int Init();

	//CResultSet* ExecuteQuery(const char* sql_query);
	CResultSetPtr ExecuteQuery(const char* sql_query);
	CResultSetPtr ExecuteQuery(const string& sql_query);
	
	bool ExecuteUpdate(const char* sql_query);
	char* EscapeString(const char* content, uint32_t content_len);

	uint32_t GetInsertId();

	const char* GetPoolName();
	MYSQL* GetMysql() { return m_mysql; }
private:
	CDBPool* 	m_pDBPool;	// to get MySQL server information
	MYSQL* 		m_mysql;
	//MYSQL_RES*	m_res;
	char		m_escape_string[MAX_ESCAPE_STRING_LEN + 1];
};

class CDBPool {
public:
	CDBPool(const string& pool_name, const string& db_server_ip, uint16_t db_server_port,
			const string& username, const string& password, const string& db_name, int max_conn_cnt);
	virtual ~CDBPool();

	int Init();
	CDBConn* GetDBConn();
	void RelDBConn(CDBConn* pConn);

	const char* GetPoolName() { return m_pool_name.c_str(); }
	const char* GetDBServerIP() { return m_db_server_ip.c_str(); }
	uint16_t GetDBServerPort() { return m_db_server_port; }
	const char* GetUsername() { return m_username.c_str(); }
	const char* GetPasswrod() { return m_password.c_str(); }
	const char* GetDBName() { return m_db_name.c_str(); }
private:
	string 		m_pool_name;
	string 		m_db_server_ip;
	uint16_t	m_db_server_port;
	string 		m_username;
	string 		m_password;
	string 		m_db_name;
	int			m_db_cur_conn_cnt;
	int 		m_db_max_conn_cnt;
	list<CDBConn*>	m_free_list;        //实际保存mysql连接的容器
	CThreadNotify	m_free_notify;
};

// manage db pool (master for write and slave for read)
class CDBManager {
public:
	virtual ~CDBManager();

	static CDBManager* getIns();

	int Init();

	CDBConn* GetDBConn();
	CDBConn* GetDBConn(const char* dbpool_name);
	CDBConn* GetDBConn(const string& dbpool_name);
	void RelDBConn(CDBConn* pConn);
private:
	CDBManager();

private:
	static CDBManager*		s_db_manager;
	map<string, CDBPool*>	m_dbpool_map;       //存储mysql连接池，每个池子里面有多个mysql连接对象
};

#endif 
