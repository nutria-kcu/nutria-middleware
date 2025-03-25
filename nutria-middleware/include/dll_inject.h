#pragma once

#ifndef DLL_INJECT_H
#define DLL_INJECT_H

#include <windows.h>
#include <string>

// Function for DLL Injection
bool dll_injection(
    __in DWORD pid,
    __in const std::wstring& dll_name
);

#endif // DLL_INJECT_H#pragma once
