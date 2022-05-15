all:
	g++ -o sniffer manager.cpp
	g++ -o worker worker.cpp

clean:
	rm -f sniffer
	rm -f worker