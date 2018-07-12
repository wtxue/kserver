version = 0.8.0

all:
	@( cd src; make )
	@( cd src/db; make )
	@( cd dispatcher; make )


clean:
	@( rm -rf lib/* )
	@( cd dispatcher; make clean )
	@( cd src/db; make clean )
	@( cd src; make clean )
