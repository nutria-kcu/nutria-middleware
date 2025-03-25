#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <windows.h>

bool isValidNumber(const std::string& str);

bool dll_injection(
    __in DWORD pid,
    __in const std::wstring& dll_name
);

DWORD getProcessID(const std::wstring& processName);

DWORD check_pid(const std::wstring& process_name);

std::wstring get_current_directory();

#endif // UTIL_H#pragma once
