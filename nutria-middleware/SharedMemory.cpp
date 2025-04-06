#include "pch.h"
#include "SharedMemory.h"
#include <iostream>
#include <tchar.h>

void SharedMemoryHandler::printEventStatus(HANDLE hEvent, const std::wstring& eventName) {
    DWORD status = WaitForSingleObject(hEvent, 0);

    if (status == WAIT_OBJECT_0) {
        std::wcout << L"Event " << eventName << L" is SIGNED (Set)\n";
    }
    else if (status == WAIT_TIMEOUT) {
        std::wcout << L"Event " << eventName << L" is NOT SIGNED (Reset)\n";
    }
    else {
        std::wcout << L"Event " << eventName << L" status unknown (Error)\n";
    }
}

SharedMemoryHandler::SharedMemoryHandler(const TCHAR* shmName, const int isProducer) {
    SharedMemoryHandler::isProducer = isProducer;

    _tprintf(_T("SharedMemoryName: %s\n"), shmName);

    if (isProducer) {
        std::cout << "Shared Mem init - producer\n";
        initProducer(shmName);
    }
    else {
        std::cout << "Shared Mem init - consumer\n";
        initConsumer(shmName);
    }

    if (!hMapFile) {
        std::cerr << "Failed to open shared file\n";
        cleanup();
        exit(1);
    }

    if (!hEventEmpty || !hEventFull) {
        std::cerr << "Failed to open shared event\n";
        cleanup();
        exit(1);
    }

    shm = (SharedMemory*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory));
    if (!shm) {
        std::cerr << "Failed to map shared memory view\n";
        cleanup();
        exit(1);
    }
}

SharedMemoryHandler::~SharedMemoryHandler() {
    cleanup();
}

bool SharedMemoryHandler::setMessage(int input, int option) {
    if (!isProducer) {
        std::cout << "setMessage accessed from consumer\n";
        return false;
    }

    printEventStatus(hEventFull, L"full event");
    printEventStatus(hEventEmpty, L"empty event");

    if (shm->isMessageSet) {
        std::cout << "Message already set. Waiting for consumer to read.\n";
        std::cout << "Ignore the given msg\n";
        return false;
    }

    shm->cmd = input;
    shm->option = option;
    shm->isMessageSet = true;

    SetEvent(hEventFull);  // Notify consumer that message is available
    return true;
}

SharedMemory* SharedMemoryHandler::getMessage() {
    if (isProducer) {
        std::cout << "getMessage accessed from producer\n";
        return nullptr;
    }

    if (!shm->isMessageSet) {
        std::cout << "No command set.\n";
        return nullptr;
    }

    static SharedMemory msgCopy; // Stack-based object instead of heap allocation
    msgCopy = *shm;  // Copy data safely

    shm->isMessageSet = false;

    return &msgCopy;  // Returning reference instead of heap allocation
}

void SharedMemoryHandler::resetEvent() {
    ResetEvent(hEventEmpty);
    ResetEvent(hEventFull);
}

void SharedMemoryHandler::cleanup() {
    if (shm) {
        UnmapViewOfFile(shm);
        shm = nullptr;  // Prevent use-after-free
    }
    if (hMapFile) {
        CloseHandle(hMapFile);
        hMapFile = nullptr;
    }
    if (hEventFull) {
        CloseHandle(hEventFull);
        hEventFull = nullptr;
    }
    if (hEventEmpty) {
        CloseHandle(hEventEmpty);
        hEventEmpty = nullptr;
    }
}

void SharedMemoryHandler::initProducer(const TCHAR* shmName) {
    // 공유 메모리 생성
    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemory), shmName);

    if (hMapFile == NULL || hMapFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create shared memory. Error: " << GetLastError() << std::endl;
        return;
    }

    // 이벤트 생성 (자동 리셋이 아닌 수동 리셋)
    hEventEmpty = CreateEvent(NULL, TRUE, TRUE, L"IPCEMPTY");
    if (hEventEmpty == NULL) {
        std::cerr << "Failed to create event IPCEMPTY. Error: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return;
    }

    hEventFull = CreateEvent(NULL, TRUE, FALSE, L"IPCFULL");
    if (hEventFull == NULL) {
        std::cerr << "Failed to create event IPCFULL. Error: " << GetLastError() << std::endl;
        CloseHandle(hEventEmpty);
        CloseHandle(hMapFile);
        return;
    }

    // 공유 메모리 매핑
    shm = (SharedMemory*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory));
    if (!shm) {
        std::cerr << "Failed to map shared memory. Error: " << GetLastError() << std::endl;
        CloseHandle(hEventFull);
        CloseHandle(hEventEmpty);
        CloseHandle(hMapFile);
        return;
    }

    // 메모리 초기화 (정상적으로 매핑된 경우에만)
    ZeroMemory(shm, sizeof(SharedMemory));
}

void SharedMemoryHandler::initConsumer(const TCHAR* shmName) {
    hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, shmName);
    if (!hMapFile) {
        std::cerr << "Failed to open shared memory. Error: " << GetLastError() << std::endl;
        return;
    }

    hEventEmpty = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"IPCEMPTY");
    if (!hEventEmpty) {
        std::cerr << "Failed to open IPCEMPTY event. Error: " << GetLastError() << std::endl;
    }

    hEventFull = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"IPCFULL");
    if (!hEventFull) {
        std::cerr << "Failed to open IPCFULL event. Error: " << GetLastError() << std::endl;
    }

    shm = (SharedMemory*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory));
    if (!shm) {
        std::cerr << "Failed to map shared memory in consumer. Error: " << GetLastError() << std::endl;
        cleanup();
        exit(1);
    }
}

HANDLE SharedMemoryHandler::getEmptyEvent() {
    return hEventEmpty;
}

HANDLE SharedMemoryHandler::getFullEvent() {
    return hEventFull;
}