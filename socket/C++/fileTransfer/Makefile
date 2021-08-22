ALL:fileserver fileclient

fileserver:fileserver.o tcpsocket.o
	g++ -o $@ $^ -lpthread

fileclient:fileclient.o tcpsocket.o
	g++ -o $@ $^

%.o:%.cpp
	g++ -c -o $@ $^

clean:
	rm -rf fileserver fileclient *.o