/*
 * Request.cpp
 *
 *  Created on: Jan 4, 2017
 *      Author: kalujny
 */

#include <string.h>

#include "RequestProcessor.hh"
#include "Server.hh"

namespace YAMemcachedServer
{
    // accepts full specs for buffers related session params, can advance inBuffer as needed, except size and amount of bytes in it; can grow outBuffer as required
    // returns true on success / false when the data is not enough
    bool processRequest(size_t threadId, RequestInfo& requestInfo, std::vector<char> & inBuffer, size_t& headerOffset, size_t& headerLen, size_t& headerBytesRequired, size_t& dataOffset, size_t& dataLen, size_t& dataBytesRequired, size_t& inBufferOffset, const size_t& inBufferBytes, std::vector<char> & outBuffer, size_t& outBufferBytes)
    {
        //memcached_parser daParser;
        StorageValue sValue;
        static const size_t SOME_TEXT_LEN = 128;

        while(true)
        {
            switch (requestInfo.state)
            {
                case RequestInfo::READY_STATE:
                {
                    headerBytesRequired = 32;
                    requestInfo.state = RequestInfo::PARSING_HEADER;
                }
                break;
                case RequestInfo::PARSING_HEADER:
                {
                    if (inBufferOffset < inBufferBytes)
                    {
                        char * pEnd = inBuffer.data() + inBufferOffset;
                        if (parseMemcachedHeader(inBuffer.data() + inBufferOffset, pEnd, requestInfo, inBufferBytes - inBufferOffset))
                        {
                            headerBytesRequired = 0;
                            headerOffset = inBufferOffset;
                            headerLen = pEnd - (inBuffer.data() + inBufferOffset);
                            dataOffset = inBufferOffset + headerLen;
                            if (requestInfo.dataLen)
                            {
                                dataBytesRequired = requestInfo.dataLen + 2;
                                dataLen = requestInfo.dataLen + 2;
                            }
                            else
                            {
                                dataBytesRequired = 0;
                                dataLen = 0;
                            }
                            requestInfo.state = RequestInfo::PARSING_DATA;
                        }
                        else
                            return false;

                    }
                    else
                        return false;
                }
                break;
                case RequestInfo::PARSING_DATA:
                {
                    if (inBufferBytes - inBufferOffset - headerLen >= dataLen)
                    {
                        if (dataLen && (inBuffer.data()[dataOffset + dataLen - 2] != '\r' || inBuffer.data()[dataOffset + dataLen - 1] != '\n'))
                            abort();

                        dataBytesRequired = 0;
                        inBufferOffset += dataLen + headerLen;

                        switch (requestInfo.requestType)
                        {
                            case RequestInfo::SET:
                            {
                                sso23::string key = sso23::string(requestInfo.pKeyBuffer, requestInfo.keyBufferLen);
                                sValue.value = sso23::string(requestInfo.pDataBuffer, requestInfo.dataLen);
                                sValue.casTicket = requestInfo.casTicket;
                                sValue.flags = requestInfo.flags;
                                globalServer()->pHash->insertOrSet(threadId, key, sValue, requestInfo.expTime);
                                if((requestInfo.requestFlags & NOREPLY) == 0)
                                {
                                    const char reply[] = "STORED\r\n";
                                    while (outBuffer.size() - outBufferBytes < sizeof(reply))
                                        outBuffer.resize(2*outBuffer.size());
                                    strncpy(outBuffer.data() + outBufferBytes, reply, sizeof(reply) - 1);
                                    outBufferBytes += sizeof(reply) - 1;
                                }
#ifdef NDEBUG
                                printf("SET key: %s, value: %s, cas: %lu, flags: %lu, requestFlags: %lu\n", key.c_str(), sValue.value.c_str(), sValue.casTicket, sValue.flags, requestInfo.requestFlags);
#endif
                            }
                            break;
                            case RequestInfo::ADD:
                            {
                                sso23::string key = sso23::string(requestInfo.pKeyBuffer, requestInfo.keyBufferLen);
                                sValue.value = sso23::string(requestInfo.pDataBuffer, requestInfo.dataLen);
                                sValue.casTicket = requestInfo.casTicket;
                                sValue.flags = requestInfo.flags;
                                bool inserted = globalServer()->pHash->insert(threadId, key, sValue, requestInfo.expTime);
                                if((requestInfo.requestFlags & NOREPLY) == 0)
                                {
                                    const char replyStored[] = "STORED\r\n";
                                    const char replyNotstored[] = "EXISTS\r\n";
                                    if (inserted)
                                    {
                                        while (outBuffer.size() - outBufferBytes < sizeof(replyStored))
                                            outBuffer.resize(2*outBuffer.size());
                                        strncpy(outBuffer.data() + outBufferBytes, replyStored, sizeof(replyStored) - 1);
                                        outBufferBytes += sizeof(replyStored) - 1;
                                    }
                                    else
                                    {
                                        while (outBuffer.size() - outBufferBytes < sizeof(replyNotstored))
                                            outBuffer.resize(2*outBuffer.size());
                                        strncpy(outBuffer.data() + outBufferBytes, replyNotstored, sizeof(replyNotstored) - 1);
                                        outBufferBytes += sizeof(replyNotstored) - 1;
                                    }
                                }
                                //printf("ADD key: %s, value: %s, cas: %lu, flags: %lu, expTime: %lu, requestFlags: %lu\n", key.c_str(), data.c_str(), casTicket, flags, expTime, requestFlags);
                            }
                            break;
                            case RequestInfo::REPLACE:
                            {
                                sso23::string key = sso23::string(requestInfo.pKeyBuffer, requestInfo.keyBufferLen);
                                sValue.value = sso23::string(requestInfo.pDataBuffer, requestInfo.dataLen);
                                sValue.casTicket = requestInfo.casTicket;
                                sValue.flags = requestInfo.flags;
                                bool setResult = globalServer()->pHash->set(threadId, key, sValue, requestInfo.expTime);
                                if((requestInfo.requestFlags & NOREPLY) == 0)
                                {
                                    const char replyStored[] = "STORED\r\n";
                                    const char replyNotstored[] = "NOT_STORED\r\n";
                                    if (setResult)
                                    {
                                        while (outBuffer.size() - outBufferBytes < sizeof(replyStored))
                                            outBuffer.resize(2*outBuffer.size());
                                        strncpy(outBuffer.data() + outBufferBytes, replyStored, sizeof(replyStored) - 1);
                                        outBufferBytes += sizeof(replyStored) - 1;
                                    }
                                    else
                                    {
                                        while (outBuffer.size() - outBufferBytes < sizeof(replyNotstored))
                                            outBuffer.resize(2*outBuffer.size());
                                        strncpy(outBuffer.data() + outBufferBytes, replyNotstored, sizeof(replyNotstored) - 1);
                                        outBufferBytes += sizeof(replyNotstored) - 1;
                                    }
                                }
                                //printf("REPLACE key: %s, value: %s, cas: %lu, flags: %lu, expTime: %lu, requestFlags: %lu\n", key.c_str(), data.c_str(), casTicket, flags, expTime, requestFlags);
                            }
                            break;
                            case RequestInfo::APPEND:
                            {
                            }
                            break;
                            case RequestInfo::PREPEND:
                            {
                            }
                            break;
                            case RequestInfo::CAS:
                            {
                                StorageValue replacedValue;
                                sso23::string key = sso23::string(requestInfo.pKeyBuffer, requestInfo.keyBufferLen);
                                sValue.value = sso23::string(requestInfo.pDataBuffer, requestInfo.dataLen);
                                sValue.casTicket = requestInfo.casTicket;
                                sValue.flags = requestInfo.flags;
                                unsigned int casLocks[LFStringMap::HASH_MAX_LOCK_DEPTH];
                                unsigned long long locksStart = 0, locksEnd = 0;
                                bool result = globalServer()->pHash->get(threadId, key, replacedValue, casLocks, locksStart, locksEnd);
                                if (result)
                                {
                                    if(sValue.casTicket == replacedValue.casTicket)
                                    {
                                        ++sValue.casTicket;
                                        globalServer()->pHash->set(threadId, key, sValue, requestInfo.expTime, casLocks, locksStart, locksEnd);
                                        if((requestInfo.requestFlags & NOREPLY) == 0)
                                        {
                                            const char replyStored[] = "STORED\r\n";
                                            while (outBuffer.size() - outBufferBytes < sizeof(replyStored))
                                                outBuffer.resize(2*outBuffer.size());
                                            strncpy(outBuffer.data() + outBufferBytes, replyStored, sizeof(replyStored) - 1);
                                            outBufferBytes += sizeof(replyStored) - 1;
                                        }
                                    }
                                    else
                                    {
                                        globalServer()->pHash->unlockElement(threadId, key, casLocks, locksStart, locksEnd);
                                        if((requestInfo.requestFlags & NOREPLY) == 0)
                                        {
                                            const char replyNotstored[] = "NOT_STORED\r\n";
                                            while (outBuffer.size() - outBufferBytes < sizeof(replyNotstored))
                                                outBuffer.resize(2*outBuffer.size());
                                            strncpy(outBuffer.data() + outBufferBytes, replyNotstored, sizeof(replyNotstored) - 1);
                                            outBufferBytes += sizeof(replyNotstored) - 1;
                                        }
                                    }
                                }
                                else
                                {
                                    const char replyNotstored[] = "NOT_STORED\r\n";
                                    if((requestInfo.requestFlags & NOREPLY) == 0)
                                    {
                                        while (outBuffer.size() - outBufferBytes < sizeof(replyNotstored))
                                            outBuffer.resize(2*outBuffer.size());
                                        strncpy(outBuffer.data() + outBufferBytes, replyNotstored, sizeof(replyNotstored) - 1);
                                        outBufferBytes += sizeof(replyNotstored) - 1;
                                    }
                                }
                            }
                            break;
                            case RequestInfo::GET:
                            case RequestInfo::GETS:
                            {
                                sso23::string key = sso23::string(requestInfo.pKeyBuffer, requestInfo.keyBufferLen);
                                bool found = globalServer()->pHash->get(threadId, key, sValue);
                                if(found)
                                {
                                    while (outBuffer.size() - outBufferBytes < key.size() + sValue.value.size() + SOME_TEXT_LEN)
                                        outBuffer.resize(2*outBuffer.size());
                                    const char reply[] = "VALUE ";
                                    strncpy(outBuffer.data() + outBufferBytes, reply, sizeof(reply) - 1);
                                    outBufferBytes += sizeof(reply) - 1;
                                    strncpy(outBuffer.data() + outBufferBytes, key.c_str(), key.size());
                                    outBufferBytes += key.size();
                                    *(outBuffer.data() + outBufferBytes++) = ' ';
                                    size_t itoaLen = 0;
                                    itoa((size_t) sValue.flags, outBuffer.data() + outBufferBytes, itoaLen);
                                    outBufferBytes += itoaLen + 1;
                                    itoa((size_t) sValue.value.size(), outBuffer.data() + outBufferBytes, itoaLen);
                                    outBufferBytes += itoaLen; // no +1 - skip space at the end
                                    *(outBuffer.data() + outBufferBytes) = '\r';
                                    *(outBuffer.data() + outBufferBytes+1) = '\n';
                                    outBufferBytes += 2;
                                    strncpy(outBuffer.data() + outBufferBytes, sValue.value.c_str(), sValue.value.size());
                                    outBufferBytes += sValue.value.size();
                                    *(outBuffer.data() + outBufferBytes) = '\r';
                                    *(outBuffer.data() + outBufferBytes+1) = '\n';
                                    outBufferBytes += 2;
                                }
#ifdef NDEBUG
                                printf("GET key: %s", key.c_str());
                                if(!found)
                                    printf(", NOT_FOUND\n");
                                else
                                    printf(", %s\n", sValue.value.c_str());
#endif
                                if(requestInfo.getKeysList)
                                {
                                    for(auto itAdditionalKey : *(requestInfo.getKeysList))
                                    {
                                        sso23::string nextKey = sso23::string(itAdditionalKey.first, itAdditionalKey.second);
                                        bool found = globalServer()->pHash->get(threadId, nextKey, sValue);
                                        if(found)
                                        {
                                            while (outBuffer.size() - outBufferBytes < nextKey.size() + sValue.value.size() + SOME_TEXT_LEN)
                                                outBuffer.resize(2*outBuffer.size());
                                            const char reply[] = "VALUE ";
                                            strncpy(outBuffer.data() + outBufferBytes, reply, sizeof(reply) - 1);
                                            outBufferBytes += sizeof(reply) - 1;
                                            strncpy(outBuffer.data() + outBufferBytes, nextKey.c_str(), nextKey.size());
                                            outBufferBytes += nextKey.size();
                                            *(outBuffer.data() + outBufferBytes++) = ' ';
                                            size_t itoaLen = 0;
                                            itoa((size_t) sValue.flags, outBuffer.data() + outBufferBytes, itoaLen);
                                            outBufferBytes += itoaLen + 1;
                                            itoa((size_t) sValue.value.size(), outBuffer.data() + outBufferBytes, itoaLen);
                                            outBufferBytes += itoaLen; // no +1 - skip space at the end
                                            *(outBuffer.data() + outBufferBytes) = '\r';
                                            *(outBuffer.data() + outBufferBytes+1) = '\n';
                                            outBufferBytes += 2;
                                            strncpy(outBuffer.data() + outBufferBytes, sValue.value.c_str(), sValue.value.size());
                                            outBufferBytes += sValue.value.size();
                                            *(outBuffer.data() + outBufferBytes) = '\r';
                                            *(outBuffer.data() + outBufferBytes+1) = '\n';
                                            outBufferBytes += 2;
                                        }
                                    }
                                }
                                const char endReply[] = "END\r\n";
                                while (outBuffer.size() - outBufferBytes < sizeof(endReply))
                                    outBuffer.resize(2*outBuffer.size());
                                strncpy(outBuffer.data() + outBufferBytes, endReply, sizeof(endReply) - 1);
                                outBufferBytes += sizeof(endReply) - 1;
                            }
                            break;
                            case RequestInfo::DELETE:
                            {
                                sso23::string key = sso23::string(requestInfo.pKeyBuffer, requestInfo.keyBufferLen);
                                bool removeResult = globalServer()->pHash->remove(threadId, key);
                                if((requestInfo.requestFlags & NOREPLY) == 0)
                                {
                                    if(removeResult)
                                    {
                                        const char reply[] = "DELETED\r\n";
                                        while (outBuffer.size() - outBufferBytes < sizeof(reply))
                                            outBuffer.resize(2*outBuffer.size());
                                        strncpy(outBuffer.data() + outBufferBytes, reply, sizeof(reply) - 1);
                                        outBufferBytes += sizeof(reply) - 1;
                                    }
                                    else
                                    {
                                        const char reply[] = "NOT_FOUND\r\n";
                                        while (outBuffer.size() - outBufferBytes < sizeof(reply))
                                            outBuffer.resize(2*outBuffer.size());
                                        strncpy(outBuffer.data() + outBufferBytes, reply, sizeof(reply) - 1);
                                        outBufferBytes += sizeof(reply) - 1;
                                    }
                                }
                            }
                            break;
                            case RequestInfo::STATS:
                            {
                                size_t counter = 0;
                                SLFSPHTItemCounter counterFunct(&counter);
                                globalServer()->pHash->processHash(threadId, counterFunct);
                                while (outBuffer.size() - outBufferBytes < SOME_TEXT_LEN)
                                    outBuffer.resize(2*outBuffer.size());
                                size_t itoaLen = 0;
                                itoa((size_t) counter, outBuffer.data() + outBufferBytes, itoaLen);
                                outBufferBytes += itoaLen; // no +1 - skip space at the end
                                *(outBuffer.data() + outBufferBytes) = '\r';
                                *(outBuffer.data() + outBufferBytes+1) = '\n';
                                outBufferBytes += 2;
                            }
                            break;
                            case RequestInfo::INCR:
                            case RequestInfo::DECR:
                            case RequestInfo::TOUCH:
                            case RequestInfo::STAT:
                            case RequestInfo::LRU_CRAWLER:
                            case RequestInfo::SLABS:
                            case RequestInfo::UNSUPPORTED:
                            case RequestInfo::TYPE_MAX:
                            default:
                            break;
                        }
                        outBuffer[outBufferBytes] = 0;
                        requestInfo.state = RequestInfo::DONE_STATE;
                        return true;
                    }
                    else
                    {
                        if (!dataBytesRequired)
                            dataBytesRequired += (dataLen - inBufferBytes + inBufferOffset);
                        return false;
                    }
                }
                break;
                case RequestInfo::DONE_STATE:
                {
                    requestInfo.state = RequestInfo::READY_STATE;
                    requestInfo.reset();
                }
                break;
                default:
                break;
            }
        }
        return false;
    }

