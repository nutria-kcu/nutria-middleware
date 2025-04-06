#include <iostream>
#include <conio.h>
#include "controller.h"
#include "util.h"

using namespace std;

bool sendMessage(Controller* controller, int cmd, int option) {
    if (controller) {
        return controller->sendMSG(cmd, option);
    }
    return false;
}

Controller* initController() {
    cout << "Initializing controller...\n";
    return new Controller();
}

void destroyController(Controller* controller) {
    if (controller) {
        delete controller;
    }
}

int main() {
    bool inited = false;
    Controller* controller = nullptr;
    int cmd, option;

    cout << "Press 'i' to initialize, 's' to send, 'q' to quit.\n";

    while (true) {
        char ch = _getch();

        if (ch == 'i') {
            if (!inited) {
                controller = initController();
                inited = true;
                cout << "Controller initialized.\n";
            }
            else {
                cout << "Already initialized.\n";
            }
        }

        else if (ch == 's') {
            if (!inited) {
                cout << "Not initialized! Press 'i' first.\n";
                continue;
            }

            cout << "Enter command (number): ";
            string cmdInput;
            getline(cin, cmdInput);

            cmd = (cmdInput.empty() || !isValidNumber(cmdInput)) ? 0 : stoi(cmdInput);

            cout << "Enter option (number): ";
            string optionInput;
            getline(cin, optionInput);

            option = (optionInput.empty() || !isValidNumber(optionInput)) ? 0 : stoi(optionInput);

            cout << "SetMSG with cmd: " << cmd << ", option: " << option << endl;
            sendMessage(controller, cmd, option);
        }

        else if (ch == 'q') {
            break;
        }
    }

    destroyController(controller);
    return 0;
}
