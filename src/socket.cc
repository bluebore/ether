// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: yanshiguang02@baidu.com

#include "socket.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <string>
#include <vector>
#include <boost/bind.hpp>

namespace baidu {
namespace ether {

static inline bool SetNonBlock(int fd) {
    int arg = 0;
    if (0 > (arg=fcntl(fd, F_GETFL, NULL))) { 
        return false;
    } 
    arg |= O_NONBLOCK; 
    if (0 > fcntl(fd, F_SETFL, arg)) { 
        return false;
    } 
    return true;
}

/// ½âÎöip
bool ResolveIpv4(const std::string& site_name, std::vector<in_addr_t>* addrs) {
    addrs->clear();
    if (site_name.empty()) {
        return false;
    }
    addrinfo hints;
    addrinfo *res = NULL;
    addrinfo *pai = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    if (getaddrinfo(site_name.c_str(), NULL, &hints, &res) != 0
            || res == NULL) {
        return false;
    }
    for (pai = res; pai != NULL; pai = pai->ai_next) {
        if (pai->ai_family != AF_INET 
                || pai->ai_addrlen != sizeof(sockaddr_in)) {
            continue;
        }
        sockaddr_in* paddr = (sockaddr_in*)pai->ai_addr;
        addrs->push_back(paddr->sin_addr.s_addr);
    }
    freeaddrinfo(res);
    return !addrs->empty();
}


void Socket::StartConnect(const std::string& remote_addr,
                          boost::function<void (bool, const std::string&)> callback) {
    int port = 0;
    size_t pos = remote_addr.find(':');
    if (pos == std::string::npos) {
        callback(false, "Bad address 1 " + remote_addr);
        return;
    }
    std::string host = remote_addr.substr(0, pos);
    std::string port_str = remote_addr.substr(pos+1);
    if (sscanf(port_str.c_str(), "%d", &port) < 1) {
        callback(false, "Bad address 2 " + remote_addr);
        return;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;

    int tmp_int = 0;
    if (sscanf(host.c_str(), "%d.%d.%d.%d", &tmp_int, &tmp_int, &tmp_int ,&tmp_int) < 4) {
        std::vector <in_addr_t> addrs;
        if (!ResolveIpv4(host, &addrs)) {
            callback(false, "Bad address 3");
            return;
        }
        servaddr.sin_addr.s_addr = addrs[rand()%addrs.size()];
    } else {
        if (0 >= inet_pton(AF_INET, host.c_str(), &servaddr.sin_addr)) {
            callback(false, "Bad address 4");
            return;
        }
    }
    
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    servaddr.sin_port = htons(port);
    int ret = connect(fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (ret < 0) {
        callback(false, "connect to " + remote_addr + " fail: " + strerror(errno));
        return;
    }
    SetNonBlock(fd);

    socket_fd_ = fd;
    int epfd = epoll_create(1024);
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    thread_.Start(boost::bind(&Socket::Loop, this, epfd));
    callback(true, "OK");
}

void Socket::Loop(int epfd) {
    struct epoll_event ev;
    char buffer[10240] = {};
    while (1 == epoll_wait(epfd, &ev, 1, -1)) {
        if (ev.events & EPOLLERR) {
            fprintf(stderr, "EPOLLERR on %d\n", ev.data.fd);
            continue;
        }
        int rev = 0;
        if (ev.events & EPOLLIN) {
            while((rev = read(ev.data.fd, buffer, sizeof(buffer))) > 0) {
                consumer_(buffer, rev);
            }
            if (rev == 0) {
                fprintf(stderr, "Server close connection\n");
            }
            if (recv < 0) {
                fprintf(stderr, "socket read fail: %s\n", strerror(errno));
            }
        }
        if (ev.events & EPOLLOUT) {
            /*
            std::string msg;
            BuildRequest("RPC", "haha", &msg);
            while(write(ev.data.fd, buffer, sizeof(buffer)) > 0) {
            }*/
        }
    }
}
}
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
