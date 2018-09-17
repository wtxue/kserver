#include "mysqlPool.h"

using namespace std;
using namespace base;

#define MIN_DB_CONN_CNT		2

CDBManager* CDBManager::s_db_manager = NULL;

CResultSet::CResultSet(MYSQL_RES* res) {
	m_res = res;

	// map table field key to index in the result array
	int num_fields = mysql_num_fields(m_res);
	SQL_TRACE("num_fields:%d",num_fields);
	MYSQL_FIELD* fields = mysql_fetch_fields(m_res);
	for(int i = 0; i < num_fields; i++) {
	   m_key_map.insert(make_pair(fields[i].name, i));
	}
}

CResultSet::~CResultSet() {
	SQL_TRACE("~CResultSet");
	if (m_res) {
		mysql_free_result(m_res);
		m_res = NULL;
	}
}

bool CResultSet::Next() {
	m_row = mysql_fetch_row(m_res);
	if (m_row) {
		return true;
	} else {
		return false;
	}
}

int CResultSet::_GetIndex(const char* key) {
	map<string, int>::iterator it = m_key_map.find(key);
	if (it == m_key_map.end()) {
		return -1;
	} else {
		return it->second;
	}
}

int CResultSet::GetInt(const char* key) {
	int idx = _GetIndex(key);
	if (idx == -1) {
		return 0;
	} else {
		if (!m_row[idx]) {
			SQL_TRACE("idx:%d,no key:%s",idx,key);
			return 0;
		}

		SQL_TRACE("idx:%d,key:%s,row:%s",idx,key,m_row[idx]);
		return atoi(m_row[idx]);
	}
}

uint64_t CResultSet::GetLong(const char* key) {
	int idx = _GetIndex(key);
	//SQL_TRACE("idx:%d",idx);
	if (idx == -1) {
		return 0;
	} else {
		if (!m_row[idx]) {
			SQL_TRACE("idx:%d,no key:%s",idx,key);
			return 0;
		}

		SQL_TRACE("idx:%d,key:%s,row:%s",idx,key,m_row[idx]);
		long long v = 0;
		string2ll(m_row[idx], strlen(m_row[idx]), &v);
		return v;
	}
}

bool CResultSet::GetString(const char* key, string &val) {
	int idx = _GetIndex(key);
	//SQL_TRACE("idx:%d",idx);
	if (idx == -1) {
		return false;
	} else {
		if (!m_row[idx]) {
			SQL_TRACE("idx:%d,no key:%s",idx,key);
			return false;
		}
			
		SQL_TRACE("idx:%d,key:%s,row:%s",idx,key,m_row[idx]);
		val = m_row[idx];
		return true;
	}
}

char* CResultSet::GetString(const char* key) {
	int idx = _GetIndex(key);
	//SQL_TRACE("idx:%d",idx);
	if (idx == -1) {
		return NULL;
	} else {
		if (!m_row[idx]) {
			SQL_TRACE("idx:%d,no key:%s",idx,key);
			return NULL;
		}
		
		SQL_TRACE("idx:%d,key:%s,row:%s",idx,key,m_row[idx]);
		return m_row[idx];
	}
}

/////////////////////////////////////////
CPrepareStatement::CPrepareStatement() {
	SQL_TRACE("CPrepareStatement");
	m_stmt = NULL;
	m_param_bind = NULL;
	m_param_cnt = 0;
}

CPrepareStatement::~CPrepareStatement() {
	SQL_TRACE("~CPrepareStatement");
	if (m_stmt) {
		mysql_stmt_close(m_stmt);
		m_stmt = NULL;
	}

	if (m_param_bind) {
		delete [] m_param_bind;
		m_param_bind = NULL;
	}
}

