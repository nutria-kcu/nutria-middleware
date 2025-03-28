#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <iostream>
#include <windows.h>
#include <SharedMemory.h>

// Define export/import macro for DLL usage
//#ifdef BUILDING_DLL
//#define DLL_API __declspec(dllexport)  // Export when building the DLL
//#else
//#define DLL_API __declspec(dllimport)  // Import when using the DLL
//#endif

struct Message {
    int cmd;
    int option;
};

class Controller {
public:
    // Constructor
    Controller();

    // Destructor
    ~Controller();

    bool sendMSG(int cmd, int option);

private:
    // Private Member Variables (State, configuration, etc.)
    SharedMemoryHandler* sh; // Shared memory handler pointer (static member)
    int state;  // Example state variable
    bool isRunning; // Flag to check if the controller is running

    // Private Helper Methods (Optional)
    void initialize(); // Method to initialize the controller
    void cleanup();    // Method to clean up resources
    bool start_32bit_injector(DWORD target_pid, const std::wstring& dll_path);
};

//// These functions are used to create/destroy Controller instances
//extern "C" DLL_API Controller* createController(); // To create a Controller instance
//extern "C" DLL_API void destroyController(Controller* controller); // To destroy a Controller instance

#endif // CONTROLLER_H
