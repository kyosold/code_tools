CC		= gcc
CFLAGS	= -g
INCS	= -I/usr/include/ -I/usr/local/include/
LDS		= -L/usr/lib64/ -L/usr/local/lib64/ -luuid
SHLD	= $(CC) $(CFLAGS)

MASTER	= ctserver
CHILD	= ctchild

all:	$(MASTER) $(CHILD)


###### server ######
$(MASTER):	ctserver.o utils.o ctlog.o ctio.o ./confparser/libconfparser.a
	$(SHLD) -o $(MASTER) ctserver.o utils.o ctlog.o ctio.o \
				./confparser/libconfparser.a $(INCS) $(LDS)

ctserver.o:	ctserver.c
	$(SHLD) -c -o ctserver.o ctserver.c $(INCS)


###### child ######
$(CHILD): ctchild.o utils.o ctlog.o ctio.o ./confparser/libconfparser.a
	$(SHLD) -o $(CHILD) ctchild.o utils.o ctlog.o ctio.o \
				./confparser/libconfparser.a $(INCS) $(LDS)

ctchild.o: ctchild.c
	$(SHLD) -c -o ctchild.o ctchild.c $(INCS)


###### other ######
utils.o: utils.c
	$(SHLD) -c -o utils.o utils.c $(INCS)

ctlog.o: ctlog.c
	$(SHLD) -c -o ctlog.o ctlog.c $(INCS)

ctio.o: ctio.c
	$(SHLD) -c -o ctio.o ctio.c $(INCS)

./confparser/libconfparser.a:
	cd confparser; make -f linux.mk



clean:
	rm -rf *.o
	rm -rf $(MASTER) $(CHILD)
	cd confparser; make -f linux.mk clean;


