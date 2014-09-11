RINGBUFFERDIR = ../../simpleringbuffer/ 

RBCFILES   = getrandom 
RBCXXFILES = pointer shm Clock procwait resolution fifo 


RBCOBJS		= $(addprefix ../../simpleringbuffer/, $(RBCFILES))
RBCXXOBJS	= $(addprefix ../../simpleringbuffer/, $(RBCXXFILES))
