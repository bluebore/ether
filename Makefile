
CXX=g++
CXXFLAGS=-g -Wall -Werror -I./common/include
all: client server


clean:
	rm -rf client server *.o
client: src/client.cc
	$(CXX) $(CXXFLAGS) src/client.cc -o client -lpthread

server: src/server.cc
	$(CXX) $(CXXFLAGS) src/server.cc -o server -lpthread


