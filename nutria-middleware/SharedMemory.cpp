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

    SharedMemory* msg = new SharedMemory(*shm);  // Copy message data
    shm->isMessageSet = false;

    ResetEvent(hEventFull);
    SetEvent(hEventEmpty);  // Notify producer that buffer is empty
    return msg;
}

void SharedMemoryHandler::resetEvent() {
    ResetEvent(hEventEmpty);
    ResetEvent(hEventFull);
}

void SharedMemoryHandler::cleanup() {
    if (shm) {
        UnmapViewOfFile(shm);
    }
    if (hMapFile) {
        CloseHandle(hMapFile);
    }
    if (hEventFull) {
        CloseHandle(hEventFull);
    }
    if (hEventEmpty) {
        CloseHandle(hEventEmpty);
    }
}

void SharedMemoryHandler::initProducer(const TCHAR* shmName) {
    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemory), shmName);

    if (!hMapFile) {
        std::cerr << "Failed to create shared memory\n";
        return;
    }

    hEventEmpty = CreateEvent(NULL, TRUE, TRUE, L"IPCEMPTY");
    hEventFull = CreateEvent(NULL, TRUE, FALSE, L"IPCFULL");

    if (GetLastError() != ERROR_ALREADY_EXISTS) {
        shm = (SharedMemory*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory));
        if (shm) {
            ZeroMemory(shm, sizeof(SharedMemory));  // Initialize only if newly created
        }
    }
}

void SharedMemoryHandler::initConsumer(const TCHAR* shmName) {
    hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, shmName);
    if (!hMapFile) {
        std::cerr << "Failed to open shared memory\n";
        return;
    }

    hEventEmpty = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"IPCEMPTY");
    hEventFull = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"IPCFULL");

    shm = (SharedMemory*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory));
    if (!shm) {
        std::cerr << "Failed to map shared memory in consumer\n";
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