bool CPrepareStatement::Init(MYSQL* mysql, string& sql) {
	mysql_ping(mysql);

	SQL_TRACE("CPrepareStatement Init");
	m_stmt = mysql_stmt_init(mysql);
	if (!m_stmt) {
		SQL_ERROR("mysql_stmt_init failed");
		return false;
	}

	if (mysql_stmt_prepare(m_stmt, sql.c_str(), sql.size())) {
		SQL_ERROR("mysql_stmt_prepare failed: %s", mysql_stmt_error(m_stmt));
		return false;
	}

	m_param_cnt = mysql_stmt_param_count(m_stmt);
	if (m_param_cnt > 0) {
		m_param_bind = new MYSQL_BIND [m_param_cnt];
		if (!m_param_bind) {
			SQL_ERROR("new failed");
			return false;
		}

		memset(m_param_bind, 0, sizeof(MYSQL_BIND) * m_param_cnt);
	}

	return true;
}

void CPrepareStatement::SetParam(uint32_t index, int& value) {
	SQL_TRACE("SetParam int");
	if (index >= m_param_cnt) {
		SQL_ERROR("index too large: %d", index);
		return;
	}

	m_param_bind[index].buffer_type = MYSQL_TYPE_LONG;
	m_param_bind[index].buffer = &value;
}

void CPrepareStatement::SetParam(uint32_t index, uint32_t& value) {
	SQL_TRACE("SetParam uint32_t");
	if (index >= m_param_cnt) {
		SQL_ERROR("index too large: %d", index);
		return;
	}

	m_param_bind[index].buffer_type = MYSQL_TYPE_LONG;
	m_param_bind[index].buffer = &value;
}

void CPrepareStatement::SetParam(uint32_t index, string& value) {
	SQL_TRACE("SetParam string");
	if (index >= m_param_cnt) {
		SQL_ERROR("index too large: %d", index);
		return;
	}

	m_param_bind[index].buffer_type = MYSQL_TYPE_STRING;
	m_param_bind[index].buffer = (char*)value.c_str();
	m_param_bind[index].buffer_length = value.size();
}

void CPrepareStatement::SetParam(uint32_t index, const string& value) {
	SQL_TRACE("SetParam const string");
    if (index >= m_param_cnt) {
        SQL_ERROR("index too large: %d", index);
        return;
    }
    
    m_param_bind[index].buffer_type = MYSQL_TYPE_STRING;
    m_param_bind[index].buffer = (char*)value.c_str();
    m_param_bind[index].buffer_length = value.size();
}

bool CPrepareStatement::ExecuteUpdate() {
	SQL_TRACE("ExecuteUpdate");
	if (!m_stmt) {
		SQL_ERROR("no m_stmt");
		return false;
	}

	if (mysql_stmt_bind_param(m_stmt, m_param_bind)) {
		SQL_ERROR("mysql_stmt_bind_param failed: %s", mysql_stmt_error(m_stmt));
		return false;
	}

	if (mysql_stmt_execute(m_stmt)) {
		SQL_ERROR("mysql_stmt_execute failed: %s", mysql_stmt_error(m_stmt));
		return false;
	}

	if (mysql_stmt_affected_rows(m_stmt) == 0) {
		SQL_ERROR("ExecuteUpdate have no effect");
		return false;
	}

	return true;
}

uint32_t CPrepareStatement::GetInsertId() {
	return mysql_stmt_insert_id(m_stmt);
}

/////////////////////
CDBConn::CDBConn(CDBPool* pPool) {
	m_pDBPool = pPool;
	m_mysql = NULL;
}

CDBConn::~CDBConn() {

}

int CDBConn::Init() {
	m_mysql = mysql_init(NULL);
	if (!m_mysql) {
		SQL_ERROR("mysql_init failed");
		return 1;
	}

	my_bool reconnect = true;
	mysql_options(m_mysql, MYSQL_OPT_RECONNECT, &reconnect);
	mysql_options(m_mysql, MYSQL_SET_CHARSET_NAME, "utf8mb4");

	if (!mysql_real_connect(m_mysql, m_pDBPool->GetDBServerIP(), m_pDBPool->GetUsername(), m_pDBPool->GetPasswrod(),
			m_pDBPool->GetDBName(), m_pDBPool->GetDBServerPort(), NULL, 0)) {
		SQL_ERROR("mysql_real_connect failed: %s", mysql_error(m_mysql));
		return 2;
	}

	SQL_DEBUG("init conn ok:%p",m_mysql);
	return 0;
}

