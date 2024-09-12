CC = gcc
CFLAGS = -Wall

all: client server

# client
client: client.o modbus.o packet.o
	$(CC) $(CFLAGS) -o client client.o modbus.o packet.o

client.o: client.c
	$(CC) $(CFLAGS) -c -o client.o client.c

# server
server: server.o modbus.o packet.o
	$(CC) $(CFLAGS) -o server server.o modbus.o packet.o

server.o: server.c
	$(CC) $(CFLAGS) -c -o server.o server.c

# packet
packet.o: packet.c
	$(CC) $(CFLAGS) -c -o packet.o packet.c

# modbus
modbus.o: modbus.c
	$(CC) $(CFLAGS) -c -o modbus.o modbus.c

clean:
	rm *.o client server