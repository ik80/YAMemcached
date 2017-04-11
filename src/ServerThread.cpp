/*
 * ServerThread.cpp
 *
 *  Created on: Jan 4, 2017
 *      Author: kalujny
 */

#include <algorithm>
#include <cstring>

#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef NDEBUG
#include <time.h>
#endif

#include "Common.hh"

#include "ServerThread.hh"
#include "Server.hh"
#include "YAMBinaryProtocolProcessor.h"


namespace YAMemcachedServer
{
    ServerThread::ServerThread() : epollFd(-1), udpFd(-1), shouldStop(false), myThreadId(0)
    {
        sessionPool.reset(new LFSPSCQueue<Session*>(MAX_CLIENTS_PER_THREAD));
        for (size_t i = 0; i < MAX_CLIENTS_PER_THREAD; ++i)
        {
            Session* pSession = new Session();
            sessionPool->push(pSession);
        }
    }

    int ServerThread::addSession(int clientSocket)
    {
        int res;
        if (epollFd == -1)
            return -1;

        makeSocketTcpNoDelay(clientSocket);
        makeSocketNonBlocking(clientSocket, true);

        struct epoll_event epevent;
        std::memset(&epevent, 0, sizeof(epevent));
        epevent.events = EPOLLIN | EPOLLET |  EPOLLOUT;
        Session* pSession = 0;
        if (!sessionPool->pop(pSession))
        {
            printf("Out of sessions!\n");
            return -1;
        }
        pSession->socket = clientSocket;
        pSession->requestInfo.reset();
        epevent.data.ptr = (void*) pSession;
        res = epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &epevent);
        if (res == -1)
        {
            perror("epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &epevent)");
        }
        return res;
    }

    void ServerThread::threadProc()
    {
#ifdef NDEBUG
        struct timespec timeNow;
        uint64_t nanosecondsWall;
#endif
        myThreadId = globalServer()->pHash->getMyThreadId();
        struct epoll_event *events = (struct epoll_event *) std::malloc(sizeof(*events) * MAX_EPOLL_EVENTS_PER_CALL);
        std::memset(events, 0, sizeof(*events) * MAX_EPOLL_EVENTS_PER_CALL);
        if(events == NULL)
        {
            perror("malloc(3) failed when attempting to allocate events buffer");
            abort();
        }

        bool udpIncompleteRead = false;
        mmsghdr * udpMessages = new mmsghdr[UDP_MAX_MSGS];
        iovec * iovecs = new iovec[UDP_MAX_MSGS];
        char ** udpBuffers;
        udpBuffers = new char*[UDP_MAX_MSGS];
        for (size_t i = 0; i < UDP_MAX_MSGS; ++i)
        {
            udpBuffers[i] = new char[UDP_BUF_SIZE];
            char * buf = &udpBuffers[i][0];
            iovec * iovec = &iovecs[i];
            mmsghdr * msg = &udpMessages[i];

            iovec->iov_base = buf;
            iovec->iov_len = UDP_BUF_SIZE;

            msg->msg_hdr.msg_iov = iovec;
            msg->msg_hdr.msg_iovlen = 1;

            msg->msg_hdr.msg_name = std::malloc(sizeof(sockaddr_in));
            msg->msg_hdr.msg_namelen = sizeof(sockaddr_in);
        }

        udpFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        int flag = 1;
        if(setsockopt(udpFd, SOL_SOCKET, SO_REUSEPORT, (char *) &flag, sizeof(int)) == -1)
        {
            perror("Cant set SO_REUSEPORT on this threads udp socket");
            abort();
        }
        if (makeSocketNonBlocking(udpFd, true) == -1)
        {
            perror("Cant make udp socket non blocking");
            abort();
        }
        sockaddr_in si_server;
        memset((char *) &si_server, 0, sizeof(si_server));
        si_server.sin_family = AF_INET;
        si_server.sin_port = htons(globalServer()->port);
        si_server.sin_addr.s_addr = htonl(INADDR_ANY);
        if( bind(udpFd , (struct sockaddr*)&si_server, sizeof(si_server) ) == -1)
        {
            perror("Cant bind this threads UDP socket");
            abort();
        }
        struct epoll_event udpepevent;
        std::memset(&udpepevent, 0, sizeof(udpepevent));
        udpepevent.events = EPOLLIN | EPOLLET |  EPOLLOUT;
        udpepevent.data.ptr = 0; // NULL in data.ptr specifies udp socket for this thread
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, udpFd, &udpepevent) == -1)
        {
            perror("Cant add udp socket to this threads epoll");
            abort();
        }

#ifdef NDEBUG
        size_t epollCallCount = 0;
        size_t epollCallCountWithZeroEvents = 0;
        Session::SessionState lastState = Session::READY_STATE;
