include ../kserver.mk

SRC_BASE=base
SRC_NET=net

BASE_OBJS = $(addsuffix .o, $(basename $(wildcard $(SRC_BASE)/*.cc)))
BASE_OBJS += $(addsuffix .o, $(basename $(wildcard $(SRC_NET)/*.cpp)))

NET_OBJS = $(addsuffix .o, $(basename $(wildcard $(SRC_NET)/*.cc)))
NET_OBJS += $(addsuffix .o, $(basename $(wildcard $(SRC_NET)/*.cpp)))

LIB_OBJS = $(BASE_OBJS) $(NET_OBJS) 

TARGETS = libkserver.a

all: $(TARGETS)

$(TARGETS): $(LIB_OBJS)
	$(AR) $@ $^
	mkdir -p $(ROOT_DIR)lib; cp $@ $(ROOT_DIR)lib

clean:
	@( $(RM) $(TARGETS) )
	@( $(RM) *.o $(LIB_OBJS) )
