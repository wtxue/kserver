include ../kserver.mk

BASE_SRC=./
JSON_OBJS = json

BASE_OBJS := $(addsuffix .o, $(basename $(wildcard $(BASE_SRC)/*.cc)))
BASE_OBJS += $(addsuffix .o, $(basename $(wildcard $(JSON_OBJS)/*.cpp)))

TARGETS = ap_log_server 

.PHONY: all $(TARGETS)

$(TARGETS): $(BASE_OBJS)
	$(BUILDEXE_WITH_SQDB)
	mkdir -p $(ROOT_DIR)bin; cp $@ $(ROOT_DIR)bin

%.o : %.cc
	$(CPP) -c $(CXXFLAGS) $(INCLS) $< -o $@ 

clean:
	-rm -f *.o *.d $(BASE_OBJS) $(TARGETS) 
