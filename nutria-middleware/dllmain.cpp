// dllmain.cpp : Defines the entry point for the DLL application.
// need to be refactored 
#include "pch.h"
#include <iostream>
#include "controller.h"
#include "util.h"

extern "C" __declspec(dllexport) bool sendMessage(Controller* controller, int cmd, int option);
extern "C" __declspec(dllexport) Controller* initController();
extern "C" __declspec(dllexport) void destroyController(Controller* controller);
extern "C" __declspec(dllexport) bool isGameOn();

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        std::cout << "MIDDLEWARE DLL ATTACHED\n"; // Fixed std::cout statement
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

bool sendMessage(Controller* controller, int cmd, int option) {
    if (controller) {
        return controller->sendMSG(cmd, option);
    }
    return false; // Return false if the controller is not initialized
}

bool isGameOn() {
    DWORD result = check_pid(L"ac_client.exe");

    if (result) {
        return true;
    }
    else {
        return false;
    }
}

Controller* initController() {
    return new Controller();
}

void destroyController(Controller* controller) {
    if (controller) {  // Ensure the pointer is valid
        //controller->cleanup();
        delete controller;
    }
}
