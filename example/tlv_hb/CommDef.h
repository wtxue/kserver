#ifndef __COMMDEF_H__
#define __COMMDEF_H__

enum LOG_STATUS {
	LOG_STATUS_OFF = 0,
	LOG_STATUS_ON  = 1,
	LOG_STATUS_QOS = 2,
	LOG_STATUS_KEY = 3, //RESULT != 0
};

enum {
	LOG_DIRECT_WEB2SER 		= 1,
	LOG_DIRECT_SER2DEV 		= 2,
	LOG_DIRECT_DEV2SER 		= 3,
	LOG_DIRECT_OPRT2DIST 	= 4,
	LOG_DIRECT_DIST2OPRT 	= 5,
	LOG_DIRECT_SER2OSER	 	= 6,
	LOG_DIRECT_OSER2SER	 	= 7,
};

#define AS_JS_RPCMETHOD 			"RPCMethod"
#define AS_JS_VENDOR 				"Vendor"
#define AS_JS_MODEL 				"Model"
#define AS_JS_FW_VER 				"FirmwareVer"
#define AS_JS_HW_VER 				"HardwareVer"
#define AS_JS_OS_VER 				"OsVer"
#define AS_JS_LOID 					"LOID"
#define AS_JS_MAC 					"MAC"
#define AS_JS_DEVRND 				"DevRND"
#define AS_JS_PTID 					"PlatformID"
#define AS_JS_CARD 					"Card"
#define AS_JS_RETURN 				"Result"
#define AS_JS_CHALLCODE 			"ChallengeCode"
#define AS_JS_SERVERIP 				"ServerIP"
#define AS_JS_INTERVAL 				"Interval"
#define AS_JS_CHECK_SN 				"CheckSN"
#define AS_JS_USER_ID 				"UserID"
#define AS_JS_UPDATE_SERVER 		"UPDATEServer"
#define AS_JS_IP_ADDR 				"IPAddr"
#define AS_JS_KEY 					"Key"
#define AS_JS_FLAG 					"FLAG"
#define AS_JS_LFLAG 				"flag"
#define AS_JS_PPPOENAME				"pppoename"
#define AS_JS_SERVER_PORT 			"ServerPort"
#define AS_JS_SERVER_ADDR 			"ServerAddr"
#define AS_JS_ID 					"ID"
#define AS_JS_PARAM 				"Parameter"
#define AS_JS_APPNAME 				"Plugin_Name"
#define AS_JS_VERSION 				"Version"
#define AS_JS_PLUGIN 				"Plugin"
#define AS_JS_APPVER 				"AppVer"
#define AS_JS_CLOUDC_VER 			"CloudClientVer"
#define AS_JS_MIDDLE_VER 			"MiddleWareVer"
#define AS_JS_CLOUDCB_VER 			"CloudClientBakVer"
#define AS_JS_MIDDLEB_VER 			"MiddleWareBakVer"

//#define AS_JS_PINNAME 				"Plugin_AppName"
#define AS_JS_DOWNURL 				"Download_url"
#define AS_JS_APPSIZE 				"Plugin_size"
#define AS_JS_OS 					"OS"
#define AS_JS_PLAT_MODE				"mode" /*type of platform*/
#define AS_JS_MSG_SERVER 			"MessageServer"

#define AS_JS_NOS_NUMOFBIND 		"numOfBind"
#define AS_JS_NOS_ACTION 			"action"
#define AS_JS_NOS_TID 				"tid"
#define AS_JS_NOS_USERLIST 			"userlist"
#define AS_JS_NOS_APPSIGN 			"AppSign"
#define AS_JS_NOS_APPOPTION 		"AppOption"
#define ADS_JS_RPCMETHOD 			"RPCMethod"
#define ADS_JS_PLUGIN_ID 			"Plugin_ID"
#define ADS_JS_PLUGIN_NAME 			"Plugin_Name"
#define ADS_JS_TIME					"time"
#define ADS_JS_MD5 					"MD5"
#define ADS_JS_MAC					"mac"
#define ADS_JS_MESSAGE				"Message"
#define ADS_JS_RETURN 				"Result"
#define ADS_JS_FAILREASON			"FailReason"
#define ADS_JS_CmdType				"CmdType"
#define ADS_JS_Enable				"Enable"
#define ADS_JS_SequenceId			"SequenceId"
#define ADS_JS_Status				"Status"
#define ADS_JS_List					"List"
#define ADS_JS_Time					"Time"


