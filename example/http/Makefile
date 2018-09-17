include ../../kserver.mk

TARGETS = httpc

.PHONY: all $(TARGETS)

httpc:client.o
	$(BUILDEXE_WITH_SSL)

%.o : %.cc
	$(CPP) -c $(CXXFLAGS) $(BASE_INCLS) $< -o $@ 

clean:
	-rm -f *.o $(TARGETS) 
