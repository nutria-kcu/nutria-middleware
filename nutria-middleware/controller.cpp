#include "pch.h"
#include "util.h"
#include <tchar.h>
#include "Controller.h"

using namespace std;

// Constructor implementation
Controller::Controller() : state(0), isRunning(false) {
    cout << "Controller initiated\n";
    initialize();
}

// Destructor implementation
Controller::~Controller() {
    // Clean up resources, if needed
    cleanup();
}

void Controller::start() {
    if (!isRunning) {
        isRunning = true;
        initialize();  // Initialization on start
        std::cout << "Controller started." << std::endl;
    }
}

void Controller::initialize() {
    bool result = false;
    wstring targetProcess = L"Notepad.exe";
    DWORD pid = check_pid(targetProcess);
    wstring sharedMemoryDLL = get_current_directory() + L"\\SharedMemory.dll";
    wstring hackCoreDLL = get_current_directory() + L"\\kcu-hack.dll";

    result = dll_injection(pid, sharedMemoryDLL);
    if (!result) {
        cout << "Failed to inject SharedMemory.dll\n";
        return;
    }
    result = dll_injection(pid, hackCoreDLL);
    if (!result) {
        cout << "Failed to inject kcu-hack.dll\n";
        return;
    }
    // event signal 대기하기 -> hack-core init 완료대기해야함
    sh = new SharedMemoryHandler(_T("NUTRI-IPC"), 1); // is producer

    if (sh == nullptr) {
        cout << "Failed to initialize SharedMemoryHandler\n";
        return;
    }
    
    HANDLE hEventInit = CreateEvent(NULL, TRUE, FALSE, L"COREINIT");

    DWORD waitResult = WaitForSingleObject(hEventInit, 5000);

    switch (waitResult) {
        case WAIT_OBJECT_0:
            cout << "The event was signaled." << endl;
            break;
        case WAIT_TIMEOUT:
            cout << "The wait timed out after 5 seconds." << endl;
            break;
        default:
            cerr << "Wait failed!" << endl;
    }

}

void Controller::cleanup() {
    // Cleanup logic (e.g., releasing resources)
    delete sh;
}