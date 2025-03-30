#include "pch.h"
#include "util.h"
#include <tchar.h>
#include "Controller.h"

using namespace std;

// Constructor implementation
Controller::Controller() : state(0), isRunning(false) {
    cout << "Controller initiated\n";
    initialize();
    cout << "Controller initiated - DONE\n";
}

// Destructor implementation
Controller::~Controller() {
    // Clean up resources, if needed
    cout << "destruct controller\n";
    cleanup();
}

bool Controller::sendMSG(int cmd, int option) {
    bool status = false;

    DWORD result = WaitForSingleObject(sh->getEmptyEvent(), 0);
       
    if (result == WAIT_OBJECT_0) {
        cout << "Able to set message\n";
        status = sh->setMessage(cmd, option);
        if (status) {
            ResetEvent(sh->getEmptyEvent());
            SetEvent(sh->getFullEvent());
            cout << "set event signal\n";
        }
        return status;
    }
    else if (result == WAIT_TIMEOUT) {
        std::cout << "Not Able to set message\n";
    }
    else {
        std::cerr << "Error checking event state.\n";
    }
        
    return status;
}

void Controller::initialize() {
    
    bool result = false;
    //wstring targetProcess = L"ac_client.exe";
    wstring targetProcess = L"test32bitapp.exe";
    DWORD pid = check_pid(targetProcess);
    wstring hackCoreDLL = get_current_directory() + L"\\res\\kcu-hack.dll";

    sh = new SharedMemoryHandler(_T("NUTRI-IPC"), 1); // is producer

    if (sh == nullptr) {
        cout << "Failed to initialize SharedMemoryHandler\n";
        return;
    }

    // event signal 대기하기 -> hack-core init 완료대기해야함
    HANDLE hEventInit = CreateEvent(NULL, TRUE, FALSE, L"COREINIT");
    //ResetEvent(hEventInit);

    if (!start_32bit_injector(pid, hackCoreDLL)) {
        std::cerr << "Injection failed." << std::endl;
        return;
    }
    else {
        cout << "Injector returned\n";
    }

    //result = dll_injection(pid, hackCoreDLL);
    //if (!result) {
    //    cout << "Failed to inject kcu-hack.dll " << endl;
    //    wcout << hackCoreDLL << endl;
    //    return;
    //}

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

    CloseHandle(hEventInit);
}

bool Controller::start_32bit_injector(DWORD target_pid, const std::wstring& dll_path) {
    std::wstring command = L"32bit_inject_helper.exe " + std::to_wstring(target_pid) + L" " + dll_path;

    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(nullptr, &command[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        std::wcerr << L"Failed to start 32-bit injector. Error: " << GetLastError() << std::endl;
        return false;
    }

    // Wait for the 32-bit injector to complete (10-second timeout)
    DWORD waitResult = WaitForSingleObject(pi.hProcess, 10000);

    if (waitResult == WAIT_TIMEOUT) {
        std::wcerr << L"32-bit injector timed out after 10 seconds." << std::endl;
        TerminateProcess(pi.hProcess, 1);  // Kill the injector process
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return false;
    }

    // Close handles after completion
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}

void Controller::cleanup() {
    // Cleanup logic (e.g., releasing resources)
    delete sh;
}
