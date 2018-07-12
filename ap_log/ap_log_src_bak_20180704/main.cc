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
#if 1
	BSONObj clOptions;
	BSONObj csOptions;

	clOptions = BSON( "ReplSize" << 0 << "ShardingKey" << BSON("_id" << 1) << "ShardingType" << "hash" << "AutoSplit" << true);
	LOG_ERROR("clOptions:%s", clOptions.toString().c_str());

	csOptions = BSON( "PageSize" << SDB_PAGESIZE_8K << "Domain" << SDBDOMAIN_NAME) ;
	LOG_ERROR("csOptions:%s", csOptions.toString().c_str()) ;
#else
	BSONObj obj ;
	BSONObjBuilder b ;
	b << "PageSize" << SDB_PAGESIZE_8K << "Domain" << SDBDOMAIN_NAME ;
	obj = b.obj() ;
	LOG_ERROR("csOptions:%s", obj.toString().c_str()) ;

	BSONObj obj1 ;
	BSONObjBuilder b1 ;
	
	BSONObj obj2 ;
	BSONObjBuilder b2 ;

	b1 << "_id" << 1;
	obj1 = b1.obj() ;
	LOG_ERROR("csOptions:%s", obj1.toString().c_str()) ;
	b1 << "ReplSize" << 0 << "ShardingKey" << obj1.toString() << "ShardingType" << "hash" << "AutoSplit" << true;
	obj2 = b.obj() ;
	LOG_ERROR("csOptions:%s", obj2.toString().c_str()) ;

	BSONObj obj ;
	BSONObjBuilder b ;
#endif
}


void ap_log_db_qos_conf_index(sdbclient::sdbCollection& cl) {
	slothjson::index_param_time_t idx;
	string obj;	
    BSONObj obj_;

	idx.skip_index_name();
	int rc = slothjson::encode<false, slothjson::index_param_time_t>(idx, obj);	
	if (!rc) {
		LOG_ERROR("decode index_param_time_t err");
		return ;
	}

    rc = fromjson(obj, obj_);
    if (rc) {
		LOG_ERROR("Failed to fromjson, rc:%d", rc) ;
		return ;
	}
	
	rc = cl.createIndex(obj_, idx.index_name.c_str(), FALSE, FALSE) ;
	if ( rc ) {
		LOG_ERROR( "Failed to create index index_addTime, rc:%d\n", rc ) ;
	}

	LOG_DEBUG("index_name:%s obj:%s ok",idx.index_name.c_str(),obj.c_str());
}

void ap_log_db_qos_vip_conf_index(sdbCollection& cl) {
	slothjson::index_mac_t idx;
	string obj;	
    BSONObj obj_;

	idx.skip_index_name();
	int rc = slothjson::encode<false, slothjson::index_mac_t>(idx, obj);	
	if (!rc) {
		LOG_ERROR("decode index_param_time_t err");
		return ;
	}


    rc = fromjson(obj, obj_);
    if (rc) {
		LOG_ERROR("Failed to fromjson, rc:%d", rc) ;
		return ;
	}
	
	rc = cl.createIndex(obj_, idx.index_name.c_str(), FALSE, FALSE) ;
	if ( rc ) {
		LOG_ERROR( "Failed to create index index_addTime, rc:%d\n", rc ) ;
	}

	LOG_DEBUG("index_name:%s obj:%s ok",idx.index_name.c_str(),obj.c_str());
}


void ap_log_db_dev_index(sdbCollection& cl) {
	slothjson::index_mac_RPCMethod_t idx_mac_RPCMethod;
	slothjson::index_ID_t idx_ID;
	slothjson::index_addtime_t idx_addtime;
	string obj;	
    BSONObj obj_;

	idx_mac_RPCMethod.skip_index_name();
	int rc = slothjson::encode<false, slothjson::index_mac_RPCMethod_t>(idx_mac_RPCMethod, obj);	
	if (!rc) {
		LOG_ERROR("decode index_param_time_t err");
		return ;
	}

    rc = fromjson(obj, obj_);
    if (rc) {
		LOG_ERROR("Failed to fromjson, rc:%d", rc) ;
		return ;
	}
	
	rc = cl.createIndex(obj_, idx_mac_RPCMethod.index_name.c_str(), FALSE, FALSE) ;
	if ( rc ) {
		LOG_ERROR( "Failed to create index index_addTime, rc:%d\n", rc ) ;
	}
	
	LOG_ERROR("index_name:%s obj:%s ok",idx_mac_RPCMethod.index_name.c_str(),obj.c_str());

	idx_ID.skip_index_name();
	rc = slothjson::encode<false, slothjson::index_ID_t>(idx_ID, obj);	
	if (!rc) {
		LOG_ERROR("decode index_param_time_t err");
		return ;
	}

    rc = fromjson(obj, obj_);
    if (rc) {
		LOG_ERROR("Failed to fromjson, rc:%d", rc) ;
		return ;
	}
	
	rc = cl.createIndex(obj_, idx_ID.index_name.c_str(), FALSE, FALSE) ;
	if ( rc ) {
		LOG_ERROR( "Failed to create index index_addTime, rc:%d\n", rc ) ;
	}
	
	LOG_DEBUG("index_name:%s obj:%s ok",idx_ID.index_name.c_str(),obj.c_str());

	idx_addtime.skip_index_name();
	rc = slothjson::encode<false, slothjson::index_addtime_t>(idx_addtime, obj);	
	if (!rc) {
		LOG_ERROR("decode index_param_time_t err");
		return ;
	}

    rc = fromjson(obj, obj_);
    if (rc) {
		LOG_ERROR("Failed to fromjson, rc:%d", rc) ;
		return ;
	}
	
	rc = cl.createIndex(obj_, idx_addtime.index_name.c_str(), FALSE, FALSE) ;
	if ( rc ) {
		LOG_ERROR( "Failed to create index index_addTime, rc:%d\n", rc ) ;
	}
	
	LOG_DEBUG("index_name:%s obj:%s ok",idx_addtime.index_name.c_str(),obj.c_str());	
}

