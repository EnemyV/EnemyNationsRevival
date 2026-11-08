CC=sc
CFLAGS=-ml
LNK=sc
OBJS=tfrmwk.obj vpengine.obj psdnet.obj smap.obj

tfrmwk.exe: $(OBJS)
	$(LNK) $(LFLASGS)  $(OBJS)




.cpp.obj:
	$(CC) $(CFLAGS) -c  $<


.include depend
