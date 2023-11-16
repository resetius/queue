queue: queue.o
	g++ $^ -o $@

%.o: %.cpp
	g++ -g -O -c $^ -o $@

clean:
	rm *.o queue