int sqdbInit() {
	string dbPoolName = "ap_log";
	sqdbCl* pDBCl = nullptr;
	sqdbPool* pool = nullptr;

	sqdbManager *dbManager = sqdbManager::getIns();
	int rc = dbManager->Init();
	if (rc)  {
		LOG_ERROR("dbManager failed");
		return -1;
	}

	pool = sqdbManager::getIns()->GetDbPool(dbPoolName);
	if (!pool) {
		LOG_ERROR("dbManager get pool err");
		return -1;
	}

	LOG_DEBUG("pool:%p",pool);

	//ap_log_20180702.ap_log_dev
	pDBCl = new sqdbCl(pool, "ap_log","ap_log_dev",24);
	pDBCl->SetCreateIndexCallback(&ap_log_db_dev_index);
	if (pDBCl->Init())  {
		LOG_ERROR("init sqdbCl ap_log_20180702 fail");
		return -2;
	}
	pool->PutDbCl(pDBCl->GetclMapName(),pDBCl);	
	LOG_DEBUG("Init cl name:%s ok",pDBCl->GetclFullName().c_str());	

	//ap_config.config
	pDBCl = new sqdbCl(pool, "ap_config","config");
	pDBCl->SetCreateIndexCallback(&ap_log_db_qos_conf_index);
	if (pDBCl->Init())  {
		LOG_ERROR("init sqdbCl ap_config.config fail");
		return -3;
	}

	pool->PutDbCl(pDBCl->GetclMapName(),pDBCl);
	LOG_DEBUG("Init cl name:%s ok",pDBCl->GetclFullName().c_str());

	//ap_config.log_qos_vip
	pDBCl = new sqdbCl(pool, "ap_config","log_qos_vip");
	pDBCl->SetCreateIndexCallback(&ap_log_db_qos_conf_index);
	if (pDBCl->Init())  {
		LOG_ERROR("init sqdbCl ap_config.log_qos_vip fail");
		return -3;
	}

	pool->PutDbCl(pDBCl->GetclMapName(),pDBCl);
	LOG_DEBUG("Init cl name:%s ok",pDBCl->GetclFullName().c_str());
	return 0;
}

void logInit() {
	string log_type = config_reader::ins()->GetString("logInfo", "log_type", "file");
	printf("log_type:%s\n",log_type.c_str());
	if (log_type == "file") {
		string log_dir = config_reader::ins()->GetString("logInfo", "log_dir", "./log/");
		string log_name = config_reader::ins()->GetString("logInfo", "log_name", "server");
		printf("log:%s %s\n",log_dir.c_str(),log_name.c_str());
		LOG_INIT(log_dir.c_str(), log_name.c_str(), false, TRACE);
	}
	else {	
		LOG_INIT(NULL, NULL, true, TRACE);
	}
}

int main(int argc, char* argv[]) {
    if (argc != 1 && argc != 3) {
        printf("Usage: %s config\n", argv[0]);
        printf("  e.g: %s conf.ini\n", argv[0]);
        return 0;
    }

    config_reader::setPath("myconf.ini");
    // must first init log
	logInit();

    LOG_INFO("sqdbInit Start first /////////"); 
	sqdbInit();

    net::EventLoop base_loop;  
    apLogServer server(&base_loop);
    server.Init();

    LOG_INFO("base loop Start /////////"); 
    base_loop.RunAfter(net::Duration(10.0), &test_test);
    base_loop.Run();    
    return 0;
}