const char* CDBConn::GetPoolName() {
	return m_pDBPool->GetPoolName();
}

CResultSetPtr CDBConn::ExecuteQuery(const char* sql_query) {
	mysql_ping(m_mysql);

	//SQL_TRACE("ExecuteQuery:%s",sql_query);
	if (mysql_real_query(m_mysql, sql_query, strlen(sql_query))) {
		SQL_ERROR("mysql_real_query failed: %s, sql: %s", mysql_error(m_mysql), sql_query);
		return nullptr;
	}

	MYSQL_RES* res = mysql_store_result(m_mysql);
	if (!res) {
		SQL_ERROR("mysql_store_result failed: %s", mysql_error(m_mysql));
		return false;
	}

	//CResultSetPtr resSet(new CResultSet(res));	
	CResultSetPtr resSet = make_shared<CResultSet>(res);
	return resSet;
}

CResultSetPtr CDBConn::ExecuteQuery(const string& sql_query) {
	return ExecuteQuery(sql_query.c_str());
}

bool CDBConn::ExecuteUpdate(const char* sql_query) {
	mysql_ping(m_mysql);

	//SQL_TRACE("ExecuteUpdate:%s",sql_query);
	if (mysql_real_query(m_mysql, sql_query, strlen(sql_query))) {
		SQL_ERROR("mysql_real_query failed: %s, sql: %s", mysql_error(m_mysql), sql_query);
		return false;
	}

	if (mysql_affected_rows(m_mysql) > 0) {
		return true;
	} else {
		return false;
	}
}

char* CDBConn::EscapeString(const char* content, uint32_t content_len) {
	if (content_len > (MAX_ESCAPE_STRING_LEN >> 1)) {
		m_escape_string[0] = 0;
	} else {
		mysql_real_escape_string(m_mysql, m_escape_string, content, content_len);
	}

	return m_escape_string;
}

uint32_t CDBConn::GetInsertId() {
	return (uint32_t)mysql_insert_id(m_mysql);
}

////////////////
CDBPool::CDBPool(const string& pool_name, const string& db_server_ip, uint16_t db_server_port,
		const string& username, const string& password, const string& db_name, int max_conn_cnt) {
	m_pool_name = pool_name;
	m_db_server_ip = db_server_ip;
	m_db_server_port = db_server_port;
	m_username = username;
	m_password = password;
	m_db_name = db_name;
	m_db_max_conn_cnt = max_conn_cnt;
	m_db_cur_conn_cnt = MIN_DB_CONN_CNT;
}

CDBPool::~CDBPool() {
	for (list<CDBConn*>::iterator it = m_free_list.begin(); it != m_free_list.end(); it++) {
		CDBConn* pConn = *it;
		delete pConn;
	}

	m_free_list.clear();
}

int CDBPool::Init() {
	for (int i = 0; i < m_db_cur_conn_cnt; i++) {
		CDBConn* pDBConn = new CDBConn(this);
		int ret = pDBConn->Init();
		if (ret) {
			delete pDBConn;
			return ret;
		}

		m_free_list.push_back(pDBConn);
	}

	SQL_DEBUG("db pool:%s, size: %d", m_pool_name.c_str(), (int)m_free_list.size());
	return 0;
}

/*
 *TODO: 增加保护机制，把分配的连接加入另一个队列，这样获取连接时，如果没有空闲连接，
 *TODO: 检查已经分配的连接多久没有返回，如果超过一定时间，则自动收回连接，放在用户忘了调用释放连接的接口
 */
