all:
	g++ -o sniffer sniffer.cpp signal_handlers.cpp
	g++ -o worker worker.cpp

clean:
	rm -f sniffer
	rm -f worker