RINGBUFFERDIR = $(shell echo ~/)/GIT_RPO/simpleringbuffer/ 

RBCFILES   = getrandom 
RBCXXFILES = pointer shm Clock


RBCOBJS		= $(addprefix ../../simpleringbuffer/, $(RBCFILES))
RBCXXOBJS	= $(addprefix ../../simpleringbuffer/, $(RBCXXFILES))
