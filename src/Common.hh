/*
 * Common.hh
 *
 *  Created on: Jan 4, 2017
 *      Author: kalujny
 */

#ifndef GENERIC_TCP_SERVER_COMMON_HH_
#define GENERIC_TCP_SERVER_COMMON_HH_

#include <list>

#include <stddef.h>

#include <asm/param.h>

#include "LFLRUHashTable.h"
#include "LFLRUHashTableUtil.h"
#include "LFSPSCQueue.h"
#include "xxhash.h"
#include "string.hpp"
#include "FastSubstringMatcher.h"
#include "trbtree.hh"

namespace YAMemcachedServer
{
    static const size_t MIN_IN_BUFFER_SIZE = 4*1024;
    static const size_t MAX_IN_BUFFER_SIZE = 128*1024*1024;
    static const size_t MIN_OUT_BUFFER_SIZE = 4*1024;
    static const size_t MAX_OUT_BUFFER_SIZE = 128*1024*1024;
    static const size_t MAX_EPOLL_EVENTS_PER_CALL = 16*1024;
    static const size_t EPOLL_PERIOD_MILLISECONDS = 1*100;
    static const int    SYSTICK_IN_MS = HZ >= 1000 ? 1 : 1000 / HZ;
    static const size_t EPOLL_READ_THRESHOLD = 16*1024*1024; // maximum amount of data to read in one go, before breaking off to let thread process other connections
    static const int    MAX_CLIENTS_PER_THREAD = 4096;
    static const size_t MAX_ACCEPTOR_BACKLOG = 1024;
    static const size_t MAX_HASH_SIZE = 4 * 1024 * 1024 * 1024ULL - 3ULL;
    static const size_t UDP_BUF_SIZE = 65536;
    static const size_t UDP_MAX_MSGS = 512;


    struct string_hash : public std::unary_function<sso23::string, std::size_t>
    {
        std::size_t operator() (const sso23::string & k) const
        {
            return XXH64 ((const void*) k.c_str(), k.size(), 0);
        }
    };

    struct StorageValue
    {
        sso23::string value;
        uint64_t casTicket;
        uint32_t flags;
    } __attribute((aligned(1),packed));

    using LFStringMap = LFLRUHashTable<sso23::string, StorageValue, string_hash >;
    using ExpTimeMap = TRBTree<unsigned int, uint32_t>;

    struct SLFSPHTItemCounter
    {
        size_t * count;
        SLFSPHTItemCounter(size_t * in_pCount) : count(in_pCount)
        {
        }
        ~SLFSPHTItemCounter()
        {
        }
        inline void operator()(LFStringMap::LRUElement& toProcess)
        {
            ++(*count);
        }
    };

    static const int CACHE_REQUEST_FLAGS_NONE = 0;
    static const int NOREPLY = 1;
    static const int FREE_KEY = 2;
    static const int FREE_DATA = 4;

    struct RequestInfo
    {
        RequestInfo();
        ~RequestInfo();

        // ordered by use frequency!!
        enum CacheRequestType
        {
            GET = 0,
            SET,
            ADD,
            DELETE,
            REPLACE,
            CAS,
            GETS,
            APPEND,
            PREPEND,
            INCR,
            DECR,
            TOUCH,
            WATCH,
            LRU_CRAWLER,
            STATS,
            SLABS,
            STAT,
            UNSUPPORTED,
            TYPE_MAX
        };

        enum RequestParsingState
        {
            READY_STATE,
            PARSING_HEADER,
            PARSING_DATA,
            DONE_STATE
        };

        RequestParsingState state;

        CacheRequestType    requestType;
        int                 requestFlags;

        char*               pKeyBuffer;
        uint16_t            keyBufferLen;
        uint16_t            flags;
        uint32_t            expTime;
        uint64_t            casTicket;

        char*               pDataBuffer;
        size_t              dataLen;

        std::list< std::pair<char*, uint16_t> > * getKeysList;

        void reset();
    };

    int makeSocketNonBlocking(int socket, bool set = true);
    int makeSocketTcpNoDelay(int socket);
    uint16_t atous(const char* beg, const char* end);
    uint32_t atoui(const char* beg, const char* end);
    uint64_t atoull(const char* beg, const char* end);
    char *strnstr(const char *haystack, const char *needle, size_t len);

/*    void saveFunc(LFStringMap::LRUElement& element, char* buffer);
    void loadFunc(LFStringMap::LRUElement& element, char* buffer);
    size_t sizeFunc(LFStringMap::LRUElement& element);*/
    char* itoa(size_t value, char* str, size_t& n);

}

#endif /* GENERIC_TCP_SERVER_COMMON_HH_ */
