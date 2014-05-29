
CXX=g++
CXXFLAGS=-g -Wall
all: client server

clean:
	rm -rf client server *.o
client: client.cc
	$(CXX) $(CXXFLAGS) client.cc -o client -lpthread

server: server.cc
	$(CXX) $(CXXFLAGS) server.cc -o server -lpthread


