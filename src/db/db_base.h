#ifndef __DB_BASE_H__
#define __DB_BASE_H__

#include <vector>
#include <new>
#include "logger.h"
#include "config_reader.h"
#include "timestamp.h"

#include "client.hpp"
#include "sdbDataSource.hpp"

using namespace std;
using namespace base;
using namespace net;

#if 1
#define DB_TRACE(...)     _LOG_HEAD("[DB_TRAC]",__VA_ARGS__)
#define DB_DEBUG(...)     _LOG_HEAD("[DB_DBUG]",__VA_ARGS__)
#define DB_ERROR(...)     _LOG_HEAD("[DB_ERRO]",__VA_ARGS__)
#else
#define DB_TRACE(...)
#define DB_DEBUG(...)
#define DB_ERROR(...) 
#endif

namespace sqdb {

using namespace sdbclient;
using namespace bson;

#define SDBDOMAIN_NAME "allGroupDomain"

typedef std::function<void(sdbclient::sdbCollection&)> CreateIndexCallback;

class sqdbPool;
class sqdbManager;

class sqdbCl {
public:
	sqdbCl(sqdbPool* pDBPool, const string &csName, const string &clName);
	sqdbCl(sqdbPool* pDBPool, const char *csName, const char *clName);
	sqdbCl(sqdbPool* pDBPool, const string &csName, const string &clName, long flag);
	sqdbCl(sqdbPool* pDBPool, const char *csName, const char *clName, long flag);
	
	~sqdbCl();
	int Init(string& csName,string& clName);
	int Init();
	void GetLoopCsName(string &trueCsName, string &csNameHead, long flag, int isNext);
	void UpdateLoopCsName();
	
	int GetDsCl(sdb *& conn,sdbCollection& cl);
	int insert(const string &obj);	
	int query(const string &cd, std::vector<string> &objs);
	int query(const string &cd, string &obj);
	int update(const bson::BSONObj &rule, const bson::BSONObj &condition);	
	int upsert(const bson::BSONObj &rule, const bson::BSONObj &condition);
	int aggregate(sdbCursor &cursor,	  std::vector<bson::BSONObj> &obj);

	const string& GetclFullName() { 
		return _clFullName; 
	}	
	const string& GetclMapName() { return _clMapName; }
	const string& GetcsNameHead() { return _csNameHead; }
	
	sqdbPool *GetPool() { return _pDBPool; }
	
    void SetCreateIndexCallback(CreateIndexCallback cb) {
        _index_fn = cb;
    }
	
private:	
	string    _clFullName;
	string    _clMapName;
	string    _csName;
	string    _clName;
	string    _csNameHead;
	bool      _isCreate;
	long      _flag;
	sqdbPool *_pDBPool;
	CreateIndexCallback _index_fn;
};

class sqdbPool {
public:
	sqdbPool(const string& poolName, 
					const string& host, 
					const string& port,
					const string& user, 
					const string& pwd, 
					const string& dbName, 
					int connectNum);
	~sqdbPool(); 

	int Init();
	int GetList(int   listType, std::vector<string> &objs);
	int dropCollectionSpace(const string& csName);
	int dropCollectionSpace(const char* csName);
	
	sqdbCl* GetDbCl(const string& clMapName);
	void PutDbCl(const string& clMapName, sqdbCl* pCl);
	
	const char* GetDbPoolName() { return _poolName.c_str(); }
	const char* GetDbHost() { return _host.c_str(); }
	const char* GetDbPort() { return _port.c_str(); }
	const char* GetDbUser() { return _user.c_str(); }
	const char* GetDbPwd() { return _pwd.c_str(); }
	const char* GetDbName() { return _dbName.c_str(); }
	int GetDsConn(sdb*& conn);
	void ReleaseDsConn(sdb* conn);

private:
	string 		_poolName;
	string 		_host;
	string  	_port;
	string 		_user;
	string 		_pwd;
	string 		_dbName;
	int			_connectNum;
    
    sdbDataSourceConf           _conf;
    sdbDataSource               _ds;
	std::map<string, sqdbCl*>	_mapCl;     
};

class sqdbManager {
public:
	sqdbManager();
	~sqdbManager();
	static sqdbManager* getIns();
	int Init();
	int InitTable();
	sqdbPool* GetDbPool(const string& dbPoolName);
	sqdbPool* GetDbPool();
	void PutDbPool(const string& dbPoolName, sqdbPool* pDBPool);

private:
	static sqdbManager*		    _sqdbManager;
	std::map<string, sqdbPool*>	_mapPool;       
};

}


#endif