#define ADS_JS_TYPE					"type"
#define ADS_JS_CATEGORY				"category"
#define ADS_JS_CONTENT				"content"
#define ADS_JS_TITLE				"title"
#define ADS_JS_PHONENUM				"phoneNumber"


enum {
	REG_FLAG_DEFAULT 	= 0,
	REG_FLAG_L_RST 		= 1,
	REG_FLAG_S_RST 		= 2, 
	REG_FLAG_WEB_RST 	= 3, 
	REG_FLAG_ITMS_RST 	= 4, 
	REG_FLAG_APP_RST 	= 5, 
};
enum {
	PCID_SPEC_PLAN, 
	PCID_SPEC_PUSH, // PUSH WINDOW
	PCID_SPEC_BIND_NUM,
	PCID_SPEC_NO_BIND_NUM,
	PCID_SPEC_XG_FROM_HGU,
	PCID_SPEC_CONNECT_AC,
	PCID_SPEC_PLUS,
	PCID_SPEC_INSTALL,
	PCID_SPEC_UNINSTALL,
	PCID_SPEC_TRIGGER,
	PCID_SPEC_NUM,
};
enum {
	UCRET_RESET 			= 3, /*Verify ok , Restore Default*/
	UCRET_VERIFY_OK  		= 2,
	UCRET_ACCEPT			= 1,
	UCRET_OK 				= 0,
	UCRET_ERROR 			= -1,
	UCRET_VERIFY_FAIL 		= -2, /*4*/
	UCRET_RE_REGISTER 		= -3,
	UCRET_TIME_OUT 			= -4,
	UCRET_REDIRECT_SERVER 	= -5,
	UCRET_MAX
};

enum {
	ACRET_ACCEPT			= 1,
	ACRET_OK 				= 0,
	ACRET_ERROR 			= -1,
	ACRET_VERIFY_FAIL 		= -2,
	ACRET_DOWNADDR_ERR 		= -3,
	ACRET_NO_SPACE 			= -4,
	ACRET_BUSY 				= -5,
	ACRET_VER_ERR 			= -6,
	ACRET_SYS_ERR		 	= -7,
	ACRET_SIGN_ERR 			= -8,
	ACRET_DOWNLOAD_FAIL 	= -9,
	ACRET_STARTAPP_FAIL 	= -10,
	ACRET_INSTALL_OK		= -11,
	ACRET_NOSUCH_APP        = -12,
	ACRET_APP_SUSY 			= -13,
	ACRET_CANT_DISABLE 		= -14,
	ACRET_CANT_UNINSTALL 	= -15,
	ACRET_APP_STOP 			= -16,
	ACRET_OP_TIMEOUT 		= -100,
	ACRET_TOKEN_INVALID 	= -1000,
	ACRET_NOSUCH_DEV 		= -1001,
	ACRET_DONT_CONN 		= -1002,
	ACRET_NOT_HB, //private define
};

enum {
	ADVRET_OK 				= 0,
	ADVRET_ERROR 			= -1,
};

enum {
	DISTRET_OK 				= 0,
	DISTRET_NOSUCH_DEV 		= -1,
	DISTRET_NOSUCH_SN 		= -2,
	DISTRET_OPRTID_VALID 	= -5,
	DISTRET_ERROR 			= -9,
};

enum {
	UCMSG_LIVE,
	UCMSG_BOOT,
	UCMSG_REGISTER,
	UCMSG_REREGISTER,
	UCMSG_HB,
	UCMSG_CONNREQ,
	UCMSG_DEV_STATUS,
	UCMSG_GET_DBADDR,
	UCMSG_DEVLIST,
	UCMSG_QUIT,
	UCMSG_CONFIG_REFRESH,
	UCMSG_STAT_INFO,
	UCMSG_QOSLOG,
	UCMSG_DEVRDM_MD5,
	UCMSG_PLAN,
	UCMSG_SN_IMPORT,
	UCMSG_USER_UNBIND,
	UCMSG_USER_BIND,
	UCMSG_NOTIFY_DPI,
	UCMSG_INDEV,
	UCMSG_ACTIVE,
	UCMSG_SSN_REQ,
	UCMSG_SSN_SET,
	UCMSG_SSN_GET,
	UCMSG_ACCESS,
	UCMSG_INFO,
	UCMSG_ABNORMAL,
	UCMSG_REPORT_PLUGIN,
	UCMSG_RECORD_PLUGIN_STATUS,
	UCMSG_DEL_SQL,
	UCMSG_BOOTFIRST,
	UCMSG_REGISTERFIRST,
	UCMSG_DISTRIBUTE,
	UCMSG_MAX,
};

