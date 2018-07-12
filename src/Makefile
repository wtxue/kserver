include ../kserver.mk

SRC_BASE=base
SRC_TLV=base/TLV
SRC_JSON=base/slothjson/include
SRC_NET=net

BASE_OBJS := $(addsuffix .o, $(basename $(wildcard $(SRC_BASE)/*.cc)))
NET_OBJS := $(addsuffix .o, $(basename $(wildcard $(SRC_NET)/*.cc)))

LIB_OBJS := $(addsuffix .o, $(basename $(wildcard $(SRC_JSON)/*.cpp)))
LIB_OBJS += $(addsuffix .o, $(basename $(wildcard $(SRC_TLV)/*.cpp)))
LIB_OBJS += $(BASE_OBJS) $(NET_OBJS) 

TARGETS = libkserver.a

all: $(TARGETS)

$(TARGETS): $(LIB_OBJS)
	$(AR) $@ $^
	mkdir -p $(ROOT_DIR)lib; cp $@ $(ROOT_DIR)lib

clean:
	$(RM) $(TARGETS)
	$(RM) *.o $(LIB_OBJS)
