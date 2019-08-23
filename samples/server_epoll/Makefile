CC		=	gcc
CFLAGS	=	-g
INCS	=	-I/usr/include/ -I/usr/local/include/ 
LDS		=	-L/usr/lib64/ -L/usr/local/lib64/ -luuid \

SHLD    = $(CC) $(CFLAGS)

SER		= ep_server
CHILD	= ep_child

all:	$(SER) $(CHILD)


###### server ######
$(SER): server.o utils.o ./confparser/libconfparser.a ctlog.o
	$(SHLD) -o $(SER) server.o utils.o ./confparser/libconfparser.a ctlog.o $(INCS) $(LDS)

server.o: server.c
	$(SHLD) -c -o server.o server.c $(INCS)

###### child ######
$(CHILD): child.o utils.o ./confparser/libconfparser.a ctlog.o
	$(SHLD) -o $(CHILD) child.o utils.o ./confparser/libconfparser.a ctlog.o $(INCS) $(LDS)

child.o: child.c
	$(SHLD) -c -o child.o child.c $(INCS)


###### other ######
utils.o: utils.c
	$(SHLD) -c -o utils.o utils.c $(INCS)

ctlog.o: ctlog.c
	$(SHLD) -c -o ctlog.o ctlog.c $(INCS)

./confparser/libconfparser.a:
	cd confparser; make -f linux.mk



clean:
	rm -rf *.o
	rm -rf $(SER)
	rm -rf $(CHILD)
	cd confparser; make -f linux.mk clean;

