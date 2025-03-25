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
        // get handle for process with given pid
        if (process_handle == nullptr) {
            break;
        }
        // allocate mem for given dll at VAS
        remote_buffer = VirtualAllocEx(
            process_handle,
            nullptr,
            dll_name.size(),
            MEM_COMMIT, // Allocates memory charges (from the overall size of memory and the paging files on disk) for the specified reserved memory pages.
            PAGE_READWRITE // protection
        );

        if (!remote_buffer) {
            break;
        } // when failed

        // dll injection (write dll path string)
        if (!WriteProcessMemory(
            process_handle,
            remote_buffer,
            dll_name.c_str(),
            dll_name.size() * sizeof(wchar_t),
            nullptr)
            ) {
            break;
        }

        // get kernel32.dll 's handle
        module = GetModuleHandle(L"kernel32.dll");

        // load and store pointer to LoadLibrary function in Kernel32.dll
        thread_start_routine = (LPTHREAD_START_ROUTINE)GetProcAddress(module, "LoadLibraryW");


        // CreateRemoteThread() �Լ��� �����带 �����ϴ� �Լ��ε�, 
        // �� �Լ��� ȣ���ϴ� ���μ����� �ƴ� �ٸ� ���μ����� �����带 �����ϴ� ����� ������ �ִ�
        // ���⼭ ����ִ� ���� DLL�� �о ���డ���� ������ �����ϴ� �Լ���
        // LoadLibrary() �Լ��� ������ ���ν����� ����� ����� ������ �ִٴ� ���̴�. CreateRemoteThread()�� Ȱ���ϴ� DLL injection��, 
        // �ٷ� �� ���� �̿��ؼ� ���ϴ� DLL�� �����ϴ� �����带 �������� ��� ���μ����� ���� �ڵ带 �����ϴ� ����̴�
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

        // time out 10 sec
        DWORD waitResult = WaitForSingleObject(thread_handle, 10000);

        switch (waitResult) {
        case WAIT_OBJECT_0:
            std::cout << "WAIT_OBJECT_0: The object is signaled." << std::endl;
            result = true;
            break;

        case WAIT_TIMEOUT:
            std::cout << "WAIT_TIMEOUT: The wait timed out before the object became signaled." << std::endl;
            result = false;
            break;

        case WAIT_ABANDONED:
            std::cout << "WAIT_ABANDONED: A mutex was not released before the owning thread terminated. Check for data consistency." << std::endl;
            result = false;
            break;

        case WAIT_FAILED:
            std::cerr << "WAIT_FAILED: The function failed. Error code: " << GetLastError() << std::endl;
            result = false;
            break;

        default:
            std::cerr << "Unknown return value: " << result << std::endl;
            result = false;
            break;
        }

        
    } while (false);

    CloseHandle(process_handle);
    CloseHandle(thread_handle);

    return result;
}