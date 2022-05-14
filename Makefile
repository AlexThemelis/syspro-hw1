all:
	g++ -o manager manager.cpp
	g++ -o worker worker.cpp

run:
	./manager

clean:
	rm -f manager
	rm -f worker