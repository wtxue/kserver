#include "tcp_server.h"
#include "buffer.h"
#include "tcp_conn.h"

#include "base64.h"
#include "config_reader.h"
#include "logging.h"

#include "db_base.h"
#include "client.hpp"
#include "sdbDataSource.hpp"

#include "mysqlPool.h"

#include "slothjson.h"
//#include "json/nos_static_msg.h"
//#include "json/nos_static_db.h"

#include "manager.h"

using namespace std;
using namespace base;
using namespace net;
using namespace slothjson;

using namespace sdbclient;
using namespace bson;
using namespace sqdb;


int sqdbInit() {
	sqdbCl* pDBCl = nullptr;
	sqdbPool* pool = nullptr;

	sqdbManager *dbManager = sqdbManager::getIns();
	int rc = dbManager->Init();
	if (rc)  {
		LOG_ERROR("dbManager failed");
		return -1;
	}

	pool = sqdbManager::getIns()->GetDbPool();
	if (!pool) {
		LOG_ERROR("dbManager get pool err");
		return -1;
	}
	
	//CSpace.manager_log
	pDBCl = new sqdbCl(pool, "CSpace","manager_log");
	if (pDBCl->Init())  {
		LOG_ERROR("init sqdbCl CSpace.manager_log fail");
		return -3;
	}

	pool->PutDbCl(pDBCl->GetclMapName(),pDBCl);
	LOG_DEBUG("Init cl name:%s ok",pDBCl->GetclFullName().c_str());
	return 0;
}

int mySqlInit() {
	CDBConn* conn = nullptr;

	CDBManager* mysqlManager = CDBManager::getIns();
	int rc = mysqlManager->Init();
	if (rc)  {
		LOG_ERROR("mysqlManager failed");
		return -1;
	}

	conn = CDBManager::getIns()->GetDBConn();
	if (!conn) {
		LOG_ERROR("CDBManager get conn err");
		return -1;
	}

	return 0;
}

void logInit() {
	string log_type = config_reader::ins()->GetString("logInfo", "log_type", "file");
	int    log_level = config_reader::ins()->GetNumber("logInfo", "log_level", 5); /* DEBUG */
	if (log_level <= 0 || log_level > TRACE)
		log_level = TRACE;
	printf("log_type:%s log_level:%d\n",log_type.c_str(),log_level);
	if (log_type == "file") {
		string log_dir = config_reader::ins()->GetString("logInfo", "log_dir", "./log/");
		string log_name = config_reader::ins()->GetString("logInfo", "log_name", "server");
		printf("log:%s %s\n",log_dir.c_str(),log_name.c_str());
		LOG_INIT(log_dir.c_str(), log_name.c_str(), false, log_level);
	}
	else {	
		LOG_INIT(NULL, NULL, true, log_level);
	}
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s config.ini\n", argv[0]);
        printf("  exp: %s ./config.ini\n", argv[0]);
        return 0;
    }

	printf("\nconf path: %s\n", argv[1]);
	if (!config_reader::setPath(argv[1])) {
		printf("load path: %s err, Please check\n", argv[1]);
		return 0;
	}

    // must first init log
	logInit();

    LOG_INFO("sqdbInit Start first /////////"); 
	sqdbInit();

    LOG_INFO("mySqlInit Start "); 
	mySqlInit();

    net::EventLoop base_loop;  
    Server server(&base_loop);
    server.Init();

    LOG_INFO("base loop Start /////////");
    //base_loop.RunAfter(net::Duration(10.0), &test_test);
    base_loop.Run();    
    return 0;
}


