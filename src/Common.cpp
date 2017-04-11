/*
 * Common.cpp
 *
 *  Created on: Jan 4, 2017
 *      Author: kalujny
 */

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "Common.hh"

namespace YAMemcachedServer
{
    int makeSocketNonBlocking(int socket, bool set)
    {
        int flags, status;
        flags = 0;
        flags = fcntl(socket, F_GETFL, 0);
        if(flags == -1)
            return -1;
        if(set)
            flags |= O_NONBLOCK;
        else
            flags &= ~O_NONBLOCK;
        status = fcntl(socket, F_SETFL, flags);
        if(status == -1)
            return -1;
        return 0;
    }

    int makeSocketTcpNoDelay(int socket)
    {
        int flag = 1;
        if(setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int)) == -1)
            return -1;
        return 0;
    }

    uint16_t atous(const char* beg, const char* end)
    {
        int value = 0;
        int len = end - beg; // assuming end is past beg;
        switch (len)
        // handle up to 5 digits, assume we're 16-bit
        {
            case 5:
            value += (beg[len - 5] - '0') * 10000;
            case 4:
            value += (beg[len - 4] - '0') * 1000;
            case 3:
            value += (beg[len - 3] - '0') * 100;
            case 2:
            value += (beg[len - 2] - '0') * 10;
            case 1:
            value += (beg[len - 1] - '0');
        }
        return value;
    }

    uint32_t atoui (const char* beg, const char* end)
    {
        int value = 0;
        int len = end - beg; // assuming end is past beg;
        switch (len)
        // handle up to 10 digits, assume we're 32-bit
        {
            case 10:
            value += (beg[len - 10] - '0') * 1000000000;
            case 9:
            value += (beg[len - 9] - '0') * 100000000;
            case 8:
            value += (beg[len - 8] - '0') * 10000000;
            case 7:
            value += (beg[len - 7] - '0') * 1000000;
            case 6:
            value += (beg[len - 6] - '0') * 100000;
            case 5:
            value += (beg[len - 5] - '0') * 10000;
            case 4:
            value += (beg[len - 4] - '0') * 1000;
            case 3:
            value += (beg[len - 3] - '0') * 100;
            case 2:
            value += (beg[len - 2] - '0') * 10;
            case 1:
            value += (beg[len - 1] - '0');
        }
        return value;
    }

    uint64_t atoull (const char* beg, const char* end)
    {
        uint64_t value = 0;
        int len = end - beg; // assuming end is past beg;
        switch (len)
        // handle up to 20 digits, assume we're 64-bit
        {
            case 20:
            value += (beg[len - 20] - '0') * 10000000000000000000ULL;
            case 19:
            value += (beg[len - 19] - '0') * 1000000000000000000ULL;
            case 18:
            value += (beg[len - 18] - '0') * 100000000000000000ULL;
            case 17:
            value += (beg[len - 17] - '0') * 10000000000000000ULL;
            case 16:
            value += (beg[len - 16] - '0') * 1000000000000000ULL;
            case 15:
            value += (beg[len - 15] - '0') * 100000000000000ULL;
            case 14:
            value += (beg[len - 14] - '0') * 10000000000000ULL;
            case 13:
            value += (beg[len - 13] - '0') * 1000000000000ULL;
            case 12:
            value += (beg[len - 12] - '0') * 100000000000ULL;
            case 11:
            value += (beg[len - 11] - '0') * 10000000000ULL;
            case 10:
            value += (beg[len - 10] - '0') * 1000000000ULL;
            case 9:
            value += (beg[len - 9] - '0') * 100000000ULL;
            case 8:
            value += (beg[len - 8] - '0') * 10000000ULL;
            case 7:
            value += (beg[len - 7] - '0') * 1000000ULL;
            case 6:
            value += (beg[len - 6] - '0') * 100000ULL;
            case 5:
            value += (beg[len - 5] - '0') * 10000ULL;
            case 4:
            value += (beg[len - 4] - '0') * 1000ULL;
            case 3:
            value += (beg[len - 3] - '0') * 100ULL;
            case 2:
            value += (beg[len - 2] - '0') * 10ULL;
            case 1:
            value += (beg[len - 1] - '0');
        }
        return value;
    }

    char *strnstr(const char *haystack, const char *needle, size_t len)
    {
        int i;
        size_t needle_len;

        /* segfault here if needle is not NULL terminated */
        if (0 == (needle_len = strlen(needle)))
                return (char *)haystack;

        for (i=0; i<=(int)(len-needle_len); i++)
        {
                if ((haystack[0] == needle[0]) &&
                        (0 == strncmp(haystack, needle, needle_len)))
                        return (char *)haystack;

                haystack++;
        }
        return NULL;
    }

