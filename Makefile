CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11

all: client serverA serverM serverR

client: client.cpp
	$(CXX) $(CXXFLAGS) -o client client.cpp -lpthread

serverA: serverA.cpp
	$(CXX) $(CXXFLAGS) -o serverA serverA.cpp -lpthread

serverM: serverM.cpp
	$(CXX) $(CXXFLAGS) -o serverM serverM.cpp -lpthread

serverR: serverR.cpp
	$(CXX) $(CXXFLAGS) -o serverR serverR.cpp -lpthread

# Clean up
clean:
	rm -f client serverA serverM serverR
