// Copyright (c) 2014, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: yanshiguang02@baidu.com

#include <set>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "src/proto/rpc.pb.h"

namespace baidu {
namespace ether {

static inline bool SetNonBlock(int fd)
{
    int arg;
    if (0 > (arg=fcntl(fd, F_GETFL, NULL))) { 
        return false;
    } 
    arg |= O_NONBLOCK; 
    if (0 > fcntl(fd, F_SETFL, arg)) { 
        return false;
    } 
    return true;
}


int SendRequest(int fd, const std::string& msg) {
    return 0;
}

int BuildRequest(const std::string& method, const std::string& msg, std::string* output) {
    RpcRequest request;
    request.set_msg(msg);
    std::string str;
    request.SerializeToString(&str);
    RpcMeta meta;
    meta.set_method(method);
    request.SerializeToString(output);
    output->append(str);
    return output->size();
}

int Run(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Use: %s host port\n", argv[0]);
        return 1;
    }
    const char* host = argv[1];
    int port = atoi(argv[2]);
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, host, &servaddr.sin_addr);
    servaddr.sin_port = htons(port);
    int ret = connect(fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (ret < 0) {
        fprintf(stderr, "connect to %s %d fail\n", host, port);
        return 2;
    }
    SetNonBlock(fd);

    int epfd = epoll_create(1024);
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

    int last_events;
    char buffer[10240] = {};
    while (1 == epoll_wait(epfd, &ev, 1, -1)) {
        //printf("epoll %d\n", ev.events);
        last_events = ev.events;
        if (ev.events & EPOLLERR) {
            fprintf(stderr, "EPOLLERR on %d\n", ev.data.fd);
            return 3;
        }
        int rev = 0;
        if (ev.events & EPOLLIN) {
            while((rev = read(ev.data.fd, buffer, sizeof(buffer))) > 0) {
            }
            if (rev == 0) {
                printf("Server close connection\n");
                return 4;
            }
        }
        if (ev.events & EPOLLOUT) {
            std::string msg;
            BuildRequest("RPC", "haha", &msg);
            while(write(ev.data.fd, buffer, sizeof(buffer)) > 0) {
            //while(write(ev.data.fd, msg.data(), msg.size()) > 0) {
                //    printf("write 4 bytes\n");
            }
        }
    }
    return 0;
}

}
}

int main(int argc, char* argv[]) {
    return baidu::ether::Run(argc, argv);
}
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
