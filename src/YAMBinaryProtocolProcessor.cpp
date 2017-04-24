/*
 * YAMBinaryProtocolProcessor.cpp
 *
 *  Created on: Mar 4, 2017
 *      Author: kalujny
 */

#include "YAMBinaryProtocolProcessor.h"
#include "Server.hh"

namespace YAMemcachedServer
{
    void processBinaryRequest(size_t threadId, RequestInfo& requestInfo, char* inBuffer, size_t& inBufferOffset, const size_t& inBufferBytes, char* outBuffer, size_t& outBufferBytes)
    {
        StorageValue sValue;
        static const size_t HEADER_SIZE = 24;
        static const size_t CAS_OFFSET = 16;
        const char replyNotFound[] = "Not found";
        const size_t replyNotFoundLen = sizeof(replyNotFound);

        BinaryHeader responseHeader;
        responseHeader.magic = 0x81;

        BinaryHeader* pHeader =  (BinaryHeader*)(&(inBuffer[inBufferOffset]));
        if (pHeader->magic != 0x80)
        {
            // anything that is 0x80 request is not supported, skip the input and return
            inBufferOffset += HEADER_SIZE + pHeader->bodyLen;
            return;
        }
        else
        {
            switch (pHeader->opCode)
            {
                case YAMBinaryOpcodes::GET :
                case YAMBinaryOpcodes::GETQ :
                {
                    if (!pHeader->keyLen || pHeader->keyLen != pHeader->bodyLen)
                    {
                        // incorrect get request, skip
                        inBufferOffset += HEADER_SIZE + pHeader->bodyLen;
                        return;
                    }
                    sso23::string key = sso23::string((char*)pHeader+HEADER_SIZE, pHeader->keyLen);
                    bool found = globalServer()->pHash->get(threadId, key, sValue);
                    if(found)
                    {
                        responseHeader.status = YAMBinaryStatus::NO_ERROR;
                        responseHeader.keyLen = 0;
                        responseHeader.extrasLen = 4;
                        responseHeader.bodyLen = 4 + sValue.value.size();
                        memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                        outBufferBytes += HEADER_SIZE;
                        *((uint32_t*)(&(outBuffer[outBufferBytes]))) = sValue.flags; // TODO: byte order?
                        outBufferBytes += sizeof(uint32_t);
                        memcpy(&(outBuffer[outBufferBytes]), sValue.value.c_str(), sValue.value.size());
                        outBufferBytes += sValue.value.size();
                    }
                    else if (pHeader->opCode == YAMBinaryOpcodes::GET)
                    {
                        responseHeader.status = YAMBinaryStatus::KEY_NOT_FOUND;
                        responseHeader.keyLen = 0;
                        responseHeader.extrasLen = 0;
                        responseHeader.bodyLen = replyNotFoundLen;
                        memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                        outBufferBytes += HEADER_SIZE;
                        memcpy(&(outBuffer[outBufferBytes]), replyNotFound, replyNotFoundLen);
                        outBufferBytes += replyNotFoundLen;
                    }
                }
                break;
                case YAMBinaryOpcodes::GETK :
                case YAMBinaryOpcodes::GETKQ :
                {
                    if (!pHeader->keyLen || pHeader->keyLen != pHeader->bodyLen)
                    {
                        // incorrect get request, skip
                        inBufferOffset += HEADER_SIZE + pHeader->bodyLen;
                        return;
                    }
                    sso23::string key = sso23::string((char*)pHeader+HEADER_SIZE, pHeader->keyLen);
                    bool found = globalServer()->pHash->get(threadId, key, sValue);
                    if(found)
                    {
                        responseHeader.status = YAMBinaryStatus::NO_ERROR;
                        responseHeader.keyLen = key.size();
                        responseHeader.extrasLen = 4;
                        responseHeader.bodyLen = 4 + sValue.value.size() + key.size();
                        memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                        outBufferBytes += HEADER_SIZE;
                        *((uint32_t*)(&(outBuffer[outBufferBytes]))) = sValue.flags; // TODO: byte order?
                        outBufferBytes += sizeof(uint32_t);
                        memcpy(&(outBuffer[outBufferBytes]), key.c_str(), key.size());
                        outBufferBytes += key.size();
                        memcpy(&(outBuffer[outBufferBytes]), sValue.value.c_str(), sValue.value.size());
                        outBufferBytes += sValue.value.size();
                    }
                    else if (pHeader->opCode == YAMBinaryOpcodes::GETK)
                    {
                        responseHeader.status = YAMBinaryStatus::KEY_NOT_FOUND;
                        responseHeader.keyLen = key.size();
                        responseHeader.extrasLen = 0;
                        responseHeader.bodyLen = replyNotFoundLen + key.size();
                        memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                        outBufferBytes += HEADER_SIZE;
                        memcpy(&(outBuffer[outBufferBytes]), key.c_str(), key.size());
                        outBufferBytes += key.size();
                        memcpy(&(outBuffer[outBufferBytes]), replyNotFound, replyNotFoundLen);
                        outBufferBytes += replyNotFoundLen;
                    }
                }
                break;
                case YAMBinaryOpcodes::SET :
                case YAMBinaryOpcodes::SETQ :
                {
                    if (!pHeader->keyLen || !pHeader->extrasLen)
                    {
                        // incorrect set request, skip
                        inBufferOffset += HEADER_SIZE + pHeader->bodyLen;
                        return;
                    }
                    sso23::string key = sso23::string((char*)pHeader+HEADER_SIZE + pHeader->extrasLen, pHeader->keyLen);
                    sValue.flags = *((uint32_t*)(&(inBuffer[inBufferOffset + HEADER_SIZE])));
                    sValue.casTicket = *((uint32_t*)(&(inBuffer[inBufferOffset + CAS_OFFSET])));
                    sValue.value = sso23::string((char*)pHeader+HEADER_SIZE + pHeader->extrasLen + pHeader->keyLen, pHeader->bodyLen - pHeader->extrasLen + pHeader->keyLen);
                    globalServer()->pHash->insertOrSet(threadId, key, sValue, requestInfo.expTime);
                    if(pHeader->opCode == YAMBinaryOpcodes::SET)
                    {
                        responseHeader.status = YAMBinaryStatus::NO_ERROR;
                        responseHeader.keyLen = 0;
                        responseHeader.extrasLen = 0;
                        responseHeader.bodyLen = 0;
                        memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                        outBufferBytes += HEADER_SIZE;
                    }
                }
                case YAMBinaryOpcodes::ADD :
                case YAMBinaryOpcodes::ADDQ :
                {
                    if (!pHeader->keyLen || !pHeader->extrasLen)
                    {
                        // incorrect set request, skip
                        inBufferOffset += HEADER_SIZE + pHeader->bodyLen;
                        return;
                    }
                    sso23::string key = sso23::string((char*)pHeader+HEADER_SIZE + pHeader->extrasLen, pHeader->keyLen);
                    sValue.flags = *((uint32_t*)(&(inBuffer[inBufferOffset + HEADER_SIZE])));
                    sValue.casTicket = *((uint32_t*)(&(inBuffer[inBufferOffset + CAS_OFFSET])));
                    sValue.value = sso23::string((char*)pHeader+HEADER_SIZE + pHeader->extrasLen + pHeader->keyLen, pHeader->bodyLen - pHeader->extrasLen + pHeader->keyLen);
                    bool inserted = globalServer()->pHash->insert(threadId, key, sValue, requestInfo.expTime);
                    if (inserted)
                    {
                        if (pHeader->opCode == YAMBinaryOpcodes::ADD)
                        {
                            responseHeader.status = YAMBinaryStatus::NO_ERROR;
                            responseHeader.keyLen = 0;
                            responseHeader.extrasLen = 0;
                            responseHeader.bodyLen = 0;
                            memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                            outBufferBytes += HEADER_SIZE;
                        }
                    }
                    else
                    {
                        responseHeader.status = YAMBinaryStatus::KEY_EXISTS;
                        responseHeader.keyLen = 0;
                        responseHeader.extrasLen = 0;
                        responseHeader.bodyLen = 0;
                        memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                        outBufferBytes += HEADER_SIZE;
                    }
                }
                break;
                case YAMBinaryOpcodes::REPLACE :
                case YAMBinaryOpcodes::REPLACEQ :
                {
                    if (!pHeader->keyLen || !pHeader->extrasLen)
                    {
                        // incorrect set request, skip
                        inBufferOffset += HEADER_SIZE + pHeader->bodyLen;
                        return;
                    }
                    if (!pHeader->casTicket)
                    {
                        sso23::string key = sso23::string((char*)pHeader+HEADER_SIZE + pHeader->extrasLen, pHeader->keyLen);
                        sValue.flags = *((uint32_t*)(&(inBuffer[inBufferOffset + HEADER_SIZE])));
                        sValue.casTicket = *((uint32_t*)(&(inBuffer[inBufferOffset + CAS_OFFSET])));
                        sValue.value = sso23::string((char*)pHeader+HEADER_SIZE + pHeader->extrasLen + pHeader->keyLen, pHeader->bodyLen - pHeader->extrasLen + pHeader->keyLen);
                        bool overwritten = globalServer()->pHash->set(threadId, key, sValue, requestInfo.expTime);
                        if (overwritten)
                        {
                            if(pHeader->opCode == YAMBinaryOpcodes::REPLACE)
                            {
                                responseHeader.status = YAMBinaryStatus::NO_ERROR;
                                responseHeader.keyLen = 0;
                                responseHeader.extrasLen = 0;
                                responseHeader.bodyLen = 0;
                                memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                                outBufferBytes += HEADER_SIZE;
                            }
                        }
                        else
                        {
                            responseHeader.status = YAMBinaryStatus::KEY_NOT_FOUND;
                            responseHeader.keyLen = 0;
                            responseHeader.extrasLen = 0;
                            responseHeader.bodyLen = 0;
                            memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                            outBufferBytes += HEADER_SIZE;
                        }
                    }
                    else
                    {
                        sso23::string key = sso23::string((char*)pHeader+HEADER_SIZE + pHeader->extrasLen, pHeader->keyLen);

                        unsigned int locks[LFStringMap::HASH_MAX_LOCK_DEPTH];
                        unsigned long long locksStart = 0;
                        unsigned long long locksEnd = 0;

                        StorageValue oldValue;
                        bool exists = globalServer()->pHash->get(threadId, key, oldValue, locks, locksStart, locksEnd);

                        if (exists)
                        {
                            sValue.flags = *((uint32_t*)(&(inBuffer[inBufferOffset + HEADER_SIZE])));
                            sValue.casTicket = *((uint32_t*)(&(inBuffer[inBufferOffset + CAS_OFFSET])));
                            sValue.value = sso23::string((char*)pHeader+HEADER_SIZE + pHeader->extrasLen + pHeader->keyLen, pHeader->bodyLen - pHeader->extrasLen + pHeader->keyLen);

                            if (oldValue.casTicket == sValue.casTicket)
                            {
                                sValue.casTicket += 1;
                                globalServer()->pHash->set(threadId, key, sValue, requestInfo.expTime, locks, locksStart, locksEnd);
                                if(pHeader->opCode == YAMBinaryOpcodes::REPLACE)
                                {
                                    responseHeader.status = YAMBinaryStatus::NO_ERROR;
                                    responseHeader.keyLen = 0;
                                    responseHeader.extrasLen = 0;
                                    responseHeader.bodyLen = 0;
                                    responseHeader.casTicket = sValue.casTicket;
                                    memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                                    outBufferBytes += HEADER_SIZE;
                                }
                            }
                            else
                            {
                                globalServer()->pHash->unlockElement(threadId, key, locks, locksStart, locksEnd);

                                responseHeader.status = YAMBinaryStatus::ITEM_NOT_STORED; // WTF? no explicit status for CAS mismatch in their protocol
                                responseHeader.keyLen = 0;
                                responseHeader.extrasLen = 0;
                                responseHeader.bodyLen = 0;
                                memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                                outBufferBytes += HEADER_SIZE;
                            }

                        }
                        else
                        {
                            globalServer()->pHash->unlockElement(threadId, key, locks, locksStart, locksEnd);

                            responseHeader.status = YAMBinaryStatus::KEY_NOT_FOUND;
                            responseHeader.keyLen = 0;
                            responseHeader.extrasLen = 0;
                            responseHeader.bodyLen = 0;
                            memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                            outBufferBytes += HEADER_SIZE;
                        }
                    }
                }
                break;
                case YAMBinaryOpcodes::DELETE :
                case YAMBinaryOpcodes::DELETEQ :
                {
                    if (!pHeader->keyLen)
                    {
                        // incorrect delete request, skip
                        inBufferOffset += HEADER_SIZE + pHeader->bodyLen;
                        return;
                    }
                    sso23::string key = sso23::string((char*)pHeader+HEADER_SIZE + pHeader->extrasLen, pHeader->keyLen);
                    bool deleted = globalServer()->pHash->remove(threadId, key);
                    if (deleted)
                    {
                        if(pHeader->opCode == YAMBinaryOpcodes::DELETE)
                        {
                            responseHeader.status = YAMBinaryStatus::NO_ERROR;
                            responseHeader.keyLen = 0;
                            responseHeader.extrasLen = 0;
                            responseHeader.bodyLen = 0;
                            memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                            outBufferBytes += HEADER_SIZE;
                        }
                    }
                    else
                    {
                        responseHeader.status = YAMBinaryStatus::KEY_NOT_FOUND;
                        responseHeader.keyLen = 0;
                        responseHeader.extrasLen = 0;
                        responseHeader.bodyLen = 0;
                        memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                        outBufferBytes += HEADER_SIZE;
                    }
                }
                break;
                case YAMBinaryOpcodes::NOOP :
                {
                    responseHeader.status = YAMBinaryStatus::NO_ERROR;
                    responseHeader.keyLen = 0;
                    responseHeader.extrasLen = 0;
                    responseHeader.bodyLen = 0;
                    memcpy(&(outBuffer[outBufferBytes]), &responseHeader, HEADER_SIZE);
                    outBufferBytes += HEADER_SIZE;
                }
                break;
            }
        }
    }

} /* namespace YAMemcachedServer */
