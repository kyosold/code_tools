CC		= gcc
CFLAGS	= -g
INCS	= -I/usr/include/ -I/usr/local/include/
LDS		= -L/usr/lib64/ -L/usr/local/lib64/ -luuid
SHLD	= $(CC) $(CFLAGS)

MASTER	= ct_server
CHILD	= ct_child

all:	$(MASTER) $(CHILD)


###### server ######
$(MASTER):	ct_server.o utils.o ctlog.o ./confparser/libconfparser.a
	$(SHLD) -o $(MASTER) ct_server.o utils.o ctlog.o \
				./confparser/libconfparser.a $(INCS) $(LDS)

ct_server.o:	ct_server.c
	$(SHLD) -c -o ct_server.o ct_server.c $(INCS)


###### other ######
utils.o: utils.c
	$(SHLD) -c -o utils.o utils.c $(INCS)

ctlog.o: ctlog.c
	$(SHLD) -c -o ctlog.o ctlog.c $(INCS)

./confparser/libconfparser.a:
	cd confparser; make -f linux.mk



clean:
	rm -rf *.o
	rm -rf $(MASTER) $(CHILD)
	cd confparser; make -f linux.mk clean;


