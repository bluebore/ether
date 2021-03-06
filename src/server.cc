// Copyright (c) 2014, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: yanshiguang02@baidu.com

#include <set>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#include <counter.h>

namespace baidu {
namespace ether {

common::Counter g_rpcs[11];
bool quit = false;
bool send_response = false;

void* monitor(void* arg) {
    while(!quit) {
        usleep(1000000);
        long qps = 0;
        for (int i = 0; i < 11; i++) {
            qps += g_rpcs[i].Clear();
        }
        fprintf(stderr, "QPS: %ld\n", qps);
    }
    return NULL;
}

void* worker(void* arg) {
    int fd = reinterpret_cast<long>(arg);
    printf("fd = %d\n", fd);
    char buffer[10240];
    int len = 0;
    while((len = read(fd, &buffer, sizeof(buffer))) > 0) {
        if (send_response) {
            write(fd, &buffer, len);
        }
        g_rpcs[fd%11].Add(len/128);
    }
    close(fd);
    printf("connect closed\n");
    return NULL;
}

int Run(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Use: %s port\n", argv[0]);
        return 1;
    }
    if (argc == 3) {
        send_response = true;
    }

    signal(SIGPIPE, SIG_IGN);

    int port = atoi(argv[1]);
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    int ret = bind(fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (ret < 0) {
        fprintf(stderr, "bind to %d fail\n", port);
        return 2;
    }
    ret = listen(fd, 1024);
    assert(ret == 0);
    printf("listen on %d\n", port);

    pthread_t mtid;
    pthread_create(&mtid, NULL, monitor, NULL);

    struct sockaddr cliaddr;
    socklen_t addrlen;
    long clifd = -1;
    std::set<pthread_t> tids;
    while((clifd = accept(fd, &cliaddr, &addrlen)) > 0) {
        printf("accept connection %ld\n", clifd);
        pthread_t tid;
        pthread_create(&tid, NULL, worker, reinterpret_cast<void*>(clifd));
    }
    printf("clifd = %ld\n", clifd);
    quit = true;
    pthread_join(mtid, NULL);
    for (std::set<pthread_t>::iterator it=tids.begin(); it != tids.end(); ++it) {
        pthread_join(*it, NULL);
    }
    return 0;
}

}
}

int main(int argc, char* argv[]) {
    return baidu::ether::Run(argc, argv);
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
