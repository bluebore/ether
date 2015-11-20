// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: yanshiguang02@baidu.com

#include "pbrpc.h"

#include <boost/bind.hpp>
#include <logging.h>

#include "socket.h"
#include "src/proto/rpc.pb.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

namespace baidu {
namespace ether {


RpcController::RpcController() : error_code_(RpcOk) {}
RpcController::~RpcController() {}

// Client-side methods ---------------------------------------------
// These calls may be made from the client side only.  Their results
// are undefined on the server side (may crash).

// Resets the RpcController to its initial state so that it may be reused in
// a new call.  Must not be called while an RPC is in progress.
void RpcController::Reset() {}

// After a call has finished, returns true if the call failed.  The possible
// reasons for failure depend on the RPC implementation.  Failed() must not
// be called before a call has finished.  If Failed() returns true, the
// contents of the response message are undefined.
bool RpcController::Failed() const {
    return error_code_ != RpcOk;
}

// If Failed() is true, returns a human-readable description of the error.
std::string RpcController::ErrorText() const {
    if (Failed()) {
        return error_text_;
    } else {
        return "";
    }
}

// Advises the RPC system that the caller desires that the RPC call be
// canceled.  The RPC system may cancel it immediately, may wait awhile and
// then cancel it, or may not even cancel the call at all.  If the call is
// canceled, the "done" callback will still be called and the RpcController
// will indicate that the call failed at that time.
void RpcController::StartCancel() {}

// Server-side methods ---------------------------------------------
// These calls may be made from the server side only.  Their results
// are undefined on the client side (may crash).

// Causes Failed() to return true on the client side.  "reason" will be
// incorporated into the message returned by ErrorText().  If you find
// you need to return machine-readable information about failures, you
// should incorporate it into your response protocol buffer and should
// NOT call SetFailed().
void RpcController::SetFailed(const std::string& reason) {
    error_text_ = reason;
    error_code_ = RpcUserSetError;
}

// If true, indicates that the client canceled the RPC, so the server may
// as well give up on replying to it.  The server should still call the
// final "done" callback.
bool RpcController::IsCanceled() const {
    return false;
}

// Asks that the given callback be called when the RPC is canceled.  The
// callback will always be called exactly once.  If the RPC completes without
// being canceled, the callback will be called after completion.  If the RPC
// has already been canceled when NotifyOnCancel() is called, the callback
// will be called immediately.
//
// NotifyOnCancel() must be called no more than once per request.
void RpcController::NotifyOnCancel(google::protobuf::Closure* callback) {}

struct RpcHeader {
    int32_t meta_len;
    int32_t request_len;
};

RpcChannel::RpcChannel(const std::string& remote_addr)
    : connected_(false), thread_pool_(1) {
    socket_ = new Socket();
    socket_->SetConsumer(boost::bind(&RpcChannel::Recv, this, _1, _2));
    socket_->StartConnect(remote_addr, boost::bind(&RpcChannel::OnConnect, this, _1, _2));
}

RpcChannel::~RpcChannel() {}

void RpcChannel::Callback(google::protobuf::Closure* done) {
    done->Run();
}
// Call the given method of the remote service.  The signature of this
// procedure looks the same as Service::CallMethod(), but the requirements
// are less strict in one important way:  the request and response objects
// need not be of any specific class as long as their descriptors are
// method->input_type() and method->output_type().
void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                                    google::protobuf::RpcController* controller,
                                    const google::protobuf::Message* request,
                                    google::protobuf::Message* response,
                                    google::protobuf::Closure* done) {
    if (!connected_) {
        reinterpret_cast<RpcController*>(controller)->SetErrorCode(RpcNotConnected);
        thread_pool_.AddTask(boost::bind(&RpcChannel::Callback, this, done));
        return;
    }
    ///TODO: 多线程调用
    RpcMeta meta;
    meta.set_method(method->full_name());
    meta.set_sequence_id(++last_sequnce_);

    std::string packet;
    packet.resize(sizeof(RpcHeader) + meta.ByteSize() + request->ByteSize());
    
    char * p = const_cast<char*>(packet.data());
    RpcHeader* header = reinterpret_cast<RpcHeader*>(p);
    header->meta_len = meta.ByteSize();
    header->request_len = request->ByteSize();

    p += sizeof(RpcHeader);
    meta.SerializeToArray(p, meta.ByteSize());
    p += meta.ByteSize();
    request->SerializeToArray(p, request->ByteSize());
    RpcItem* item = new RpcItem(response, controller, done);
    {
        MutexLock lock(&map_lock_);
        callback_map_.insert(std::make_pair(meta.sequence_id(), item));
    }
    socket_->Write(packet, 
            boost::bind(&RpcChannel::WriteDone, this));
}

void RpcChannel::WriteDone() {
}

void RpcChannel::OnConnect(bool success, const std::string& error_msg) {
    if (!success) {
        fprintf(stderr, "Connect fail: %s\n", error_msg.c_str());
    } else {
        connected_ = true;
    }
}

void RpcChannel::Recv(const char* buf, int len) {
    if (len <= 0 || buf == NULL) {
        return;
    }
    recv_buf_.append(buf, len);
    if (recv_buf_.size() >= sizeof(RpcHeader)) {
        const RpcHeader* header = reinterpret_cast<const RpcHeader*>(recv_buf_.data());
        uint32_t packet_len = sizeof(RpcHeader) + header->meta_len + header->request_len;
        if (recv_buf_.size() >= packet_len) {
            /// callback
            RpcMeta meta;
            if (!meta.ParseFromArray(recv_buf_.data() + sizeof(RpcHeader), header->meta_len)) {
                LOG(WARNING, "Parse response pack fail");
            }
            int64_t seq = meta.sequence_id();
            RpcItem* item = NULL;
            {
                MutexLock lock(&map_lock_);
                CallBackMap::iterator it = callback_map_.find(seq);
                assert(it != callback_map_.end());
                item = it->second;
                callback_map_.erase(it);
            }
            item->response->ParseFromArray(recv_buf_.data() + sizeof(RpcHeader) + header->meta_len,
                                           header->request_len);
            item->done->Run();
            delete item;
            /// next
            recv_buf_ = recv_buf_.substr(packet_len);
        }
    }
}


class RpcConnection {
public:
    RpcConnection(RpcServer* rpc_server, int fd) : fd_(fd) {
        service_map_ = rpc_server->GetServiceMap();
    }
    google::protobuf::Message* CallMethod(const std::string& service_name,
                                         const std::string& method_name,
                                         const char* data, int32_t len) {
        ServiceMap::iterator it = service_map_.find(service_name);
        if (it == service_map_.end()) {
            LOG(WARNING, "No service %s", service_name.c_str());
            return NULL;
        }
        google::protobuf::Service* service = it->second;
        const google::protobuf::MethodDescriptor* method_desc =
            service->GetDescriptor()->FindMethodByName(method_name);

        google::protobuf::Message* request = service->GetRequestPrototype(method_desc).New();
        if (!request->ParseFromArray(data, len)) {
            LOG(WARNING, "Parse reqeust fail");
            delete request;
            return NULL;
        }
        google::protobuf::Message* response = service->GetResponsePrototype(method_desc).New();
        RpcController* cntl = new RpcController();
        google::protobuf::Closure* done = google::protobuf::NewCallback(this,
            &RpcConnection::CallMethodDone, request, response);
        service->CallMethod(method_desc, NULL, request, response, done);
        delete cntl;
        delete request;
        return response;
    }
    void CallMethodDone(google::protobuf::Message* request,
                        google::protobuf::Message* response) {
    }
    void ParseRequest(const RpcHeader* header) {
        RpcMeta meta;
        if (!meta.ParseFromArray(recv_buf_.data() + sizeof(RpcHeader), header->meta_len)) {
            LOG(WARNING, "Parse packet fail");
        }
        size_t pos = meta.method().rfind('.');
        if (pos == std::string::npos) {
            LOG(WARNING, "bad method full name: %s", meta.method().c_str());
        } else {
            std::string method_name = meta.method().substr(pos+1);
            std::string service_name = meta.method().substr(0, pos);
            LOG(DEBUG, "Recv methed %s-%s id %ld",
                service_name.c_str(), method_name.c_str(), meta.sequence_id());
            google::protobuf::Message* response = CallMethod(service_name, method_name,
                       recv_buf_.data() + sizeof(RpcHeader) + header->meta_len,
                       header->request_len);
            if (response) {
                std::string str(recv_buf_.data(), sizeof(RpcHeader) + header->meta_len);
                str.resize(sizeof(RpcHeader) + header->meta_len + response->ByteSize());
                char* str_buf = const_cast<char*>(str.data());
                RpcHeader* new_header = reinterpret_cast<RpcHeader*>(str_buf);
                if (response->SerializeToArray(
                         str_buf + sizeof(RpcHeader) + header->meta_len,
                        response->ByteSize())) {
                    new_header->request_len = response->ByteSize();
                    Send(str.data(), str.size());
                } else {
                    LOG(WARNING, "Serialize response fail");
                }
                delete response;
            }
        }
    }
    int Send(const char* data, int len) {
        int w = 0;
        while (w < len) {
            int ret = write(fd_, data + w, len - w);
            if (ret <= 0) {
                fprintf(stderr, "Socket write failed\n");
                break;
            }
            w += ret;
        }
        return w;
    }
    void Recv(const char* buf, int len) {
        //LOG(INFO, "Recv %d bytes", len);
        recv_buf_.append(buf, len);
        if (recv_buf_.size() >= sizeof(RpcHeader)) {
            const RpcHeader* header = reinterpret_cast<const RpcHeader*>(recv_buf_.data());
            uint32_t packet_len = sizeof(RpcHeader) + header->meta_len + header->request_len;
            if (recv_buf_.size() >= packet_len) {
                /// callback
                ParseRequest(header);
                /// next
                recv_buf_ = recv_buf_.substr(packet_len);
            } else {
                //LOGS(INFO) << "packet_len =" << packet_len;
            }
        }
    }
    void Worker() {
        char buffer[10240];
        int len = 0;
        while((len = read(fd_, &buffer, sizeof(buffer))) > 0) {
            Recv(buffer, len);
        }
        close(fd_);
        fprintf(stderr, "connect closed\n");
        return;
    }
private:
    std::string recv_buf_;
    int fd_;
    std::map<std::string, google::protobuf::Service*> service_map_;
};

RpcServer::RpcServer() {
    RpcServer(RpcServerOptions());
}

RpcServer::RpcServer(const RpcServerOptions& options) {
}

void RpcServer::Loop() {
    struct sockaddr cliaddr;
    memset(&cliaddr, 0, sizeof(cliaddr));
    socklen_t addrlen = 0;
    long clifd = -1;
    std::set<pthread_t> tids;
    while((clifd = accept(listen_fd_, &cliaddr, &addrlen)) > 0) {
        printf("accept connection %ld\n", clifd);
        RpcConnection* connection = new RpcConnection(this, clifd);
        //connection->Worker();
        common::Thread* thread = new common::Thread();
        thread->Start(boost::bind(&RpcConnection::Worker, connection));
    }
}
bool RpcServer::Start(const std::string& bind_address) {
    int port = 0;
    size_t pos = bind_address.find(':');
    if (pos == std::string::npos) {
        return false;
    }
    std::string host = bind_address.substr(0, pos);
    std::string port_str = bind_address.substr(pos+1);
    if (sscanf(port_str.c_str(), "%d", &port) < 1) {
        return false;
    }

    int fd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);


    int flag=1;
    int len=sizeof(int); 
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, len);
    int ret = bind(fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (ret < 0) {
        fprintf(stderr, "bind to %d fail\n", port);
        return false;
    }
    ret = listen(fd, 1024);
    assert(ret == 0);
    printf("listen on %d\n", port);

    listen_fd_ = fd;
    loop_thread_.Start(boost::bind(&RpcServer::Loop, this));
    return true;
}

bool RpcServer::Stop() {
    return true;
}

RpcServer::~RpcServer() {
    MutexLock lock(&lock_);
    std::map<std::string, google::protobuf::Service*>::iterator it = service_map_.begin();
    for (; it != service_map_.end(); ++it) {
        delete it->second;
    }
    service_map_.clear();
}
bool RpcServer::RegisterService(google::protobuf::Service* service) {
    MutexLock lock(&lock_);
    std::string name = service->GetDescriptor()->full_name();
    service_map_[name] = service;
    return true;
}

ServiceMap RpcServer::GetServiceMap() {
    MutexLock lock(&lock_);
    return service_map_;
}
}
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
