// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: yanshiguang02@baidu.com

#include <boost/bind.hpp>

#include <common/thread.h>
#include <common/thread_pool.h>
#include <counter.h>

#include "echo.pb.h"
#include <src/pbrpc.h>

baidu::common::Counter pending;

std::string server_addr = "localhost:8810";
void EchoCallback(baidu::ether::RpcController* cntl, EchoResponse* response) {
    if (cntl->Failed()) {
        // SLOG(ERROR, "rpc failed: %s", cntl->ErrorText().c_str());
    } else {
    }

    pending.Dec();
    delete cntl;
    // delete request;
    delete response;
}

void TestEcho(int id) {
    baidu::ether::RpcChannel* rpc_channel = new baidu::ether::RpcChannel(server_addr);
    EchoService_Stub* stub = new EchoService_Stub(rpc_channel);
    EchoRequest* request = new EchoRequest();
    request->set_msg("a");
    while (1) {
        while(pending.Get() > 1000) {
        }

        EchoResponse* response = new EchoResponse();
        baidu::ether::RpcController* cntl = new baidu::ether::RpcController();
        google::protobuf::Closure* done =
            google::protobuf::NewCallback(&EchoCallback, cntl, response);

        pending.Inc();
        stub->Echo(cntl, request, response, done);
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
       server_addr = argv[1];
    }
    const int tn = 1;
    baidu::common::Thread* t = new baidu::common::Thread[tn];
    for (int i = 0; i < tn; i++) {
        t[i].Start(boost::bind(&TestEcho, i));
    }
    while(1)
        sleep(1);

    delete[] t;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