CDBConn* CDBPool::GetDBConn() {
	m_free_notify.Lock();

	while (m_free_list.empty()) {
		SQL_DEBUG("m_free_list is empty");
		if (m_db_cur_conn_cnt >= m_db_max_conn_cnt) {
			SQL_DEBUG("max so Wait");
			m_free_notify.Wait();
		} else {		
			SQL_DEBUG("create new mysql conn m_db_cur_conn_cnt:%d",m_db_cur_conn_cnt);
			CDBConn* pDBConn = new CDBConn(this);
			int ret = pDBConn->Init();
			if (ret) {
				SQL_ERROR("Init DBConnecton failed");
				delete pDBConn;
				m_free_notify.Unlock();
				SQL_TRACE("leave GetDBConn NULL");
				return NULL;
			} else {
				m_free_list.push_back(pDBConn);
				m_db_cur_conn_cnt++;
				SQL_DEBUG("new db connection: %s, conn_cnt: %d", m_pool_name.c_str(), m_db_cur_conn_cnt);
			}
		}
	}

	CDBConn* pConn = m_free_list.front();
	m_free_list.pop_front();

	m_free_notify.Unlock();
	return pConn;
}

void CDBPool::RelDBConn(CDBConn* pConn)
{
	m_free_notify.Lock();

	list<CDBConn*>::iterator it = m_free_list.begin();
	for (; it != m_free_list.end(); it++) {
		if (*it == pConn) {
			break;
		}
	}

	if (it == m_free_list.end()) {
		m_free_list.push_back(pConn);
	}

	m_free_notify.Signal();
	m_free_notify.Unlock();
}

/////////////////
CDBManager::CDBManager() {
}

CDBManager::~CDBManager() {
}

CDBManager* CDBManager::getIns() {
	if (!s_db_manager) {
		s_db_manager = new CDBManager();
		//if (s_db_manager->Init()) {
		//	delete s_db_manager;
		//	s_db_manager = NULL;
		//}
	}

	return s_db_manager;
}

int CDBManager::Init() {
    string name = config_reader::ins()->GetString("mysql", "name", "default");
    string host = config_reader::ins()->GetString("mysql", "host", "172.16.52.98");
    string user = config_reader::ins()->GetString("mysql", "user", "DBProxyFF");
    string pwd  = config_reader::ins()->GetString("mysql", "pwd",  "DBProxyFF");
	string dbname = config_reader::ins()->GetString("mysql", "dbname", "shroute");
    int  port = config_reader::ins()->GetNumber("mysql", "port", 3306);
    int maxConnNum = config_reader::ins()->GetNumber("mysql", "maxConnNum", 48);	

	SQL_DEBUG("host:%s user:%s pwd:%s dbname:%s", host.c_str(), user.c_str(), pwd.c_str(), dbname.c_str());
	CDBPool* pDBPool = new CDBPool(name, host, port, user, pwd, dbname, maxConnNum);
	if (pDBPool->Init()) {
		SQL_ERROR("init db instance failed: %s", name.c_str());
		return -1;
	}

	SQL_TRACE("new CDBPool:%p",pDBPool);
	m_dbpool_map.insert(make_pair(name, pDBPool));
	return 0;
}

CDBConn* CDBManager::GetDBConn(const char* dbpool_name) {
	//map<string, CDBPool*>::iterator it = m_dbpool_map.find(dbpool_name);
	auto it = m_dbpool_map.find(dbpool_name);
	if (it == m_dbpool_map.end()) {
		return NULL;
	} else {
		return it->second->GetDBConn();
	}
}

CDBConn* CDBManager::GetDBConn(const string& dbpool_name) {
	return GetDBConn(dbpool_name.c_str());
}


CDBConn* CDBManager::GetDBConn() {
	return GetDBConn("default");
}

void CDBManager::RelDBConn(CDBConn* pConn) {
	if (!pConn) {
		return;
	}

	SQL_TRACE("RelDBConn:%p",pConn);
	//map<string, CDBPool*>::iterator it = m_dbpool_map.find(pConn->GetPoolName());
	auto it = m_dbpool_map.find(pConn->GetPoolName());
	if (it != m_dbpool_map.end()) {
		it->second->RelDBConn(pConn);
	}
}
