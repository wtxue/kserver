include ../../kserver.mk

TARGETS = dns_resolver

.PHONY: all $(TARGETS)

dns_resolver:dns_resolver.o
	$(BUILDEXE)

%.o : %.cc
	$(CPP) -c $(CXXFLAGS) $(BASE_INCLS) $< -o $@ 

clean:
	-rm -f *.o $(TARGETS) 
