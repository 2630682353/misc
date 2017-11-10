
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
MYOBJS= memory_pool.o  sock.o thread_pool.o 

all1:target client server client2

target: $(MYOBJS) client1.o client2.o server1.o message.o debug.o
	ar cr libipc.a $(MYOBJS) message.o debug.o 

client:
	$(CC) client1.o -o client -L. -lipc -lpthread -g
client2:
	$(CC) client2.o -o client2 -L. -lipc -lpthread -g
server:
	$(CC) server1.o -o server -L. -lipc -lpthread -g
message.o:
	$(CC) message.c -c -o message.o -DDEBUG -g
debug.o:
	$(CC) debug.c -c -o debug.o -DDEBUG_STDOUT

%.o: %.c
	$(CC) -c -o $@ $<

clean:
	rm -rf .depfile *.o *.so libipc.a client server client2