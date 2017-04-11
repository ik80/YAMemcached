/*
 * Session.hh
 *
 *  Created on: Jan 4, 2017
 *      Author: kalujny
 */

#ifndef GENERIC_TCP_SERVER_SESSION_HH_
#define GENERIC_TCP_SERVER_SESSION_HH_

#include <vector>

#include "RequestProcessor.hh"

namespace YAMemcachedServer
{
    struct Session
    {
        enum SessionState
        {
            READY_STATE,
            READING_REQUEST,
            WRITING_REPLY,
            WRITING_WITH_PENDING_READ
        };

        int socket;
        SessionState state;

        size_t headerOffset;
        size_t headerLen;
        size_t headerBytesRequired;
        size_t dataOffset;
        size_t dataLen;
        size_t dataBytesRequired;

        size_t inBufferBytes;
        size_t inBufferOffset;
        size_t outBufferBytes;
        size_t outBufferOffset;

        std::vector<char> inBuffer;
        std::vector<char> outBuffer;

        RequestInfo requestInfo;
        uint32_t postponedEpollFlags;

        Session();
        ~Session() = default;
        Session(const Session& other) = default;
        Session(Session&& other) = default;
        Session& operator=(const Session& other) = default;
        Session& operator=(Session&& other) = default;

        enum SessionResult
        {
            SUCCESS,
            FAILURE,
            POSTPONED
        };

        SessionResult process(uint32_t events, size_t threadId);
        void reset();
        void clear();
    };
}

#endif /* GENERIC_TCP_SERVER_SESSION_HH_ */
