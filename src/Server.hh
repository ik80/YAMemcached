/*
 * Server.hh
 *
 *  Created on: Jan 4, 2017
 *      Author: kalujny
 */

#ifndef GENERIC_TCP_SERVER_SERVER_HH_
#define GENERIC_TCP_SERVER_SERVER_HH_

#include <stddef.h>
#include <thread>

#include "Common.hh"
#include "ServerThread.hh"

namespace YAMemcachedServer
{
    using ServerThreads = std::vector<ServerThread>;
    using ListenThread = std::unique_ptr<std::thread>;

    struct Server
    {
        static const size_t DEFAULT_PORT = 11212;
        static const size_t DEFAULT_NUM_THREADS = 32;

        size_t          port;
        size_t          numThreads;
        int             listenSocket;
        size_t          nextThreadIdx;
        ServerThreads   serverThreads;
        ListenThread    listenThread;
        bool            udpEnabled;

        FastSubstringMatcher*   pMatcher;
        LFStringMap*            pHash;
        size_t                  maxHashSize;

        Server();
        ~Server();
        Server(const Server& other) = delete;
        Server(Server&& other) = delete;
        Server& operator=(const Server& other) = delete;
        Server& operator=(Server&& other) = delete;

        void threadProc();
        void start();
        void stop();
    };

    Server* globalServer();
}

#endif /* GENERIC_TCP_SERVER_SERVER_HH_ */
