/*
 * YAMBinaryProtocolProcessor.h
 *
 *  Created on: Mar 4, 2017
 *      Author: kalujny
 */

#ifndef YAMBINARYPROTOCOLPROCESSOR_H_
#define YAMBINARYPROTOCOLPROCESSOR_H_

#include "Common.hh"

namespace YAMemcachedServer
{
    enum YAMBinaryStatus
    {
        NO_ERROR = 0,
        KEY_NOT_FOUND = 1,
        KEY_EXISTS = 2,
        VALUE_TOO_LARGE = 3,
        INVALID_ARGUMENTS = 4,
        ITEM_NOT_STORED = 5,
        INCR_DECR_NA = 6,
        WRONG_VBUCKET = 7,
        AUTHENTICATION_ERROR = 8,
        AUTHENTICATION_CONTINUE = 9,
        UNKNOWN_COMMAND = 81,
        OUT_OF_MEMORY = 82,
        NOT_SUPPORTED = 83,
        INTERNAL_ERROR = 84,
        BUSY = 85,
        TEMPORARY_FAILURE = 86
    };

    enum YAMBinaryOpcodes
    {
        GET = 0X00,
        SET = 0X01,
        ADD = 0X02,
        REPLACE = 0X03,
        DELETE = 0X04,
        INCREMENT = 0X05,
        DECREMENT = 0X06,
        QUIT = 0X07,
        FLUSH = 0X08,
        GETQ = 0X09,
        NOOP = 0X0A,
        VERSION = 0X0B,
        GETK = 0X0C,
        GETKQ = 0X0D,
        APPEND = 0X0E,
        PREPEND = 0X0F,
        STAT = 0X10,
        SETQ = 0X11,
        ADDQ = 0X12,
        REPLACEQ = 0X13,
        DELETEQ = 0X14,
        INCREMENTQ = 0X15,
        DECREMENTQ = 0X16,
        QUITQ = 0X17,
        FLUSHQ = 0X18,
        APPENDQ = 0X19,
        PREPENDQ = 0X1A,
        VERBOSITY = 0X1B,
        TOUCH = 0X1C,
        GAT = 0X1D,
        GATQ = 0X1E,
        SASL_LIST_MECHS = 0X20,
        SASL_AUTH = 0X21,
        SASL_STEP = 0X22,
        RGET = 0X30,
        RSET = 0X31,
        RSETQ = 0X32,
        RAPPEND = 0X33,
        RAPPENDQ = 0X34,
        RPREPEND = 0X35,
        RPREPENDQ = 0X36,
        RDELETE = 0X37,
        RDELETEQ = 0X38,
        RINCR = 0X39,
        RINCRQ = 0X3A,
        RDECR = 0X3B,
        RDECRQ = 0X3C,
        SET_VBUCKET = 0X3D,
        GET_VBUCKET = 0X3E,
        DEL_VBUCKET = 0X3F,
        TAP_CONNECT = 0X40,
        TAP_MUTATION = 0X41,
        TAP_DELETE = 0X42,
        TAP_FLUSH = 0X43,
        TAP_OPAQUE = 0X44,
        TAP_VBUCKET_SET = 0X45,
        TAP_CHECKPOINT_START = 0X46,
        TAP_CHECKPOINT_END = 0X47
    };

    struct BinaryHeader
    {
        unsigned char magic;
        unsigned char opCode;
        unsigned short keyLen;
        unsigned char extrasLen;
        unsigned char dataType;
        unsigned short status;
        unsigned int bodyLen; // total body len = extras len + key len + body len
        unsigned int opaque;
        unsigned long long casTicket;

    }__attribute((aligned(1),packed));

    void processBinaryRequest(size_t threadId, RequestInfo& requestInfo, char* inBuffer, size_t& inBufferOffset, const size_t& inBufferBytes, char* outBuffer, size_t& outBufferBytes);

} /* namespace YAMemcachedServer */

#endif /* YAMBINARYPROTOCOLPROCESSOR_H_ */
