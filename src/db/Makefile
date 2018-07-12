include ../../kserver.mk

SRC_BASE=.

#BASE_OBJS = $(addsuffix .o, $(basename $(wildcard $(SRC_BASE)/*.cc)))
#LIB_OBJS = $(BASE_OBJS) 

SQDB_OBJS := db_base.o
MYSQL_OBJS += mysqlPool.o

SQDB_TARGETS = libkdb.a
MYSQL_TARGETS = libkmysql.a

all: $(SQDB_TARGETS) $(MYSQL_TARGETS)

$(SQDB_TARGETS): $(SQDB_OBJS)
	$(AR) $@ $^
	mkdir -p $(ROOT_DIR)lib; cp $@ $(ROOT_DIR)lib

$(MYSQL_TARGETS): $(MYSQL_OBJS)
	$(AR) $@ $^
	mkdir -p $(ROOT_DIR)lib; cp $@ $(ROOT_DIR)lib

db_base.o : db_base.cc
	@echo [CC] $<
	@$(CPP) -c $(CXXFLAGS)  $(BASE_INCLS) $(SQDB_INCLS) $< -o $@ 

mysqlPool.o : mysqlPool.cc
	@echo [CC] $<
	@$(CPP) -c $(CXXFLAGS)  $(BASE_INCLS) $(MYSQL_INCLS) $< -o $@

clean:
	$(RM) $(SQDB_TARGETS) $(MYSQL_TARGETS) *.o
