/*
 * LFLRUHashTableUtil.h
 *
 *  Created on: Dec 18, 2016
 *      Author: kalujny
 */

#ifndef LFLRUHASHTABLEUTIL_H_
#define LFLRUHASHTABLEUTIL_H_

#include <vector>

#include "LFLRUHashTable.h"

// TODO: implement all TODO's in LFSparseHashTableUtil
template<typename K, typename V, class HashFunc = std::hash<K> >
class LFLRUHashTableUtil
{
public:
    typedef LFLRUHashTable<K, V, HashFunc> LFHT;
    typedef typename LFHT::LRUBucket LFHTSB;
    typedef typename LFHT::LRUBucketElement LFHTSBE;

/*    static inline void addItemExpTime(LFHT* pHash, unsigned long long threadIdx, unsigned int elementIdx, uint32_t expTime);
    static inline void changeItemExpTime(LFHT* pHash, unsigned long long threadIdx, unsigned int elementIdx, uint32_t expTime);
    // read element links. if its in freelist - or in the head/tail of another threads LRUList return false. otherwise lock it and check expTime, if its a mach - fill outKey, unlock and return true, based on return call remove
    static inline bool expireElement(LFHT* pHash, unsigned long long threadIdx, unsigned int elementIdx, uint32_t expTime, K& outKey);*/
};

#endif /* LFLRUHASHTABLEUTIL_H_ */
