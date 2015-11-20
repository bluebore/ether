include depends.mk

# OPT ?= -O2        # (A) Production use (optimized mode)
OPT ?= -g2      # (B) Debug mode, w/ full line-level debugging symbols
# OPT ?= -O2 -g2  # (C) Profiling mode: opt, but w/debugging symbols

CXX=g++
INCPATH=-I. -I./common/include -I$(PROTOBUF_DIR)/include
CXXFLAGS=$(OPT) -pipe -Wall -fPIC $(INCPATH)

LDFLAGS=-L./common/ -lcommon -L$(PROTOBUF_DIR)/lib/ -lprotobuf -lpthread

PROTO_FILE := $(wildcard src/proto/*.proto)
PROTO_SRC := $(PROTO_FILE:.proto=.pb.cc) 
PROTO_OBJ := $(PROTO_FILE:.proto=.pb.o)
HEADER_FILE := include/ether/pbrpc.h 
all: client server libether.a sample_client sample_server perf_client

sample_client: sample/sample_client.o sample/echo.pb.o libether.a $(HEADER_FILE)
	$(CXX) $(CXXFLAGS) sample/sample_client.o sample/echo.pb.o -o $@ -L./ -lether $(LDFLAGS)

perf_client: sample/perf_client.o sample/echo.pb.o libether.a $(HEADER_FILE)
	$(CXX) $(CXXFLAGS) sample/perf_client.o sample/echo.pb.o -o $@ -L./ -lether $(LDFLAGS)

sample_server: sample/sample_server.o sample/echo.pb.o libether.a $(HEADER_FILE)
	$(CXX) $(CXXFLAGS) sample/sample_server.o sample/echo.pb.o -o $@ -L./ -lether $(LDFLAGS)

$(HEADER_FILE): src/pbrpc.h
	mkdir -p include/ether
	cp src/pbrpc.h include/ether

clean:
	rm -rf client server src/*.o libether.a $(PROTO_SRC) $(PROTO_OBJ) sample/*.o sample_*

client: src/client.cc $(PROTO_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

server: src/server.cc $(PROTO_SRC)
	$(CXX) $(CXXFLAGS) src/server.cc -o server $(LDFLAGS)

libether.a: src/pbrpc.o src/socket.o $(PROTO_OBJ)
	ar rs $@ $^

%.pb.cc: %.proto
	$(PROTOBUF_DIR)/bin/protoc $(PROTO_OPTIONS) --cpp_out=. $<

%.pb.h: %.proto
	$(PROTOBUF_DIR)/bin/protoc $(PROTO_OPTIONS) --cpp_out=. $<

$(PROTO_OBJ):%.o: %.cc %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

