/*
 * Session.cpp
 *
 *  Created on: Jan 4, 2017
 *      Author: kalujny
 */

#include <sys/epoll.h>
#include <sys/socket.h>
#include <string.h>

#include "Common.hh"
#include "Session.hh"
#include "RequestProcessor.hh"

#ifdef NDEBUG
#include <time.h>
#endif

namespace YAMemcachedServer
{

    Session::Session() :
                    socket(-1),
                    state(READY_STATE),
                    headerOffset(0),
                    headerLen(0),
                    headerBytesRequired(0),
                    dataOffset(0),
                    dataLen(0),
                    dataBytesRequired(0),
                    inBufferBytes(0),
                    inBufferOffset(0),
                    outBufferBytes(0),
                    outBufferOffset(0),
                    postponedEpollFlags(0)

    {
        memset(&requestInfo, 0, sizeof(RequestInfo));
        inBuffer.resize(MIN_IN_BUFFER_SIZE);
        outBuffer.resize(MIN_OUT_BUFFER_SIZE);
    }

    void Session::reset()
    {
        state = READING_REQUEST;
        headerOffset = 0;
        headerLen = 0;
        headerBytesRequired = 0;
        dataOffset = 0;
        dataLen = 0;
        dataBytesRequired = 0;
        inBufferBytes = 0;
        inBufferOffset = 0;
        outBufferBytes = 0;
        outBufferOffset = 0;
        postponedEpollFlags = 0;
        memset(&requestInfo, 0, sizeof(RequestInfo));
    }

    void Session::clear()
    {
#ifdef NDEBUG
        printf("Session::clear()!\n");
#endif
        socket = -1;
        state = READY_STATE;
        headerOffset = 0;
        headerLen = 0;
        headerBytesRequired = 0;
        dataOffset = 0;
        dataLen = 0;
        dataBytesRequired = 0;
        inBufferBytes = 0;
        inBufferOffset = 0;
        outBufferBytes = 0;
        outBufferOffset = 0;
        postponedEpollFlags = 0;
        memset(&requestInfo, 0, sizeof(RequestInfo));
        inBuffer.resize(MIN_IN_BUFFER_SIZE);
        inBuffer.shrink_to_fit();
        outBuffer.resize(MIN_OUT_BUFFER_SIZE);
        outBuffer.shrink_to_fit();
    }