enum {
	ACMSG_LIVE,
	ACMSG_BOOT,
	ACMSG_REGESTER,
	ACMSG_HB,
	ACMSG_CHK_CONN,
	ACMSG_DEV_STATUS,
	ACMSG_SET_PARAM,
	ACMSG_GET_PARAM,
	ACMSG_ADD_OBJECT,
	ACMSG_DEL_OBJECT,
	ACMSG_REBOOT,
	ACMSG_FACTORY_RESET,
	ACMSG_DEVLIST,
	ACMSG_APP_LIST,
	ACMSG_APP_OPERATION,
	ACMSG_APP_POST,
	ACMSG_CHK_UPDATE,
	ACMSG_GET_DBADDR,
	ACMSG_QUIT,
	ACMSG_CONFIG_REFRESH,
	ACMSG_QOSLOG,
	ACMSG_USERINFO,
	ACMSG_PLAN,
	ACMSG_BATCH,
	ACMSG_PUSH_WINDOW,
	ACMSG_GA_TASK,
	ACMSG_DEL_FILES,
	ACMSG_USER_UNBIND,
	ACMSG_USER_BIND,
	ACMSG_GET_LIST,
	ACMSG_INSTALL_LIST,
	ACMSG_SET_CONFIG,
	ACMSG_UNINSTALL_LIST,
	ACMSG_MAX,
};

enum {
	ADVMSG_PUSH_CONF_REFRESH,
	ADVMSG_PUSH_SINGLE_APP,
	ADVMSG_PUSH_ALL_APP,
	ADVMSG_PUSH_TAGS_APP,
	ADVMSG_TAGS_LIST_GET,
	ADVMSG_MSG_STATUS_GET,
	ADVMSG_DB_INFO_GET,
	ADVMSG_MAX,
};

enum {
	UCPA_MSG 				= 1,
	UCPA_RETURN 			= 2,
	UCPA_MD5DIGEST 			= 3,
	UCPA_VENDOR 			= 4,
	UCPA_MODLE 				= 5,
	UCPA_FWVER 				= 6,
	UCPA_HDVER 				= 7,
	UCPA_MAC 				= 8,
	UCPA_IPADDR 			= 9,
	UCPA_PFID 				= 10,
	UCPA_RDM 				= 11,
	UCPA_INTERVAL 			= 12,
	UCPA_SERVER_ADDR 		= 13,
	UCPA_SERVER_PORT 		= 14,
	UCPA_REMOTE_IP 			= 15,
//	UCPA_UPDATE_PORT 		= 16,
	UCPA_HB_IPADDR 			= 17,
	UCPA_PCID 				= 18,
	UCPA_PBID 				= 19,
	UCPA_PLID 				= 20,
	UCPA_DEVINFO 			= 21,
	UCPA_CARD 				= 22,
	UCPA_USERID 			= 23,
	UCPA_ACTION 			= 24,
	UCPA_PT_MODE 			= 25,
	UCPA_DEV_SN 			= 26,
	UCPA_ASID 				= 27,
	UCPA_DB_INFO			= 28,
	UCPA_OPT_TIME			= 29,
	UCPA_PLAT_MODE			= 30,	
	UCPA_JSON				= 31,
	UCPA_CONFIG 			= 32,
	UCPA_LOGID 				= 33,
	UCPA_LOG_DIRECT 		= 34,
	UCPA_LOGMSG 			= 35,
	UCPA_LOGSTEP 			= 36,
	UCPA_S_TIMEVAL 			= 37,
	UCPA_E_TIMEVAL 			= 38,
	UCPA_PLANID				= 39,
	UCPA_STR_PARAM 			= 40,
	UCPA_APP_NAME 			= 41,
	UCPA_APP_OPTION 		= 42,


