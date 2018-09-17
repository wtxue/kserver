#include <signal.h> 
#include "tcp_server.h"
#include "buffer.h"
#include "tcp_conn.h"

#include "base64.h"
#include "config_reader.h"
#include "db_base.h"
#include "ap_log.h"
#include "client.hpp"
#include "logging.h"

#include "client.hpp"
#include "sdbDataSource.hpp"


#include "slothjson.h"
#include "json/ap_log_msg.h"
#include "json/ap_log_db.h"

using namespace std;
using namespace base;
using namespace net;
using namespace slothjson;
using namespace ap_log;
using namespace sqdb;

using namespace sdbclient;
using namespace bson;


void test_test() {
}



void SignalUsr1Hander() {
	LOG_DEBUG("catch SIGUSR1");
	ring_log *plog = ring_log::ins();

	auto level = plog->get_level();
	if (level == TRACE) {
		LOG_INFO("change level TRACE to DEBUG");
		plog->set_level(DEBUG);
	}

	if (level == DEBUG) {
		LOG_INFO("change level DEBUG to TRACE");
		plog->set_level(TRACE);
	}
}

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
		return -2;
	}
	LOG_DEBUG("GetDbPool get ok:%p",pool);

	auto cl = config_reader::ins()->GetStringList("sqdb", "cl");
	for (size_t i = 0; i < cl.size(); i++) {
		auto flag = config_reader::ins()->GetNumber(cl[i], "flag", 0);
		auto index = config_reader::ins()->GetStringList(cl[i], "index");
		LOG_DEBUG("i:%d cl:%s flag:%d,index.size():%d", i, cl[i].c_str(), flag, index.size());
		pDBCl = new sqdbCl(pool, cl[i], flag);
		if (index.size())
			pDBCl->PushAllIndex(index);

		if (pDBCl->Init())	{
			LOG_ERROR("init sqdbCl:%s fail",cl[i].c_str());
			return -3;
		}		
		pool->PutDbCl(pDBCl->GetclMapName(),pDBCl);	
		LOG_DEBUG("Init cl:%p name:%s ok",pDBCl,pDBCl->GetclFullName().c_str());	
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
		printf("load path: %s Failed, please check your config\n", argv[1]);
		return 0;
	}

    // must first init log
	logInit();

    LOG_INFO("sqdbInit Start first /////////"); 
	if (sqdbInit() < 0) {
		printf("sqdbInit Failed, please check your config\n");
		return 0;
	}

    net::EventLoop base_loop;  
    apLogServer server(&base_loop);
    server.Init();

    LOG_INFO("base loop Start init SIGUSR1 /////////"); 
	std::unique_ptr<net::SignalEventWatcher> ev(new net::SignalEventWatcher(SIGUSR1, &base_loop, &SignalUsr1Hander));
	ev->Init();
    ev->AsyncWait();
	
	LOG_INFO("base loop Start /////////"); 
	//base_loop.RunAfter(net::Duration(5.0), &test_test);
    base_loop.Run();    
    return 0;
}


