CC		=	gcc
CFLAGS	=	-g
INCS	=	-I/usr/include/ -I/usr/local/include/ -I/usr/local/libgs/include/libgs/ 
LDS		=	-L/usr/lib64/ -L/usr/local/lib64/ -luuid \
			-L/usr/local/libgs/lib64/libgs/ctlog -lgs_ctlog \
			-L/usr/local/libgs/lib64/libgs/confparser -lgs_confparser 

SHLD    = $(CC) $(CFLAGS)

all:	server child


###### server ######
server: server.o utils.o
	$(SHLD) -o server server.o utils.o $(INCS) $(LDS)

server.o: server.c
	$(SHLD) -c -o server.o server.c $(INCS)

###### child ######
child: child.o utils.o
	$(SHLD) -o child child.o utils.o $(INCS) $(LDS)

child.o: child.c
	$(SHLD) -c -o child.o child.c $(INCS)


###### other ######
utils.o: utils.c
	$(SHLD) -c -o utils.o utils.c $(INCS)



clean:
	rm -rf *.o
	rm -rf server
	rm -rf child

