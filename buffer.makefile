RINGBUFFERDIR = ../../fifo_dev/

RBCFILES   = getrandom 
RBCXXFILES = pointer shm Clock procwait resolution fifo systeminfo


RBCOBJS		= $(addprefix ../../fifo_dev/, $(RBCFILES))
RBCXXOBJS	= $(addprefix ../../fifo_dev/, $(RBCXXFILES))
