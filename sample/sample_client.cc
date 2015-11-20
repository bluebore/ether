// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: yanshiguang02@baidu.com

#include "src/pbrpc.h"

#include <mutex.h>
#include "sample/echo.pb.h"

namespace baidu {
namespace ether_sample {

Mutex g_mu;
CondVar g_cv(&g_mu);

void EchoCallback(EchoResponse* response,
                  ether::RpcController* cntl) {
    if (cntl->Failed()) {
        fprintf(stderr, "Echo failed\n");
    } else {
        fprintf(stderr, "Echo from server: %s\n", response->msg().c_str());
    }
    delete response;
    delete cntl;

    MutexLock lock(&g_mu);
    g_cv.Signal();
}

int Run() {
    ether::RpcChannel* channel = new ether::RpcChannel("localhost:8810");
    EchoService* service = new EchoService::Stub(channel);

    EchoRequest request;
    request.set_msg("hi");
    EchoResponse* response = new EchoResponse();
    ether::RpcController* cntl = new ether::RpcController();

    google::protobuf::Closure* callback = 
        google::protobuf::NewCallback(&EchoCallback, response, cntl);
    MutexLock lock(&g_mu);
    service->Echo(cntl, &request, response, callback);
    g_cv.Wait();
    return 0;
}
}
}

int main(int argc, char* argv[]) {
    return baidu::ether_sample::Run();
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
