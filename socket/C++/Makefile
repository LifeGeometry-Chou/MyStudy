ALL:server client

server:server.o tcpsocket.o
	g++ -o $@ $^ -lpthread

client:client.o tcpsocket.o
	g++ -o $@ $^

%.o:%.cpp
	g++ -c -o $@ $^

clean:
	rm -rf server client *.o