/*    void saveFunc(LFStringMap::SparseBucketElement& element, char* buffer)
    {
        const size_t keySize = element.key.size();
        const size_t valueSize = element.value.value.size();
        *((size_t*) (buffer)) = keySize;
        buffer += sizeof(size_t);
        *((size_t*) (buffer)) = valueSize;
        buffer += sizeof(size_t);
        *((uint64_t*) (buffer)) = element.value.casTicket;
        buffer += sizeof(uint64_t);
        *((uint32_t*) buffer) = element.value.expTime;
        buffer += sizeof(uint32_t);
        *((uint16_t*) buffer) = element.value.flags;
        buffer += sizeof(uint16_t);
        memcpy(buffer, element.key.c_str(), keySize);
        buffer += keySize;
        memcpy(buffer, element.value.value.c_str(), valueSize);
    }

    void loadFunc(LFStringMap::SparseBucketElement& element, char* buffer)
    {
        const size_t keySize = *((size_t*) buffer);
        buffer += sizeof(size_t);
        const size_t valueSize = *((size_t*) buffer);
        buffer += sizeof(size_t);
        element.value.casTicket = *((uint64_t*) (buffer));
        buffer += sizeof(uint64_t);
        element.value.expTime = *((uint32_t*) buffer);
        buffer += sizeof(uint32_t);
        element.value.flags = *((uint16_t*) buffer);
        buffer += sizeof(uint16_t);
        element.key = sso23::string(buffer, keySize);
        buffer += keySize;
        element.value.value = sso23::string(buffer, valueSize);
    }

    size_t sizeFunc(LFStringMap::SparseBucketElement& element)
    {
        return sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint16_t) + element.value.value.size() + element.key.size();
    }*/

    char* itoa(size_t value, char* str, size_t& n)
    {
        n = 0;
        size_t radix = 10;
        static const char dig[] = "0123456789";
        size_t v = value;
        do
        {
            str[n++] = dig[v % radix];
            v /= radix;
        }
        while(v);
        str[n] = ' ';
        for (size_t i = 0; i < n/2; ++i)
        {
            char tmp = str[i];
            str[i] = str[(n - 1) - i];
            str[(n - 1) - i] = tmp;
        }
        return str;
    }

    RequestInfo::RequestInfo() :
    state(READY_STATE),
    requestType(TYPE_MAX),
    requestFlags(0),
    pKeyBuffer(0),
    keyBufferLen(0),
    flags(0),
    expTime(0),
    casTicket(0),
    pDataBuffer(0),
    dataLen(0),
    getKeysList(0)
    {
    }

    RequestInfo::~RequestInfo()
    {
        if (getKeysList)
            delete getKeysList;
    }
    void RequestInfo::reset()
    {
        if (getKeysList)
            delete getKeysList;
        state = READY_STATE;
        requestType = TYPE_MAX;
        requestFlags = 0;
        pKeyBuffer = 0;
        keyBufferLen = 0;
        flags = 0;
        expTime = 0;
        casTicket = 0;
        pDataBuffer = 0;
        dataLen = 0;
        getKeysList = 0;
    }


}

