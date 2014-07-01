RINGBUFFERDIR = ../../simpleringbuffer/ 

RBCFILES   = getrandom 
RBCXXFILES = pointer shm Clock systeminfo 


RBCOBJS		= $(addprefix ../../simpleringbuffer/, $(RBCFILES))
RBCXXOBJS	= $(addprefix ../../simpleringbuffer/, $(RBCXXFILES))
