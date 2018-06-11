version = 0.8.0

all:
	@( cd src; make )
	@( cd example/pingpong; make )

clean:
	@( rm -rf lib/* )
	@( cd example/pingpong; make clean )
	@( cd src; make clean )
