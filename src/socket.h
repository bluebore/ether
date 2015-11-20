// Copyright (c) 2015, Baidu.com, Inc. All Rights Reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: yanshiguang02@baidu.com

#ifndef  BAIDU_ETHER_SOCKET_H_
#define  BAIDU_ETHER_SOCKET_H_

#include <string.h>

#include <boost/function.hpp>

#include <thread.h>

namespace baidu {
namespace ether {

class Socket {
public:
    void StartConnect(const std::string& host,
                      boost::function<void (bool, const std::string&)> callback);
    void OnConnect() {
    }
    void Write(const std::string& buffer, boost::function<void ()> callback) {
        const char* p = buffer.data();
        int len = buffer.size();
        int w = 0;
        while (w < len) {
            int ret = write(socket_fd_, p + w, len - w);
            if (ret <= 0) {
                fprintf(stderr, "Socket write failed\n");
                callback();
                return;
            }
            w += ret;
        }
        callback();
    }
    void SetConsumer(boost::function<void (const char* buf, int len)> consumer) {
        consumer_ = consumer;
    }
    void Loop(int epfd);
private:
    int socket_fd_;
    boost::function<void (const char* buf, int len)> consumer_;
    common::Thread thread_;
};

} // namespace ether
} // namespace baidu

#endif  //BAIDU_ETHER_SOCKET_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