	UCPA_OSVER 		 		= 53,
	UCPA_LOID 		 		= 54,
	UCPA_PPPOENAME 		 	= 55,
	UCPA_IN_DATA 		 	= 56,
	UCPA_OUT_DATA 		 	= 57,
	UCPA_VALIDSOFT 		 	= 58,
	UCPA_APPVER 		 	= 59,
	UCPA_NUM 			 	= 60,
	UCPA_FLAG 				= 61,
	UCPA_PROVINCE 			= 62,
	UCPA_CITY 				= 63,
	UCPA_OLDMAC 			= 64, 
	UCPA_SN                 = 65,
	UCPA_PID                = 66,
	UCPA_PLU                = 67,
	UCPA_PLUV               = 68,
	UCPA_OP                 = 69,
	UCPA_CLOUDC_VER 		= 70,
	UCPA_MIDDLE_VER 		= 71,
	UCPA_MAX
};

enum {
	UCPA_NUM_NULL = 0,
	UCPA_NUM_LOID_CHANGE,
	UCPA_NUM_MAC_CHANGE,
};

enum {
	ACPA_MSG 				= 1,
	ACPA_RETURN 			= 2,
	ACPA_MD5DIGEST 			= 3,
	ACPA_VENDOR 			= 4,
	ACPA_MODLE 				= 5,
	ACPA_FWVER 				= 6,
	ACPA_HDVER 				= 7,
	ACPA_MAC 				= 8,
	ACPA_IPADDR 			= 9,
	ACPA_PFID 				= 10,
	ACPA_RDM 				= 11,
	ACPA_INTERVAL 			= 12,
	ACPA_SERVER_ADDR 		= 13,
	ACPA_SERVER_PORT 		= 14,
	ACPA_UPDATE_SERVER 		= 15,
	ACPA_HB_IPADDR 			= 16,
	ACPA_PCID 				= 17,  //U32
	ACPA_PBID 				= 19,  //U16
	ACPA_PLID 				= 20,  //U16
	ACPA_DEVINFO 			= 21,
	ACPA_APP_VER 			= 22,
	ACPA_JSON 				= 23,
	ACPA_APP_NAME 			= 24,
	ACPA_CARD 				= 25,
	ACPA_LOID 				= 26,
	ACPA_DEV_SN 			= 27,
	ACPA_ACTION 			= 28,  //U8
	ACPA_STR_PARAM 			= 29,
	ACPA_DB_INFO			= 30,
	ACPA_APP_SIZE 			= 31,  //U32
	ACPA_DOWNLOAD_RUL 		= 32,
//	ACPA_PINAPP_NAME 		= 33,  // Plugin_AppName
	ACPA_OS 				= 34,
	ACPA_LOGID				= 35,
	ACPA_LOG_DIRECT 		= 36,
	ACPA_LOGMSG 			= 37,
	ACPA_LOGSTEP 			= 38,
	ACPA_S_TIMEVAL 			= 39,
	ACPA_E_TIMEVAL 			= 40,
	ACPA_PLANID				= 41,
	ACPA_CONFIG 			= 42,
	ACPA_PUSHID 			= 43,
	ACPA_APPID 				= 44,
	ACPA_IN_DATA 			= 45,
	ACPA_OUT_DATA 			= 46,
	ACPA_BIND 				= 47,
	ACPA_UPGRADE_ID         = 48,
	ACPA_APK_URL            = 49,
	ACPA_CLOUDC_VER 		= 70,
	ACPA_MIDDLE_VER 		= 71,
	ACPA_CLOUDCB_VER 		= 72,
	ACPA_MIDDLEB_VER 		= 73,
	ACPA_PROVINCE 			= 74,
	ACPA_CITY 				= 75,
	ACPA_MAX,
};

enum {
	ADVPA_MSG 				= 1,
	ADVPA_RETURN 			= 2,
	ADVPA_STR_PARAM			= 3,
	ADVPA_JSON				= 4,
	ADVPA_MAC				= 5,
	ADVPA_MAX,
};

enum {
	ACTION_INSTALL 			= 0,
	ACTION_UNINSTALL 		= 1,
	ACTION_START 			= 2,
	ACTION_STOP 			= 3,
	ACTION_FACTORY 			= 4,
	ACTION_PROGRESS 		= 5,
	ACTION_CANCEL_INSTALL 	= 6,
	ACTION_ADD 				= 7,
	ACTION_DEL 				= 8,
	ACTION_CLEAN 			= 9,
	ACTION_UP 				= 10,
	ACTION_DOWN 			= 11,
};

#endif
