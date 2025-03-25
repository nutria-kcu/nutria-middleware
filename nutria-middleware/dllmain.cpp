// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <iostream>
#include "controller.h"

extern "C" __declspec(dllexport) bool sendMessage(int cmd, int option);


Controller* controller;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        std::cout << "MIDDLEWARE DLL ATTACHED\n"; // Fixed std::cout statement
        controller = new Controller();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        if (controller) {
            delete controller; // Clean up the controller object when the DLL is unloaded
        }
        break;
    }
    return TRUE;
}

bool sendMessage(int cmd, int option) {
    if (controller) {
        return controller->sendMSG(cmd, option);
    }
    return false; // Return false if the controller is not initialized
}

