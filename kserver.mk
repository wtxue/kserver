
where-am-i = $(abspath $(word $(words $(MAKEFILE_LIST)), $(MAKEFILE_LIST)))
ROOT_DIR := $(dir $(call where-am-i))
PREFIX := $(ROOT_DIR)

UNAME := $(shell uname -s)

##define the compliers
CPP = g++
#AR = ar -rc
AR := ar cruv
RANLIB = ranlib
RM = /bin/rm -f

CXX := g++
AR := ar cruv
MAKE := make


INCLS := -I$(ROOT_DIR)src/base -I$(ROOT_DIR)src/net -I$(ROOT_DIR)src/ 


ifeq ($v,release)
CXXFLAGS= -std=c++11 -O2 -fPIC  -DLINUX -pipe -Wno-deprecated  $(INCLS) 
else
#CXXFLAGS= -std=c++11 -g -O2 -Wall -fPIC -Wno-deprecated  $(INCLS)
#CXXFLAGS = -g -fPIC -Wall -Wno-unused-parameter -Wno-unused-function -Wunused-variable -Wunused-value -Wshadow 
CXXFLAGS = -g -fPIC -Wall -Wcast-align -Wwrite-strings -Wsign-compare -Winvalid-pch -Wunused-variable\
		-fms-extensions -Wfloat-equal -Wextra -Wno-missing-field-initializers -std=c++11
endif

ifneq ($v,release)
BFLAGS= -g
endif

CPPSRCS  = $(wildcard *.cpp)
CCSRCS  = $(wildcard *.cc)
CPPOBJS  = $(patsubst %.cpp,%.o,$(CPPSRCS))
CCOBJS  = $(patsubst %.cc,%.o,$(CCSRCS))

SRCS = $(CPPSRCS) $(CCSRCS)
OBJS = $(CPPOBJS) $(CCOBJS)

LDFLAGS := -L$(ROOT_DIR)lib -lkserver  /usr/local/lib/libevent.a -lpthread -Wl,-Bdynamic -lrt -ldl

BUILDEXE = $(CPP) $(BFLAGS) -o $@ $^ $(LDFLAGS) 

# make rule

%.o : %.c
	$(CPP) -c $(CXXFLAGS) $(INCLS) $< -o $@ 

%.o : %.cc
	$(CPP) -c $(CXXFLAGS) $(INCLS) $< -o $@ 

%.o : %.cpp
	$(CPP) -c $(CXXFLAGS) $(INCLS) $< -o $@ 	