    // accepts header start, parses up to maxChars, updates pEnd, returns true on successful parse false otherwise
    // TODO: use SSE42 string stuff for string searches
    bool parseMemcachedHeader(char* pBeg, char*& pEnd, RequestInfo& requestInfo, size_t maxCharsToParse)
    {
        // parse command first
        char* pLimit = pBeg + maxCharsToParse;
        char* pCur = pBeg;
        while(*pCur != ' ' && *pCur != '\r' && pCur != pLimit)
            ++pCur;

        if (pCur == pLimit)
            return false;

        size_t commandIndex = 0;
        if (!globalServer()->pMatcher->MatchSubstring(pBeg, pCur, commandIndex))
            return false;
        else
            requestInfo.requestType = (RequestInfo::CacheRequestType) commandIndex;

        switch (requestInfo.requestType)
        {
            // key flags exptime bytes noreply?
            case RequestInfo::SET:
            case RequestInfo::ADD:
            case RequestInfo::REPLACE:
            case RequestInfo::APPEND:
            case RequestInfo::PREPEND:
            case RequestInfo::CAS:
            {
                ++pCur; //skip space
                requestInfo.pKeyBuffer = pCur;
                char* pParsePoint = pCur;
                while(*pCur != ' ' && pCur != pLimit)
                    ++pCur;
                if (pCur == pLimit)
                    return false;
                requestInfo.keyBufferLen = pCur - requestInfo.pKeyBuffer;
                ++pCur; //skip space
                pParsePoint = pCur;
                while(*pCur != ' ' && pCur != pLimit)
                    ++pCur;
                if (pCur == pLimit)
                    return false;
                requestInfo.flags = atous(pParsePoint, pCur);
                ++pCur; //skip space
                pParsePoint = pCur;
                while(*pCur != ' ' && pCur != pLimit)
                    ++pCur;
                if (pCur == pLimit)
                    return false;
                requestInfo.expTime = atoui(pParsePoint, pCur);
                ++pCur; //skip space
                pParsePoint = pCur;
                while(*pCur != ' ' && *pCur != '\r' && pCur != pLimit)
                    ++pCur;
                if (pCur == pLimit)
                    return false;
                requestInfo.dataLen = atoull(pParsePoint, pCur);
                if (requestInfo.requestType == RequestInfo::CAS)
                {
                    ++pCur; //skip space
                    pParsePoint = pCur;
                    while(*pCur != ' ' && *pCur != '\r' && pCur != pLimit)
                        ++pCur;
                    if (pCur == pLimit)
                        return false;
                    requestInfo.casTicket = atoull(pParsePoint, pCur);
                }
                if (*pCur == ' ')
                {
                    requestInfo.requestFlags |= NOREPLY;
                    pCur += 9;
                }
                else
                    pCur += 2;
                requestInfo.pDataBuffer = pCur;
                pEnd = pCur;
                return true;
            }
            break;
            // key key? key? ....
            case RequestInfo::GET:
            case RequestInfo::GETS:
            {
                ++pCur; //skip space
                requestInfo.pKeyBuffer = pCur;
                while(*pCur != ' ' && *pCur != '\r' && pCur != pLimit)
                    ++pCur;
                if (pCur == pLimit)
                    return false;
                requestInfo.keyBufferLen = pCur - requestInfo.pKeyBuffer;
                while(*pCur == ' ')
                {
                    if (!requestInfo.getKeysList)
                        requestInfo.getKeysList = new std::list<std::pair<char*, uint16_t> >();

                    ++pCur; //skip space
                    std::pair<char*, uint16_t> nextKey;
                    nextKey.first = pCur;
                    while(*pCur != ' ' && *pCur != '\r' && pCur != pLimit)
                        ++pCur;
                    if (pCur == pLimit)
                        return false;
                    nextKey.second = pCur - nextKey.first;
                    requestInfo.getKeysList->push_back(nextKey);
                }
                pCur += 2;
                pEnd = pCur;
                return true;
            }
            break;
            // key noreply?
            case RequestInfo::DELETE:
            {
                abort();
                return true;
            }
            break;
            case RequestInfo::STATS:
            {
                pCur += 2;
                pEnd = pCur;
                return true;
            }
            break;
            // key bytes noreply?
            case RequestInfo::INCR:
            case RequestInfo::DECR:
            // key exptime noreply?
            case RequestInfo::TOUCH:
            // unsupported
            case RequestInfo::STAT:
            case RequestInfo::LRU_CRAWLER:
            case RequestInfo::SLABS:
            {
                abort();
            }
            break;
            default:
            abort();
        }
        return false;
    }

} /* namespace YAMemcachedServer */
