// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: yanshiguang02@baidu.com

#ifndef  BAIDU_ETHER_PBRPC_H_
#define  BAIDU_ETHER_PBRPC_H_

#include <map>

#include <google/protobuf/service.h>

#include <mutex.h>
#include <thread.h>
#include <thread_pool.h>

namespace baidu {
namespace ether {

enum RpcErrorCode {
    RpcOk = 0,
    RpcNotConnected = 1,
    RpcUserSetError = 2,
};

class RpcController : public google::protobuf::RpcController {
public:
    RpcController();
    virtual ~RpcController();
public:
    // Resets the RpcController to its initial.
    virtual void Reset();

    // Returns true if the call failed.
    virtual bool Failed() const;

    // If Failed() is true, returns a human-readable description of the error.
    virtual std::string ErrorText() const;

    // Advises the RPC system that the caller desires that the RPC call be canceled.
    virtual void StartCancel();

    // Causes Failed() to return true on the client side.
    virtual void SetFailed(const std::string& reason);

    // If true, indicates that the client canceled the RPC.
    virtual bool IsCanceled() const;

    // Asks that the given callback be called when the RPC is canceled.
    virtual void NotifyOnCancel(google::protobuf::Closure* callback);
public:
    void SetErrorCode(RpcErrorCode ec) {error_code_ = ec; }
private:
    std::string error_text_;
    RpcErrorCode error_code_;
};

class Socket;

struct RpcItem {
    google::protobuf::Message* response;
    google::protobuf::RpcController* controller;
    google::protobuf::Closure* done;
    RpcItem(google::protobuf::Message* p, 
            google::protobuf::RpcController* c,
            google::protobuf::Closure* d)
        : response(p), controller(c), done(d) {}
};

class  RpcChannel : public google::protobuf::RpcChannel {
public:
    RpcChannel(const std::string& remote_addr);
    virtual ~RpcChannel();

    // Call the given method of the remote service.
    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
            google::protobuf::RpcController* controller,
            const google::protobuf::Message* request,
            google::protobuf::Message* response,
            google::protobuf::Closure* done);
public:
    void OnConnect(bool success, const std::string& error_msg);
    void WriteDone();
    void Recv(const char* buf, int len);
    void Callback(google::protobuf::Closure* done);
private:
    Socket* socket_;
    std::string recv_buf_;
    typedef std::map<int64_t, RpcItem*> CallBackMap;
    CallBackMap callback_map_;
    volatile int64_t last_sequnce_;
    common::Mutex map_lock_;
    bool    connected_;
    common::ThreadPool thread_pool_;
};

struct RpcServerOptions {
};


typedef std::map<std::string, google::protobuf::Service*> ServiceMap;
class RpcServer {
public:
    RpcServer();
    RpcServer(const RpcServerOptions& options);
    ~RpcServer();
    bool RegisterService(google::protobuf::Service* service);
    ServiceMap GetServiceMap();
    
    bool Start(const std::string& bind_address);
    bool Stop();
    void Loop();
    void Worker();
private:
    ServiceMap service_map_;
    common::Thread loop_thread_;
    int listen_fd_;
    Mutex lock_;
};


}
}

#endif  // BAIDU_ETHER_PBRPC_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
