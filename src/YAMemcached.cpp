/*#include <iostream>
 #include <string.h>

 #include "ProtocolParser.h"

 int main(int argc, char * argv[])
 {
 memcached_parser daParser;
 const char daRequest[] = "add 123123 0 0 4 noreply\r\nasdf";
 CacheRequest daCacheRequest = {0};
 daParser.pRequest = &daCacheRequest;
 size_t daRequestLen = strlen(daRequest);
 memcached_parser_init(&daParser);
 memcached_parser_execute(&daParser, daRequest, daRequestLen, 0);
 daCacheRequest.pDataBuffer = (char*)daRequest + daParser.body_start;
 return 0;
 }*/

#include <unistd.h>
#include <signal.h>
#include <getopt.h>

#include "Common.hh"
#include "Server.hh"

#include "LFLRUHashTable.h"

//#define NDEBUG 1

struct size_t_hash : public std::unary_function<std::size_t, std::size_t>
{
    std::size_t operator() (const std::size_t & k) const
    {
        return XXH64 ((const void*) &k, sizeof(std::size_t), 0);
    }
};

YAMemcachedServer::Server* theServer;
bool exitMainThread;

void sig_handler(int signo)
{
    if(signo == SIGUSR1)
    {
        printf("received SIGUSR1\n");
//        LFSparseHashTableUtil<sso23::string, YAMemcachedServer::StorageValue, YAMemcachedServer::string_hash>::save(gpHash, "/media/data/Temp/omg.bin", YAMemcachedServer::saveFunc, YAMemcachedServer::sizeFunc);
    }
    else if(signo == SIGUSR2)
        printf("received SIGUSR2\n");
    else if(signo == SIGHUP)
        printf("received SIGHUP\n");
    else if(signo == SIGINT)
#ifdef NDEBUG
        printf("received SIGINT\n");
#else
    {
        printf("received SIGINT\n");
        theServer->stop();
        exitMainThread = true;
    }
#endif
    else if(signo == SIGTERM)
    {
        printf("received SIGTERM\n");
        theServer->stop();
        exitMainThread = true;
    }
}

void printVersion() 
{
    // version will be printed here
    exit(0);
}

// TODO: 1) UDP memcached protocol - debug
// TODO: 2) save/load
// TODO: 3) stats
// TODO: 4) jemalloc for strings

// TODO: OPTIONAL for real lulz implement UDP via AF_PACKET

int main(int argc, char * argv[])
{
    exitMainThread = false;
    theServer = YAMemcachedServer::globalServer();

    if(signal(SIGUSR1, sig_handler) == SIG_ERR)
        abort();
    if(signal(SIGUSR2, sig_handler) == SIG_ERR)
        abort();
    if(signal(SIGHUP, sig_handler) == SIG_ERR)
        abort();
    if(signal(SIGINT, sig_handler) == SIG_ERR)
        abort();
    if(signal(SIGTERM, sig_handler) == SIG_ERR)
        abort();

    theServer->maxHashSize = 1024*1024;

    char* loadFileName = 0;

    char c;
    while((c = getopt(argc, argv, "ut:s:p:l:v")) != EOF)
    {
        switch (c)
        {
            case 'u':
            theServer->udpEnabled = true;
            break;
            case 't':
            theServer->numThreads = atoi(optarg);
            break;
            case 'p':
            theServer->port = atoi(optarg);
            break;
            case 's':
            theServer->maxHashSize = atoi(optarg);
            if (theServer->maxHashSize*2 >= YAMemcachedServer::MAX_HASH_SIZE)
            {
                printf("Requested number of elements is too large\n");
                abort();
            }
            break;
            case 'l':
            loadFileName = optarg;
            break;
            case 'v':
            printf("sizeof(LFLRUHashTable<sso23::string, YAMemcachedServer::StorageValue, YAMemcachedServer::string_hash>) == %lu\n", sizeof(LFLRUHashTable<sso23::string, YAMemcachedServer::StorageValue, YAMemcachedServer::string_hash>));
            printf("sizeof(LFLRUHashTable<sso23::string, YAMemcachedServer::StorageValue, YAMemcachedServer::string_hash>::LRULinks) == %lu\n", sizeof(LFLRUHashTable<sso23::string, YAMemcachedServer::StorageValue, YAMemcachedServer::string_hash>::LRULinks));
            printf("sizeof(LFLRUHashTable<sso23::string, YAMemcachedServer::StorageValue, YAMemcachedServer::string_hash>::LRUElement) == %lu\n", sizeof(LFLRUHashTable<sso23::string, YAMemcachedServer::StorageValue, YAMemcachedServer::string_hash>::LRUElement));
            printf("sizeof(LFLRUHashTable<sso23::string, YAMemcachedServer::StorageValue, YAMemcachedServer::string_hash>::ThreadLRUList) == %lu\n", sizeof(LFLRUHashTable<sso23::string, YAMemcachedServer::StorageValue, YAMemcachedServer::string_hash>::ThreadLRUList));
            printVersion();
            break;
            default:
            return 1;
        }
    }

    theServer->pHash = new YAMemcachedServer::LFStringMap(theServer->maxHashSize, 2*theServer->maxHashSize, std::thread::hardware_concurrency());

    if(loadFileName)
    {
        printf("Loading data\n");
        //LFSparseHashTableUtil<sso23::string, YAMemcachedServer::StorageValue, YAMemcachedServer::string_hash>::load(theServer.pHash, loadFileName, YAMemcachedServer::loadFunc);
    }

    theServer->start();
    while(!exitMainThread)
        sleep(1);

    delete theServer;
    return 0;
}

