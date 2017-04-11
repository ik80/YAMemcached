/*
 * Request.hh
 *
 *  Created on: Jan 4, 2017
 *      Author: kalujny
 */

#ifndef GENERIC_TCP_SERVER_REQUEST_HH_
#define GENERIC_TCP_SERVER_REQUEST_HH_

#include <memory>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <list>

#include "Common.hh"

namespace YAMemcachedServer
{
    // accepts header start, parses up to maxCharsToParse, updates pEnd and requestInfo, returns true on successful parse false otherwise
    inline bool parseMemcachedHeader(char* pBeg, char*& pEnd, RequestInfo& requestInfo, size_t maxCharsToParse);

    // accepts full spec for inBuffer related session params, which it can advance as needed, except size and amount of bytes in inBuffer.
    // can grow outBuffer as required
    // returns true on success / false when the data is not enough
    bool processRequest(size_t threadId, RequestInfo& requestInfo, std::vector<char> & inBuffer, size_t& headerOffset, size_t& headerLen, size_t& headerBytesRequired, size_t& dataOffset, size_t& dataLen, size_t& dataBytesRequired, size_t& inBufferOffset, const size_t& inBufferBytes, std::vector<char> & outBuffer, size_t& outBufferBytes);
}

#endif /* REQUEST_HH_ */
