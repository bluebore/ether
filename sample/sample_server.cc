// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: yanshiguang02@baidu.com

#include "echo.pb.h"

#include "src/pbrpc.h"
#include <counter.h>

baidu::common::Counter g_counter;

namespace baidu {
namespace ether_sample {

class EchoServiceImpl : public EchoService {
public:
    virtual void Echo(::google::protobuf::RpcController* controller,
                       const EchoRequest* request,
                       EchoResponse* response,
                       ::google::protobuf::Closure* done) {
        response->set_msg("e");
        g_counter.Inc();
        done->Run();
    }
};

}
}

int main(int argc, char* argv[]) {
    baidu::ether::RpcServerOptions options;
    baidu::ether::RpcServer rpc_server(options);

    if (!rpc_server.Start("0.0.0.0:8810")) {
        fprintf(stderr, "start server failed\n");
        return 1;
    }   

    baidu::ether_sample::EchoServiceImpl* service = new baidu::ether_sample::EchoServiceImpl();
    if (!rpc_server.RegisterService(service)) {
        fprintf(stderr, "register service failed\n");
        return 1;
    }   

    while(1) {
        usleep(1000000);
        fprintf(stderr, "%ld\n", g_counter.Clear());
    }   
    rpc_server.Stop();
    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
