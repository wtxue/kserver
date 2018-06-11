include ../../kserver.mk


TARGETS = server client

.PHONY: all $(TARGETS)

server:server.o
	$(BUILDEXE)

client:client.o
	$(BUILDEXE)

clean:
	-rm -f *.o *.d $(TARGETS) 
