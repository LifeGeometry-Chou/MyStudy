ALL:server client

server:server.o tcpsocket.o
	gcc -o $@ $^ -lpthread

client:client.o tcpsocket.o
	gcc -o $@ $^

%.o:%.c
	g++ -c -o $@ $^

clean:
	rm -rf server client *.o