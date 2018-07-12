#include <vector>
#include <new>
#include "logger.h"
#include "timestamp.h"
#include "config_reader.h"
#include "db_base.h"
#include "client.hpp"
#include "sdbDataSource.hpp"

using namespace std;
using namespace base;
using namespace net;
using namespace sdbclient;
using namespace bson;

namespace sqdb {

sqdbManager* sqdbManager::_sqdbManager = NULL;

inline void DefaultCreateIndexCallback(const sdbCollection& conn) {
	DB_TRACE("DefaultCreateIndexCallback");
}

sqdbCl::sqdbCl(sqdbPool* pDBPool, const string &csName, const string &clName) {
	_isCreate = false;
	_flag = 0;
	_csName = csName;
	_csNameHead = csName;
	_clName = clName;
	_clMapName = _csName + "." + _clName;
	_clFullName = _clMapName;
	_pDBPool = pDBPool;
	SetCreateIndexCallback(&DefaultCreateIndexCallback);
	DB_DEBUG("_pDBPool:%p _clFullName:%s _clMapName:%s", _pDBPool, _clFullName.c_str(), _clMapName.c_str());
}

sqdbCl::sqdbCl(sqdbPool* pDBPool, const char *csName, const char *clName) {
	_isCreate = false;
	_flag = 0;
	_csName = csName;	
	_csNameHead = csName;
	_clName = clName;
	_clMapName = _csName + "." + _clName;
	_clFullName = _clMapName;
	_pDBPool = pDBPool;
	SetCreateIndexCallback(&DefaultCreateIndexCallback);
	DB_DEBUG("_pDBPool:%p _clFullName:%s _clMapName:%s", _pDBPool, _clFullName.c_str(), _clMapName.c_str());
}

void sqdbCl::GetLoopCsName(string &trueCsName, string &csNameHead, long flag, int isNext) {
	struct tm tm;	
	int64_t timep; 
	char csPace[128] = {0};

	if (flag >= 24) {
		timep = net::Timestamp::Now().Unix();
		if (isNext) 
			timep += 24*60*60;
		localtime_r(&timep, &tm);
		snprintf(csPace,127,"%s_%04d%02d%02d",csNameHead.c_str(),1900+tm.tm_year,1+tm.tm_mon,tm.tm_mday);	
		trueCsName = csPace;
	}
	else if (flag >= 1) {
		timep = net::Timestamp::Now().Unix();
		if (isNext)
			timep += 1*60*60;
		localtime_r(&timep, &tm);
		snprintf(csPace,127,"%s_%04d%02d%02d%02d",csNameHead.c_str(),1900+tm.tm_year,1+tm.tm_mon,tm.tm_mday,tm.tm_hour);
		trueCsName = csPace;
	}
	else {
		trueCsName = csNameHead;
	}

	DB_DEBUG("get trueCsName:%s",trueCsName.c_str());
}

sqdbCl::sqdbCl(sqdbPool* pDBPool, const string &csName, const string &clName, long flag) {
	_isCreate = false;	
	_flag = 0;
	_csNameHead = csName;
	_clName = clName;
	_clMapName = _csNameHead + "." + _clName;
	
	GetLoopCsName(_csName, _csNameHead, flag, 0);
	_clFullName = _csName + "." + _clName;

	_flag = flag;
	_pDBPool = pDBPool;
	SetCreateIndexCallback(&DefaultCreateIndexCallback);
	DB_DEBUG("_pDBPool:%p _clFullName:%s _clMapName:%s", _pDBPool, _clFullName.c_str(), _clMapName.c_str());
}

sqdbCl::sqdbCl(sqdbPool* pDBPool, const char *csName, const char *clName, long flag) {
	_isCreate = false;
	_flag = 0;
	_csNameHead = csName;
	_clName = clName;
	_clMapName = _csNameHead + "." + _clName;

	GetLoopCsName(_csName, _csNameHead, flag, 0);
	_clFullName = _csName + "." + _clName;

	_flag = flag;
	_pDBPool = pDBPool;
	SetCreateIndexCallback(&DefaultCreateIndexCallback);
	DB_DEBUG("_pDBPool:%p _clFullName:%s _clMapName:%s", _pDBPool, _clFullName.c_str(), _clMapName.c_str());
}

sqdbCl::~sqdbCl() {
}

void sqdbCl::UpdateLoopCsName() {
	string calcCsName;
	
	GetLoopCsName(calcCsName, _csNameHead, _flag, 0);
	if (_csName != calcCsName) {	
		_isCreate = false;
		_csName = calcCsName;		
		_clFullName = _csName + "." + _clName;
		DB_DEBUG("change _csName:%s, _clFullName:%s", _csName.c_str(), _clFullName.c_str());
		Init();	
	}
}

int sqdbCl::Init(string& csName,string& clName) {
    INT32 rc = SDB_OK ;
	sdb *conn;
	sdbCollection cl;
    sdbCollectionSpace cs ;
	BSONObj csOptions;
	BSONObj clOptions;

	rc = _pDBPool->GetDsConn(conn);
	if (rc) {
		DB_ERROR("Failed to get connection rc:%d",rc);
		return rc;
	}
	
	string clFullName;	
	clFullName = csName + "." + clName;
	
	rc = conn->getCollection(clFullName.c_str(), cl);
    if ((SDB_DMS_NOTEXIST == rc) || (SDB_DMS_CS_NOTEXIST == rc)) {
        DB_DEBUG("Collection %s does not exist, creating a new collection", clFullName.c_str());
        rc = conn->getCollectionSpace(csName.c_str(), cs);
        if (rc) {
            DB_DEBUG("cSpace to get _csName:%s not exist, creating a new cSpace", csName.c_str());
			csOptions = BSON( "PageSize" << SDB_PAGESIZE_8K << "Domain" << SDBDOMAIN_NAME) ;
	        DB_DEBUG("csOptions:%s", csOptions.toString().c_str()) ;
			rc = conn->createCollectionSpace(csName.c_str(), csOptions, cs);
			if (rc) {
				DB_ERROR("Failed to create new _csName:%s, rc:%d", csName.c_str(), rc);
				goto error;
			}
        }

		clOptions = BSON( "ReplSize" << 0 << "ShardingKey" << BSON("_id" << 1) << "ShardingType" << "hash" << "AutoSplit" << true);
		DB_DEBUG("clOptions:%s", clOptions.toString().c_str());
		/* may need 2s */
        rc = cs.createCollection(clName.c_str(), clOptions, cl);
        if ( rc ) {
            DB_ERROR("Failed to create new _clName:%s, rc:%d", clName.c_str(), rc);
            goto error;
        }

		if (_index_fn) {
			DB_DEBUG("start create index fn");
			_index_fn(cl);
		}
		
        DB_DEBUG("Successfully created new collection:%s", clFullName.c_str());
    }
    else if (rc) {
        DB_ERROR("Failed to get collection:%s, rc:%d", clFullName.c_str() , rc) ;
        goto error;
    }	
    
	DB_DEBUG("Successfully get collection:%s", clFullName.c_str());
	if (clFullName == _clFullName)
		_isCreate = true;

error:
	_pDBPool->ReleaseDsConn(conn);
	return rc;
}

int sqdbCl::Init() {	
	Init(_csName,_clName);
	if (_flag > 0) {
		DB_DEBUG("enter init next CsName");
		string calcNextCsName;	
		GetLoopCsName(calcNextCsName, _csNameHead, _flag, 1);
		Init(calcNextCsName, _clName);
	}

	return 0;
}

int sqdbCl::GetDsCl(sdb *& conn,sdbCollection& cl) {
	int rc;

	if (!_isCreate) {
		DB_ERROR("sqdbCl not create");
		return -1;
	}
	
	//DB_TRACE("GetDsConn");
	rc = _pDBPool->GetDsConn(conn);
	if (SDB_OK != rc) {
		DB_ERROR("Failed to get connection, rc:%d", rc);
		return rc;
	}

	//DB_TRACE("getCollection");
	rc = conn->getCollection(GetclFullName().c_str(), cl);
	if (SDB_OK != rc) {
		DB_ERROR("Failed to get cl, rc:%d _clFullName:%s", rc, _clFullName.c_str());
		return rc;
	}
	
	//DB_TRACE("getCollection ok");
	return rc;
}


int sqdbCl::query(const string &condition, std::vector<string> &objs) {
    INT32 rc = SDB_OK;
	sdb * conn;
	sdbCollection cl;
    sdbCursor cursor;
    BSONObj condition_;
    BSONObj obj_;

	if (condition.size()) {
	    rc = fromjson(condition, condition_);
	    if (rc) {
			DB_ERROR("Failed to fromjson, rc:%d", rc) ;
			return rc;
		}
	}

	//DB_TRACE("GetDsCl");
	rc = GetDsCl(conn,cl);
	if (rc) {
		DB_ERROR("Failed to GetDsCl, rc:%d", rc) ;
		return rc;
	}
	
    rc = cl.query(cursor, condition_);
    if (rc) {
        if (SDB_DMS_EOC != rc) 
            DB_ERROR ( "Failed to query from collection, rc = %d" , rc ) ;
        else
            DB_ERROR ( "No records can be read"  ) ;
        goto error;
    }
    
    while (SDB_OK == cursor.next(obj_)) {
        //DB_TRACE("%s", obj_.toString().c_str()) ;
        objs.push_back(obj_.toString());
    }
    rc = objs.size();

error:
	_pDBPool->ReleaseDsConn(conn);	
    return rc;
}

int sqdbCl::query(const string &condition, string &obj) {
    INT32 rc = SDB_OK;
	sdb * conn;
	sdbCollection cl;
    sdbCursor cursor;
    BSONObj condition_;
    BSONObj obj_;

    rc = fromjson(condition, condition_);
    if (rc) {
		DB_ERROR("Failed to fromjson, rc:%d", rc) ;
		return rc;
	}

	//DB_TRACE("GetDsCl");
	rc = GetDsCl(conn,cl);
	if (rc) {
		DB_ERROR("Failed to GetDsCl, rc:%d", rc) ;
		return rc;
	}
	
    rc = cl.query(cursor, condition_);
    if (rc) {
        if (SDB_DMS_EOC != rc) 
            DB_ERROR ( "Failed to query from collection, rc:%d", rc) ;
        else
            DB_ERROR ( "No records can be read"  ) ;
        goto error;
    }

    rc = cursor.current(obj_);
    if (rc && (SDB_DMS_EOC != rc)) {
		DB_ERROR("Failed to fetch next record, rc:%d", rc) ;
		goto error;
	}
	
    DB_DEBUG("condition:%s query:%s", condition.c_str(), obj_.toString().c_str()) ;
    obj = obj_.toString();
    
error:
	_pDBPool->ReleaseDsConn(conn);    
    return rc;
}

int sqdbCl::insert(const string &obj) {	
	sdb * conn;
	sdbCollection cl;
    BSONObj oneObj;
	int rc = SDB_OK ;

	//DB_TRACE("fromjson");
    rc = fromjson(obj, oneObj);
    if (rc) {
		DB_ERROR("Failed to fromjson, rc:%d", rc) ;
		return rc;
	}

	//DB_TRACE("GetDsCl");
	rc = GetDsCl(conn,cl);
	if (rc) {
		DB_ERROR("Failed to GetDsCl, rc:%d", rc) ;
		return rc;
	}

	//DB_TRACE("insert");
    rc = cl.insert(oneObj);
    if (rc) {
		DB_ERROR("Failed to insert, rc:%d", rc) ;
		goto error;
	}

    //DB_TRACE("insert ok");
error:
	_pDBPool->ReleaseDsConn(conn);
	return rc;
}


sqdbPool::sqdbPool(const string& poolName, const string& host, const string& port,
		const string& user, const string& pwd, const string& dbName, int connectNum) {
	_poolName = poolName;
	_host = host;
	_port = port;
	_user = user;
	_pwd = pwd;
	_dbName = dbName;
	_connectNum = connectNum;

    /* 设置数据库鉴权的用户名密码。
       若数据库没开启鉴权，此处可填空字符串，或直接忽略此项。
       以下2个参数分别取默认值。 */
    _conf.setUserInfo(_user, _pwd);
    /* 第一个参数10表示连接池启动时，初始化10个可用连接。
       第二个参数10表示当连接池增加可用连接时，每次增加10个。
       第三个参数20表示当连接池空闲时，池中最多保留20个连接，多余的将被释放。
       第四个参数500表示连接池最多保持500个连接。
       以下4个参数分别取默认值。 */
    _conf.setConnCntInfo(10, 10, 20, 500);
    /* 第一个参数60000，单位为毫秒。表示连接池以60秒为一个周期，
       周期性地删除多余的空闲连接及检测连接是否过长时间没有被使用(收发消息)。
       第二个参数0，单位为毫秒。表示池中空闲连接的存活时间。
       0毫秒表示连接池不关心连接隔了多长时间没有收发消息。
       以下2个参数分别取默认值。 */
    _conf.setCheckIntervalInfo(60000, 0);
    /* 设置周期性从catalog节点同步coord节点地址的周期。单位毫秒。
       当设置为0毫秒时，表示不同步coord节点地址。默认值为0毫秒。 */
    _conf.setSyncCoordInterval(30000);
    /* 设置使用coord地址负载均衡的策略获取连接。默认值为DS_STY_BALANCE。 */
    _conf.setConnectStrategy(DS_STY_BALANCE);
    /* 连接出池时，是否检测连接的可用性，默认值为FALSE。 */
    _conf.setValidateConnection(TRUE);
    /* 设置连接是否开启SSL功能，默认值为FALSE。 */
    _conf.setUseSSL(FALSE);
}

sqdbPool::~sqdbPool() {
	int rc = SDB_OK;

	rc = _ds.disable();
	if (SDB_OK != rc) {
		DB_ERROR("Failed to disable sdbDataSource pool name:%s, rc:%d",_poolName.c_str(), rc);
	}
	_ds.close();
}

int sqdbPool::Init() {
	string url = _host + ':' + _port;
	int rc = _ds.init(url, _conf);
	if (SDB_OK != rc) {
		DB_ERROR("Failed to init sdbDataSource, rc:%d url:%s", rc, url.c_str());
		goto error;
	}

	rc = _ds.enable();
	if (SDB_OK != rc) {
		DB_ERROR("Failed to enable sdbDataSource, rc:%d url:%s", rc, url.c_str());
		goto error;
	}

	DB_DEBUG("db url:%s pool:%s, init ok",url.c_str(), _poolName.c_str());
error:	
	return rc;
}

int sqdbPool::GetDsConn(sdb*& conn) {
	int rc = _ds.getConnection(conn);
	if (SDB_OK != rc) {
		DB_ERROR("Failed to get connection, rc:%d", rc);
		return rc;
	}

	return rc;
}

void sqdbPool::ReleaseDsConn(sdb* conn) {
	_ds.releaseConnection(conn);
}

int sqdbPool::GetList(int   listType, std::vector<string> &objs) {
	sdb *conn;
	sdbCursor cursor;
	BSONObj   obj;	
	int rc = 0;

	rc = GetDsConn(conn);
	if (rc) {
		DB_ERROR("Failed to get connection, rc:%d", rc);
		return -1;
	}

	rc = conn->getList(cursor, listType);
	if (SDB_OK != rc) {
		DB_ERROR("Failed to get csList, rc:%d",rc);
		goto error;
	}

	while (SDB_OK == cursor.next(obj)) {
		//DB_DEBUG("obj:%s",obj.toString().c_str());
        objs.push_back(obj.toString());
    }
    
    rc = objs.size();		

error:
	this->ReleaseDsConn(conn);
	return rc;
}

int sqdbPool::dropCollectionSpace(const char* csName) {
	sdb *conn;
	int rc = 0;

	rc = GetDsConn(conn);
	if (rc) {
		DB_ERROR("Failed to get conn, rc:%d", rc);
		return -1;
	}

	rc = conn->dropCollectionSpace(csName);
	if (SDB_OK != rc) {
		DB_ERROR("Failed to dropCollectionSpace, rc:%d",rc);
		goto error;
	}
	DB_DEBUG("drop csName ok:%s",csName);
error:
	this->ReleaseDsConn(conn);
	return rc;
}

int sqdbPool::dropCollectionSpace(const string& csName) {
	return dropCollectionSpace(csName.c_str());
}

sqdbCl* sqdbPool::GetDbCl(const string& clMapName) {
	//std::map<string, sqdbCl*>::iterator it = _mapCl.find(clMapName);
	auto it = _mapCl.find(clMapName);
	if (it == _mapCl.end()) {
		return NULL;
	} else {
		return it->second;
	}
}

void sqdbPool::PutDbCl(const string& clMapName, sqdbCl* pCl) {
	DB_DEBUG("clMapName:%s", clMapName.c_str());
	_mapCl.insert(make_pair(clMapName, pCl));
}

sqdbManager::sqdbManager() {
//	DB_TRACE("sqdbManager");
}

sqdbManager::~sqdbManager() {
}

sqdbManager* sqdbManager::getIns() {
	if (!_sqdbManager) {
		_sqdbManager = new sqdbManager();
		//if (_sqdbManager->Init()) {
		//	delete _sqdbManager;
		//	_sqdbManager = NULL;
		//}
	}
	
	return _sqdbManager;
}

int sqdbManager::Init() {
    std::string name = config_reader::ins()->GetString("sqdb", "name", "default");
    std::string host = config_reader::ins()->GetString("sqdb", "host", "127.0.0.1");
    std::string port = config_reader::ins()->GetString("sqdb", "port", "11810");
    std::string user = config_reader::ins()->GetString("sqdb", "user", "sdbadmin");
    std::string pwd = config_reader::ins()->GetString("sqdb", "pwd", "netcoresdbadmin");
    int connectNum = config_reader::ins()->GetNumber("sqdb", "connectNum", 48);

	DB_TRACE("name:%s",name.c_str());
	sqdbPool* pDBPool = new sqdbPool(name, host, port, user, pwd, name, connectNum);
	if (pDBPool->Init()) {
		DB_ERROR("init db instance failed: %s", name.c_str());
		return -1;
	}

	PutDbPool(name,pDBPool);
	DB_TRACE("Init pool name:%s %p ok",name.c_str(),pDBPool);
	return 0;
}

sqdbPool* sqdbManager::GetDbPool(const string& dbPoolName) {
	//std::map<string, sqdbPool*>::iterator it = _mapPool.find(dbPoolName);
	auto it = _mapPool.find(dbPoolName);
	if (it == _mapPool.end()) {
		return NULL;
	} else {
		return it->second;
	}
}

sqdbPool* sqdbManager::GetDbPool() {
	string dbPoolName = "default";
	return GetDbPool(dbPoolName);
}

void sqdbManager::PutDbPool(const string& dbPoolName, sqdbPool* pDBPool) {
	DB_DEBUG("dbPoolName:%s",dbPoolName.c_str());
	_mapPool.insert(make_pair(dbPoolName, pDBPool));
}

}

