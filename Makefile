CC    = clang
CXX   = clang++

CFLAGS =    -O0 -g -Wall -std=c99
CXXFLAGS =  -O0 -g -Wall -std=c++11

COBJS = getrandom
CXXOBJS = main pointer shm 

CFILES = $(addsuffix .c, $(COBJS) )
CXXFILES = $(addsuffix .cpp, $(CXXOBJS) )

OBJS = $(addsuffix .o, $(COBJS) ) $(addsuffix .o, $(CXXOBJS) )

ifneq ($(shell uname -s), Darwin)
RT = -lrt
endif

INCS = 
LIBS = -lpthread $(RT)

compile: $(CXXFILES) $(CFILES)
	$(MAKE) $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCS) $(OBJS) $(LIBS) -o ringb

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(INCS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) $(INCS) -o $@ $<

.PHONY: clean
clean:
	rm -rf ringb $(OBJS)
