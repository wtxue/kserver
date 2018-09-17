
where-am-i = $(abspath $(word $(words $(MAKEFILE_LIST)), $(MAKEFILE_LIST)))
ROOT_DIR := $(dir $(call where-am-i))
PREFIX := $(ROOT_DIR)

UNAME := $(shell uname -s)

##define the compliers
CPP = g++
#AR = ar -rc
AR := ar cruv
RANLIB = ranlib
RM = /bin/rm -f

CXX := g++
AR := ar cruv
MAKE := make

BASE_PATH := $(ROOT_DIR)src/base/
NET_PATH := $(ROOT_DIR)src/net/ 
SLOTHJSON_PATH := $(BASE_PATH)slothjson/include/

SLOTHJSON_INCLS := -I$(SLOTHJSON_PATH)rapidjson/ -I$(SLOTHJSON_PATH)
BASE_INCLS := -I$(ROOT_DIR)/3rdparty/libevent_install/include
BASE_INCLS += $(SLOTHJSON_INCLS) -I$(NET_PATH) -I$(BASE_PATH) -I$(BASE_PATH)/TLV -I$(ROOT_DIR)src/
SQDB_INCLS := -I$(ROOT_DIR)src/db/ -I$(ROOT_DIR)src/db/sequoiadb/include/ 
MYSQL_INCLS := -I/usr/include/mysql
INCLS := $(BASE_INCLS) $(SQDB_INCLS) $(MYSQL_INCLS)

ifeq ($v,release)
CXXFLAGS = -std=c++11 -O2 -fPIC -Wall -Wno-unknown-pragmas 
else
#CXXFLAGS= -std=c++11 -g -O2 -Wall -fPIC -Wno-deprecated  $(INCLS)
#CXXFLAGS = -g -fPIC -Wall -Wno-unused-parameter -Wno-unused-function -Wunused-variable -Wunused-value -Wshadow -Wfloat-equal -Wcast-align -Wwrite-strings -Wsign-compare -Winvalid-pch \
		-fms-extensions  -Wextra -Wno-missing-field-initializers   -Wunknown-pragmas
CXXFLAGS = -std=c++11 -g -fPIC -Wall -Wno-unknown-pragmas
endif

ifneq ($v,release)
BFLAGS= -g
endif

CPPSRCS  = $(wildcard *.cpp)
CCSRCS  = $(wildcard *.cc)
CPPOBJS  = $(patsubst %.cpp,%.o,$(CPPSRCS))
CCOBJS  = $(patsubst %.cc,%.o,$(CCSRCS))

SRCS = $(CPPSRCS) $(CCSRCS)
OBJS = $(CPPOBJS) $(CCOBJS)

BASE_LDFLAGS := -L$(ROOT_DIR)lib -lkserver $(ROOT_DIR)/3rdparty/libevent_install/lib/libevent.a -lpthread -Wl,-Bdynamic -lrt -ldl
SQDB_LDFLAGS := -L$(ROOT_DIR)lib -lkdb $(ROOT_DIR)src/db/sequoiadb/lib/libstaticsdbcpp.a 
MYSQL_LDFLAGS := -L$(ROOT_DIR)lib -lkmysql -L/usr/lib64/mysql -lmysqlclient
SSL_LDFLAGS := $(ROOT_DIR)/3rdparty/libevent_install/lib/libevent_openssl.a -lssl -lcrypto 

LDFLAGS := $(SQDB_LDFLAGS) $(MYSQL_LDFLAGS) $(BASE_LDFLAGS)

BUILDEXE = $(CPP) $(BFLAGS) -o $@ $^ $(BASE_LDFLAGS) 
BUILDEXE_WITH_SQDB = $(CPP) $(BFLAGS) -o $@ $^ $(SQDB_LDFLAGS) $(BASE_LDFLAGS) 
BUILDEXE_WITH_SQDB_MYSQL = $(CPP) $(BFLAGS) -o $@ $^ $(LDFLAGS)
BUILDEXE_WITH_SSL = $(CPP) $(BFLAGS) -o $@ $^ $(BASE_LDFLAGS) $(SSL_LDFLAGS)

# make rule
%.o : %.c
	@echo [CC] $<
	@$(CPP) -c $(CXXFLAGS) $(BASE_INCLS) $< -o $@ 

%.o : %.cc
	@echo [CC] $<
	@$(CPP) -c $(CXXFLAGS) $(BASE_INCLS) $< -o $@ 

%.o : %.cpp
	@echo [CC] $<
	@$(CPP) -c $(CXXFLAGS) $(BASE_INCLS) $< -o $@ 	

