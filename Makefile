CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

all: client serverA serverM serverR serverD

client: client.cpp
	$(CXX) $(CXXFLAGS) -o client client.cpp

serverA: serverA.cpp
	$(CXX) $(CXXFLAGS) -o serverA serverA.cpp

serverM: serverM.cpp
	$(CXX) $(CXXFLAGS) -o serverM serverM.cpp -lpthread

serverR: serverR.cpp
	$(CXX) $(CXXFLAGS) -o serverR serverR.cpp

serverD: serverD.cpp
	$(CXX) $(CXXFLAGS) -o serverD serverD.cpp

# Clean up
clean:
	rm -f client serverA serverM serverR serverD
