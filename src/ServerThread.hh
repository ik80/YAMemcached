/*
 * ServerThread.h
 *
 *  Created on: Jan 4, 2017
 *      Author: kalujny
 */

#ifndef GENERIC_TCP_SERVER_SERVERTHREAD_H_
#define GENERIC_TCP_SERVER_SERVERTHREAD_H_

#include <vector>
#include <memory>
#include <thread>
#include <atomic>

#include "LFSPSCQueue.h"

#include "Session.hh"

namespace YAMemcachedServer
{
    using SessionPool = std::unique_ptr< LFSPSCQueue<Session*> >;
    using SessionBacklog = std::vector<Session*>;

    struct ServerThread
    {
        int                             epollFd;
        int                             udpFd;
        bool                            shouldStop;
        std::unique_ptr<std::thread>    serverThread;
        size_t                          myThreadId;

        SessionPool sessionPool;
        SessionBacklog sessionBacklog;

        void threadProc();
        void start();
        void stop();
        int addSession(int clientSocket); // returns 0 on success

        ServerThread();
        ~ServerThread() = default;

        ServerThread(const ServerThread& other) = default;
        ServerThread(ServerThread&& other) = default;
        ServerThread& operator=(const ServerThread& other) = default;
        ServerThread& operator=(ServerThread&& other) = default;
    };
}

#endif /* GENERIC_TCP_SERVER_SERVERTHREAD_H_ */
