#include <iostream>
#include <conio.h>
#include "controller.h"
#include "util.h"

using namespace std;


int main() {
	Controller controller;

    int cmd, option;

	while (true) {
		char ch = _getch();

		if (ch == 's') {
            cout << "Enter command (number): ";
            string cmdInput;
            getline(cin, cmdInput);  // Read the full input line

            if (cmdInput.empty() || !isValidNumber(cmdInput)) {
                cmd = 0;  // If input is empty or not a valid number, set cmd to 0
            }
            else {
                cmd = stoi(cmdInput);  // Convert string to integer
            }

            // Prompt user for option input
            cout << "Enter option (number): ";
            string optionInput;
            getline(cin, optionInput);  // Read the full input line

            if (optionInput.empty() || !isValidNumber(optionInput)) {
                option = 0;  // If input is empty or not a valid number, set option to 0
            }
            else {
                option = stoi(optionInput);  // Convert string to integer
            }

            cout << "SetMSG with cmd: " << cmd << ", option: " << option << endl;

            controller.sendMSG(cmd, option);
		}
        if (ch == 'q') {
            break;
        }
	}

	return 0;
}