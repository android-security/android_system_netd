/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _SOCK_DIAG_H
#define _SOCK_DIAG_H

#include <unistd.h>
#include <sys/socket.h>

#include <linux/netlink.h>
#include <linux/sock_diag.h>
#include <linux/inet_diag.h>

#include <functional>
#include <set>

#include "Permission.h"
#include "UidRanges.h"

struct inet_diag_msg;
class SockDiagTest;

class SockDiag {

  public:
    static const int kBufferSize = 4096;

    // Callback function that is called once for every socket in the dump. A return value of true
    // means destroy the socket.
    typedef std::function<bool(uint8_t proto, const inet_diag_msg *)> DumpCallback;

    struct DestroyRequest {
        nlmsghdr nlh;
        inet_diag_req_v2 req;
    } __attribute__((__packed__));

    SockDiag() : mSock(-1), mWriteSock(-1), mSocketsDestroyed(0) {}
    bool open();
    virtual ~SockDiag() { closeSocks(); }

    int sendDumpRequest(uint8_t proto, uint8_t family, uint32_t states);
    int sendDumpRequest(uint8_t proto, uint8_t family, const char *addrstr);
    int readDiagMsg(uint8_t proto, DumpCallback callback);
    int sockDestroy(uint8_t proto, const inet_diag_msg *);
    // Destroys all sockets on the given IPv4 or IPv6 address.
    int destroySockets(const char *addrstr);
    // Destroys all sockets for the given protocol and UID.
    int destroySockets(uint8_t proto, uid_t uid, bool excludeLoopback);
    // Destroys all "live" (CONNECTED, SYN_SENT, SYN_RECV) TCP sockets for the given UID ranges.
    int destroySockets(const UidRanges& uidRanges, const std::set<uid_t>& skipUids,
                       bool excludeLoopback);
    // Destroys all "live" (CONNECTED, SYN_SENT, SYN_RECV) TCP sockets that no longer have
    // the permissions required by the specified network.
    int destroySocketsLackingPermission(unsigned netId, Permission permission,
                                        bool excludeLoopback);

  private:
    friend class SockDiagTest;
    int mSock;
    int mWriteSock;
    int mSocketsDestroyed;
    int sendDumpRequest(uint8_t proto, uint8_t family, uint32_t states, iovec *iov, int iovcnt);
    int destroySockets(uint8_t proto, int family, const char *addrstr);
    int destroyLiveSockets(DumpCallback destroy, const char *what, iovec *iov, int iovcnt);
    bool hasSocks() { return mSock != -1 && mWriteSock != -1; }
    void closeSocks() { close(mSock); close(mWriteSock); mSock = mWriteSock = -1; }
    static bool isLoopbackSocket(const inet_diag_msg *msg);
};

#endif  // _SOCK_DIAG_H