    Session::SessionResult Session::process(uint32_t events, size_t threadId)
    {
        postponedEpollFlags = 0;
        Session::SessionResult res = FAILURE;
        if (events & EPOLLIN)
        {
            switch (state)
            {
                case READY_STATE:
                case READING_REQUEST:
                {
                    bool readAborted = false;
                    // first make sure we have enough space in inBuffer to accommodate incoming data
                    if ((dataBytesRequired + headerBytesRequired > 0) || (inBuffer.size() - inBufferOffset == 0))
                    {
                        while (dataBytesRequired + headerBytesRequired >= inBuffer.size() - inBufferOffset)
                        {
                            size_t keyOffset = 0;
                            size_t dataOffset = 0;
                            std::vector<size_t> additionalKeysOffsets;
                            if (requestInfo.pKeyBuffer)
                                keyOffset = requestInfo.pKeyBuffer - inBuffer.data();
                            if (requestInfo.pDataBuffer)
                                dataOffset = requestInfo.pDataBuffer - inBuffer.data();
                            if (requestInfo.getKeysList)
                            {
                                additionalKeysOffsets.resize(requestInfo.getKeysList->size());
                                size_t counter = 0;
                                for(auto itAdditionalKey : *(requestInfo.getKeysList))
                                {
                                    additionalKeysOffsets[counter++] = itAdditionalKey.first - inBuffer.data();
                                }
                            }
                            inBuffer.resize(2*(inBuffer.size()));
                            if (requestInfo.pKeyBuffer)
                                requestInfo.pKeyBuffer = inBuffer.data() + keyOffset;
                            if (requestInfo.pDataBuffer)
                                requestInfo.pDataBuffer = inBuffer.data() + dataOffset;
                            if (requestInfo.getKeysList)
                            {
                                size_t counter = 0;
                                for(auto& itAdditionalKey : *(requestInfo.getKeysList))
                                {
                                    itAdditionalKey.first = inBuffer.data() + additionalKeysOffsets[counter++];
                                }
                            }
                        }
                    }
                    // read everything client has to offer for now
                    ssize_t bytesRead = 0;
                    ssize_t bytesReadOnce = 0;
#ifdef NDEBUG
                        printf("Trying to read %lu bytes = (%lu size - %lu inBufferBytes)\n", inBuffer.size() - inBufferBytes, inBuffer.size(), inBufferBytes);
#endif
                    bytesReadOnce = recv(socket, inBuffer.data() + inBufferBytes, inBuffer.size() - inBufferBytes, 0);
                    while(bytesReadOnce > 0)
                    {
#ifdef NDEBUG
                        printf("bytesReadOnce %lu\n", bytesReadOnce);
#endif

                        inBufferBytes += bytesReadOnce;
                        bytesRead += bytesReadOnce;
                        bytesReadOnce = 0;

                        // this special case is when client sends requests non-stop, not giving server a chance to process some input
                        // do not abort if we are reading a huge piece of data
                        if ((size_t)bytesRead >= EPOLL_READ_THRESHOLD && (requestInfo.state == RequestInfo::DONE_STATE || requestInfo.state == RequestInfo::READY_STATE || requestInfo.state == RequestInfo::PARSING_HEADER))
                        {
                            readAborted = true;
#ifdef NDEBUG
                            printf("Read aborted! Tell the developer fast lol\n");
#endif
                            break;
                        }

                        if (inBufferBytes == inBuffer.size())
                        {
                            size_t keyOffset = 0;
                            size_t dataOffset = 0;
                            std::vector<size_t> additionalKeysOffsets;
                            if (requestInfo.pKeyBuffer)
                                keyOffset = requestInfo.pKeyBuffer - inBuffer.data();
                            if (requestInfo.pDataBuffer)
                                dataOffset = requestInfo.pDataBuffer - inBuffer.data();
                            if (requestInfo.getKeysList)
                            {
                                additionalKeysOffsets.resize(requestInfo.getKeysList->size());
                                size_t counter = 0;
                                for(auto itAdditionalKey : *(requestInfo.getKeysList))
                                {
                                    additionalKeysOffsets[counter++] = itAdditionalKey.first - inBuffer.data();
                                }
                            }
                            inBuffer.resize(2*inBuffer.size());
                            if (requestInfo.pKeyBuffer)
                                requestInfo.pKeyBuffer = inBuffer.data() + keyOffset;
                            if (requestInfo.pDataBuffer)
                                requestInfo.pDataBuffer = inBuffer.data() + dataOffset;
                            if (requestInfo.getKeysList)
                            {
                                size_t counter = 0;
                                for(auto& itAdditionalKey : *(requestInfo.getKeysList))
                                {
                                    itAdditionalKey.first = inBuffer.data() + additionalKeysOffsets[counter++];
                                }
                            }
                        }
#ifdef NDEBUG
                        printf("Trying to read %lu bytes = (%lu size - %lu inBufferBytes)\n", inBuffer.size() - inBufferBytes, inBuffer.size(), inBufferBytes);
#endif
                        bytesReadOnce = recv(socket, inBuffer.data() + inBufferBytes, inBuffer.size() - inBufferBytes, 0);
                    }

                    if(bytesReadOnce == -1 && errno != EWOULDBLOCK)
                    {
                        printf("recv failed %ld, %d, %lu, %lu, %lu\n", bytesReadOnce, errno, inBufferBytes, inBufferOffset, inBuffer.size());
                        perror("recv failed"); // recv with size 0 means the client is done OR some kind of error
                        res = FAILURE;
                        break;
                    }

                    if(bytesReadOnce == 0 && inBufferBytes == 0)
                    {
                        printf("recv failed %ld, %d, %lu, %lu, %lu\n", bytesReadOnce, errno, inBufferBytes, inBufferOffset, inBuffer.size());
                        perror("read of size 0"); // recv with size 0 means the client is done OR some kind of error
                        res = FAILURE;
                        break;
                    }

#ifdef NDEBUG
                    printf("bytesRead %lu\n", bytesRead);
#endif

                    //do the processing loop
                    int elementsProcessed = 0;
                    while (inBufferOffset < inBufferBytes && processRequest(threadId, requestInfo, inBuffer, headerOffset, headerLen, headerBytesRequired, dataOffset, dataLen, dataBytesRequired, inBufferOffset, inBufferBytes, outBuffer, outBufferBytes))
                        ++elementsProcessed;

#ifdef NDEBUG
                    struct timespec timeNow;
                    uint64_t nanosecondsWall;
                    clock_gettime(CLOCK_MONOTONIC, &timeNow);
                    nanosecondsWall = 1000000000ULL * timeNow.tv_sec + timeNow.tv_nsec;
                    printf("Process %d %llu nanoseconds\n", elementsProcessed, (long long unsigned int) nanosecondsWall);
                    if (!elementsProcessed)
                    {
                        char tmp[256];
                        snprintf(tmp, 256, "%s", inBuffer.data());
                        printf("inBuffer \"%s\" %lu %lu\n", tmp, inBufferOffset, inBufferBytes);
                    }
#endif

                    if (elementsProcessed)
                    {
                        //move leftovers to the begin of the inBuffer if needed
                        if (inBufferOffset < inBufferBytes && requestInfo.state != RequestInfo::DONE_STATE)
                        {
#ifdef NDEBUG
                            printf("Leftovers! %lu, %lu, ", inBufferOffset, inBufferBytes);
#endif
                            size_t leftoversLen = inBufferBytes - inBufferOffset;
                            memmove(inBuffer.data(), inBuffer.data() + inBufferOffset, leftoversLen); // +1 ??
                            if (requestInfo.pKeyBuffer)
                                requestInfo.pKeyBuffer -= inBufferOffset;
                            if (requestInfo.pDataBuffer)
                                requestInfo.pDataBuffer -= inBufferOffset;
                            if (requestInfo.getKeysList)
                            {
                                for(auto itAdditionalKey : *(requestInfo.getKeysList))
                                {
                                    itAdditionalKey.first -= inBufferOffset;
                                }
                            }
                            headerOffset = 0;
                            headerLen = 0;
                            headerBytesRequired = 0;
                            dataOffset = 0;
                            dataLen = 0;
                            dataBytesRequired = 0;
                            inBufferOffset = 0;
                            requestInfo.state = RequestInfo::READY_STATE;
                            requestInfo.reset();
                            inBufferBytes = leftoversLen;
#ifdef NDEBUG
                            printf("-> %lu, %lu, %lu, %lu\n", inBufferOffset, inBufferBytes, headerOffset, dataOffset);
#endif
                        }
                        else if (inBufferOffset == inBufferBytes && (requestInfo.state == RequestInfo::DONE_STATE || requestInfo.state == RequestInfo::READY_STATE))
                        {
                            headerOffset = 0;
                            headerLen = 0;
                            headerBytesRequired = 0;
                            dataOffset = 0;
                            dataLen = 0;
                            dataBytesRequired = 0;
                            inBufferBytes = 0;
                            inBufferOffset = 0;
                            requestInfo.state = RequestInfo::READY_STATE;
                            requestInfo.reset();
#ifdef NDEBUG
                            memset(inBuffer.data(), 0, inBuffer.size());
#endif
                        }
                        else
                        {
                            perror("Incorrect session state");
                            abort();
                        }

                        if (outBufferBytes - outBufferOffset < 0)
                        {
                            perror("Incorrect session state");
                            abort();
                        }
                        else if (outBufferBytes - outBufferOffset == 0) // nothing to send out
                        {
                            outBufferBytes = 0;
                            outBufferOffset = 0;
                            state = READING_REQUEST;
                            res = SUCCESS; // sent out everything
                            break;
                        }

                        ssize_t bytesSent = 0;
                        state = readAborted ? WRITING_WITH_PENDING_READ : WRITING_REPLY;
#ifdef NDEBUG
                        printf("Trying to write %lu bytes\n%s", outBufferBytes - outBufferOffset, outBuffer.data());
#endif
                        while (outBufferBytes - outBufferOffset > 0 && ((bytesSent = send(socket, outBuffer.data() + outBufferOffset, outBufferBytes - outBufferOffset, 0)) > 0))
                            outBufferOffset += bytesSent;

                        if (bytesSent == 0 || (bytesSent == -1 && errno == ECONNRESET))
                        {
                            perror("send of size 0"); // recv with size 0 means the client is done OR some kind of error
                            res = FAILURE;
                            break;
                        }
                        else if (bytesSent == -1 && errno == EWOULDBLOCK)
                        {
                            res = SUCCESS;
                            break;
                        }
                        else if (bytesSent >= 0 && outBufferBytes == outBufferOffset)
                        {
                            outBufferBytes = 0;
                            outBufferOffset = 0;
                            res = state == WRITING_REPLY ? SUCCESS : POSTPONED; // sent out everything
                            state = READING_REQUEST;
                            break;
                        }
                        else if (bytesSent == -1)
                        {
                            res = FAILURE; // send resulted in error other than EWOULDBLOCK
                            break;
                        }
                    }
                    else
                        state = READING_REQUEST;

                    res = SUCCESS;
                    break;
                }
                break;
                case WRITING_WITH_PENDING_READ:
                case WRITING_REPLY:
                {
                    state = WRITING_WITH_PENDING_READ;
                    res = SUCCESS;
                    break;
                }
                break;
                default:
                {
                    printf("Incorrect session state!\n");
                    res = FAILURE;
                    break;
                }
                break;
            }
            if (res == FAILURE)
                return res;
#ifdef NDEBUG
            printf("EPOLLIN done, state = %lu, res = %lu, inBufferBytes = %lu\n", (size_t)state, (size_t)res, inBufferBytes);
#endif
        }
        if (events & EPOLLOUT)
        {
            switch (state)
            {
                case WRITING_REPLY:
                {
                    ssize_t bytesSent = 0;
                    state = WRITING_REPLY;
                    while (outBufferBytes - outBufferOffset > 0 && ((bytesSent = send(socket, outBuffer.data() + outBufferOffset, outBufferBytes - outBufferOffset, 0)) > 0))
                        outBufferOffset += bytesSent;

                    if (bytesSent == 0 || (bytesSent == -1 && errno == ECONNRESET))
                    {
                        perror("send of size 0"); // recv with size 0 means the client is done OR some kind of error
                        res = FAILURE;
                        break;
                    }
                    else if (bytesSent == -1 && errno == EWOULDBLOCK)
                    {
                        res =  SUCCESS;
                        break;
                    }
                    else if (bytesSent >= 0 && outBufferBytes == outBufferOffset)
                    {
                        outBufferBytes = 0;
                        outBufferOffset = 0;
                        state = READING_REQUEST;
                        res =  SUCCESS; // sent out everything
                        break;
                    }
                    else if (bytesSent == -1)
                    {
                        res =  FAILURE; // send resulted in error other than EWOULDBLOCK
                        break;
                    }
                }
                break;
                case WRITING_WITH_PENDING_READ:
                {
                    ssize_t bytesSent = 0;
                    state = WRITING_REPLY;
                    while (outBufferBytes - outBufferOffset > 0 && ((bytesSent = send(socket, outBuffer.data() + outBufferOffset, outBufferBytes - outBufferOffset, 0)) > 0))
                        outBufferOffset += bytesSent;

                    if (bytesSent == 0 || (bytesSent == -1 && errno == ECONNRESET))
                    {
                        perror("send of size 0"); // recv with size 0 means the client is done OR some kind of error
                        res = FAILURE;
                        break;
                    }
                    else if (bytesSent == -1 && errno == EWOULDBLOCK)
                    {
                        res = SUCCESS;
                        break;
                    }
                    else if (bytesSent >= 0 && outBufferBytes == outBufferOffset)
                    {
                        outBufferBytes = 0;
                        outBufferOffset = 0;
                        state = READING_REQUEST;
                        res =  POSTPONED;
                        break;
                    }
                    else if (bytesSent == -1)
                    {
                        res =  FAILURE; // send resulted in error other than EWOULDBLOCK
                        break;
                    }
                }
                break;
                case READING_REQUEST: // we get this just after client is connected
                case READY_STATE:
                {
                    if (res != POSTPONED)
                    {
                         state = READING_REQUEST;
                         res =  SUCCESS;
                    }
                    break;
                }
                break;
                default:
                {
                    printf("Incorrect session state!\n");
                    res = FAILURE;
                    break;
                }
                break;
            }
            if(res == POSTPONED)
                postponedEpollFlags |= EPOLLIN;
        }
        if (!(events & EPOLLIN) && !(events & EPOLLOUT))
        {
            printf("Incorrect epoll event flags! events == %d\n", events);
            res = FAILURE;
        }
#ifdef NDEBUG
            printf("EPOLLOUT done, state = %lu, res = %lu\n", (size_t)state, (size_t)res);
#endif
        return res;
    }
} /* namespace YAMemcachedServer */
