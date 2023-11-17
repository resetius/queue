All: queue server client

queue: queue.o queue.h
	g++ $< -o $@

server: server.o queue.h
	g++ $< -o $@

client: client.o queue.h
	g++ $< -o $@

%.o: %.cpp
	g++ -g -O -Wall -c $^ -o $@

clean:
	rm *.o queue client server