#endif
        while(!shouldStop)
        {
            auto pSession = std::begin(sessionBacklog);
            while (pSession != std::end(sessionBacklog))
            {
#ifdef NDEBUG
                printf("Next postponed session!");
#endif
                Session::SessionResult res = (*pSession)->process((*pSession)->postponedEpollFlags, myThreadId); // anything ending up in backlog should restart with read
                switch(res)
                {
                    case Session::FAILURE:
                    {
                        perror("Closing socket\n");
                        close((*pSession)->socket); // should be removed from epoll set automatically
                        (*pSession)->reset();
                        sessionPool->push((*pSession));
                        pSession = sessionBacklog.erase(pSession);
                    }
                    break;
                    case Session::SUCCESS:
                    {
#ifdef NDEBUG
                        printf("Postponed session cleared!!\n");
#endif
                        pSession = sessionBacklog.erase(pSession);
                    }
                    break;
                    case Session::POSTPONED:
                    default:
                    {
#ifdef NDEBUG
                        printf("Postponed session not done!!\n");
#endif
                        ++pSession;
                    }
                    break;
                }
            }
#ifdef NDEBUG
            ++epollCallCount;
            clock_gettime(CLOCK_MONOTONIC, &timeNow);
            nanosecondsWall = 1000000000ULL * timeNow.tv_sec + timeNow.tv_nsec;
            printf("EpolSta %d %llu nanoseconds\n", ((int) lastState), (long long unsigned int) nanosecondsWall);
#endif
            // if we have postponed sessions, wait at most systick
            int eventsCnt = epoll_wait(epollFd, events, MAX_EPOLL_EVENTS_PER_CALL, (sessionBacklog.empty() && !udpIncompleteRead) ? EPOLL_PERIOD_MILLISECONDS : SYSTICK_IN_MS );
#ifdef NDEBUG
            clock_gettime(CLOCK_MONOTONIC, &timeNow);
            nanosecondsWall = 1000000000ULL * timeNow.tv_sec + timeNow.tv_nsec;
            printf("EpolE %d %d %llu nanoseconds\n", eventsCnt, events[0].events,  (long long unsigned int) nanosecondsWall);
            if (!eventsCnt)
                ++epollCallCountWithZeroEvents;
#endif

#ifdef NDEBUG
            if (eventsCnt == -1 && errno != EINTR)
#else
            if (eventsCnt == -1)
#endif
            {
                perror("epoll_wait(2) error");
                abort();
            }

            for(int i = 0; i < eventsCnt; i++)
            {
                if (events[i].data.ptr)
                {
#ifdef NDEBUG
                    printf("Next session\n");
#endif
                    Session* pSession = static_cast<Session*>(events[i].data.ptr);
                    Session::SessionResult res = pSession->process(events[i].events, myThreadId);
                    switch(res)
                    {
                        case Session::FAILURE:
                        {
                            perror("Closing socket\n");
                            close(pSession->socket); // should be removed from epoll set automatically
                            pSession->reset();
                            sessionPool->push(pSession);
                        }
                        break;
                        case Session::POSTPONED: // can only be returned from writes with pending reads
                        {
#ifdef NDEBUG
                            printf("Postponed session, pushing to backlog!!\n");
#endif
                            sessionBacklog.push_back(pSession);
                        }
                        break;
                        case Session::SUCCESS:
                        default:
                        break;
                    }
#ifdef NDEBUG
                    lastState = pSession->state;
#endif
                }
                else if (events[i].events & EPOLLIN)
                    udpIncompleteRead = true;
            }

            if (udpIncompleteRead)
            {
                int r = recvmmsg(udpFd, &udpMessages[0], UDP_MAX_MSGS, MSG_DONTWAIT, 0);
                if (r <= 0)
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
                        perror("Error in recvmmsg\n");
                    else
                    {
                        printf("epoll fired/incomplete read present but recvmmsg returns EAGAIN?!\n");
                    }
                }
                else
                {
                    udpIncompleteRead = (r == UDP_MAX_MSGS);

                    for (int i = 0; i < r; i++) {
                        mmsghdr *msg = &udpMessages[i];
                        char *buf =  (char *) msg->msg_hdr.msg_iov->iov_base;
                        size_t bufLen = msg->msg_hdr.msg_iov->iov_len;
                        size_t offset = 0;
                        size_t outLen = 0;

                        RequestInfo reqInfo;
                        processBinaryRequest(myThreadId,reqInfo, buf, offset, bufLen, buf, outLen);
                        msg->msg_hdr.msg_iov->iov_len = outLen;
                    }
                    sendmmsg(udpFd, &udpMessages[0], r, MSG_DONTWAIT);
                }
            }

#ifdef NDEBUG
            printf("Epoll cycle done.\n");
#endif
        }
        std::free(events);
        delete[] iovecs;
        for (size_t i = 0; i < UDP_MAX_MSGS; ++i)
        {
            mmsghdr * msg = &udpMessages[i];
            std::free(msg->msg_hdr.msg_name);
            delete[] udpBuffers[i];
        }
        delete[] udpBuffers;
        delete[] udpMessages;
    }

    void ServerThread::start()
    {
        if((epollFd = epoll_create(1)) < 0)
        {
            perror("epoll_create(2) failed");
            abort();
        }
        serverThread.reset(new std::thread(&ServerThread::threadProc, this));
    }

    void ServerThread::stop()
    {
        shouldStop = true;
        serverThread->join();
        printf("Worker thread stopped\n");
    }

} /* namespace YAMemcachedServer */
