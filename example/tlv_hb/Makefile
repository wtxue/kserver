include ../../kserver.mk

BASE_SRC=./

BASE_OBJS := $(addsuffix .o, $(basename $(wildcard $(BASE_SRC)/*.cc)))

TARGETS = tlv_hb

.PHONY: all $(TARGETS)

$(TARGETS): $(BASE_OBJS)
	$(BUILDEXE)

%.o : %.cc
	$(CPP) -c $(CXXFLAGS) $(BASE_INCLS) $< -o $@ 

clean:
	-rm -f *.o $(BASE_OBJS) $(TARGETS) 
