include ../../kserver.mk

SRC_BASE=include

BASE_OBJS = $(addsuffix .o, $(basename $(wildcard $(SRC_BASE)/*.cpp)))

LIB_OBJS = $(BASE_OBJS) 

TARGETS = libslothjson.a

all: $(TARGETS)

$(TARGETS): $(LIB_OBJS)
	$(AR) $@ $^
	mkdir -p $(ROOT_DIR)lib; cp $@ $(ROOT_DIR)lib

clean:
	@( $(RM) $(TARGETS) )
	@( $(RM) *.o $(LIB_OBJS) )

