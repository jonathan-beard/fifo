RINGBUFFERDIR = ../../fifo_dev/

RBCFILES   = getrandom 
RBCXXFILES = pointer shm Clock procwait resolution fifo 


RBCOBJS		= $(addprefix ../../fifo_dev/, $(RBCFILES))
RBCXXOBJS	= $(addprefix ../../fifo_dev/, $(RBCXXFILES))
