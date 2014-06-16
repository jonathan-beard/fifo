RINGBUFFERDIR = ../../simpleringbuffer/ 

RBCFILES   = getrandom 
RBCXXFILES = pointer shm Clock


RBCOBJS		= $(addprefix ../../simpleringbuffer/, $(RBCFILES))
RBCXXOBJS	= $(addprefix ../../simpleringbuffer/, $(RBCXXFILES))
