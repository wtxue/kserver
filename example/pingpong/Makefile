include ../../kserver.mk

TARGETS := server client

all:$(TARGETS)

.PHONY:all $(TARGETS)

server:server.o
	$(BUILDEXE)

client:client.o
	$(BUILDEXE)

%.o : %.cc
	$(CPP) -c $(CXXFLAGS) $(BASE_INCLS) $< -o $@ 

clean:
	-rm -f *.o $(TARGETS) 
