#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <windows.h>
#include <string>

#ifdef SHAREDMEMORY_EXPORTS
#define SHAREDMEMORY_API __declspec(dllexport)
#else
#define SHAREDMEMORY_API __declspec(dllimport)
#endif

struct SharedMemory {
    int cmd = -1;
    int option = -1;
    bool isMessageSet = false;
};

class SHAREDMEMORY_API SharedMemoryHandler {
private:
    HANDLE hMapFile;
    HANDLE hEventFull;
    HANDLE hEventEmpty;
    SharedMemory* shm;

    bool isProducer;
    void cleanup();
    void initProducer(const TCHAR* shmName);
    void initConsumer(const TCHAR* shmName);

public:
    SharedMemoryHandler(const TCHAR* shmName, const int isProducer);

    ~SharedMemoryHandler();

    bool setMessage(int cmd, int option);

    SharedMemory* getMessage();

    void resetEvent();

    HANDLE getFullEvent();

    HANDLE getEmptyEvent();
};

#endif // SHAREDMEMORY_H