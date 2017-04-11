/*
 * Server.cpp
 *
 *  Created on: Jan 4, 2017
 *      Author: kalujny
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include "Common.hh"

#include "Server.hh"

// TODO: allocate sessions as continuous array when pushing them into pools. Later you can use this to copy whole array on the fly and calculate load on threads.
// TODO: this is so next incoming thread is used based on load and not simple round robin
namespace YAMemcachedServer
{
    Server::Server() : port(Server::DEFAULT_PORT), numThreads(Server::DEFAULT_NUM_THREADS), listenSocket(-1), nextThreadIdx(0), udpEnabled(false)
    {
        maxHashSize = 0;
        pHash = 0;
        pMatcher = new FastSubstringMatcher();

        // these have to be in correct order (see CacheRequestType enum)
        std::vector<std::string> memcachedCommands;
        memcachedCommands.push_back("get");
        memcachedCommands.push_back("set");
        memcachedCommands.push_back("add");
        memcachedCommands.push_back("delete");
        memcachedCommands.push_back("replace");
        memcachedCommands.push_back("cas");
        memcachedCommands.push_back("gets");
        memcachedCommands.push_back("append");
        memcachedCommands.push_back("prepend");
        memcachedCommands.push_back("incr");
        memcachedCommands.push_back("deccr");
        memcachedCommands.push_back("touch");
        memcachedCommands.push_back("watch");
        memcachedCommands.push_back("lru_crawler");
        memcachedCommands.push_back("stats");
        memcachedCommands.push_back("slabs");

        pMatcher->SetKeywords(memcachedCommands);
    }

    Server::~Server()
    {
        // TODO: UGLEY!!
        listenThread.release();
        delete pMatcher;
        delete pHash;
    }

    void Server::threadProc()
    {
        if((listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        {
            perror("socket(2) failed");
            abort();
        }

        int enable = 1;
        if(setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        {
            perror("setsockopt(SO_REUSEADDR) failed");
            abort();
        }

        struct sockaddr_in serveraddr;
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(port);
        serveraddr.sin_addr.s_addr = INADDR_ANY;

        if(bind(listenSocket, (const struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
        {
            perror("bind(2) failed");
            abort();
        }

        if(listen(listenSocket, MAX_ACCEPTOR_BACKLOG) < 0)
        {
            perror("listen(2) failed");
            abort();
        }

        while(true)
        {
            int clientsock;
            struct sockaddr_in addr;
            socklen_t addrlen = sizeof(addr);
            if((clientsock = accept(listenSocket, (struct sockaddr *) &addr, &addrlen)) < 0)
            {
                perror("accept");
                close(clientsock);
            }
            int threadId = (nextThreadIdx++) % numThreads;
            if (serverThreads[threadId].addSession(clientsock))
            {
                perror("addSession");
                close(clientsock);
                continue;
            }
        }
    }

    void Server::start()
    {
        size_t numCpus = sysconf(_SC_NPROCESSORS_ONLN);
        numThreads =  numCpus < numThreads ? numCpus : numThreads;

        serverThreads.resize(numThreads);
        for (auto& serverThread : serverThreads)
        {
            serverThread.start();
        }
        listenThread.reset(new std::thread(&Server::threadProc, this));
    }

    void Server::stop()
    {
        printf("Server stopping\n");
        for (auto& serverThread : serverThreads)
        {
            serverThread.stop();
            printf("Done with worker thread\n");
        }
        close(listenSocket);
        // TODO: graceful listen thread shutdown
        // listenThread->join();
        printf("To hell with listener thread\n");
    }

    Server* globalServer()
    {
        static Server* pServer = 0;
        if (pServer == 0)
            pServer = new Server();
        return pServer;
    }


} /* namespace YAMemcachedServer */
