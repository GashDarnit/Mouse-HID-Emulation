#include <windows.h>
#include <iostream>
#include <thread>
#include <map>

HHOOK hHook = NULL;
bool suppressionActive = true;
const int MOVE_STEP = 10;
const float FAST_MOVE_FACTOR = 2.0f;
const float SLOW_MOVE_FACTOR = 0.3f;
const int MOVE_DELAY_MS = 10;
volatile bool running = true; 

bool isLeftButtonPressed = false;

void moveCursor(int dx, int dy) {
    POINT cursorPos;
    if (GetCursorPos(&cursorPos)) {
        SetCursorPos(cursorPos.x + dx, cursorPos.y + dy);
    }
}

void leftClick() {
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

void rightClick() {
    mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
}

void pressLeftButton() {
    if (!isLeftButtonPressed) {
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        isLeftButtonPressed = true;
    }
}

void releaseLeftButton() {
    if (isLeftButtonPressed) {
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        isLeftButtonPressed = false;
    }
}

std::map<int, volatile bool> keyStates = {
    {'O', false}, {'K', false}, {'L', false}, {VK_OEM_1, false},
    {'C', false}, {'X', false}, {'Z', false}
};

void moveCursorContinuous() {
    while (running) {
        int currentMoveStep = MOVE_STEP;
        
        // Speed up cursor movement
        if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) { 
            currentMoveStep = static_cast<int>(MOVE_STEP * FAST_MOVE_FACTOR);
        }
        
        // Slow down cursor movement
        if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) { 
            currentMoveStep = static_cast<int>(MOVE_STEP * SLOW_MOVE_FACTOR);
        }

        if (suppressionActive) {
            if (keyStates['O']) moveCursor(0, -currentMoveStep);
            if (keyStates['K']) moveCursor(-currentMoveStep, 0);
            if (keyStates['L']) moveCursor(0, currentMoveStep);
            if (keyStates[VK_OEM_1]) moveCursor(currentMoveStep, 0);
            
            // Left and Right clicks
            if (keyStates['X']) {
                pressLeftButton();
            } else {
                releaseLeftButton();
            }

            if (keyStates['Z']) {
                rightClick();
                keyStates['Z'] = false;
            }
        }
        Sleep(MOVE_DELAY_MS);
    }
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && running) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
        bool ctrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;
        bool escapePressed = (kbStruct->vkCode == VK_ESCAPE);

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            // Ctrl + Escape to stop the program
            if (ctrlPressed && escapePressed) {
                std::cout << "Ctrl + Escape pressed. Exiting program...\n";
                exit(0);
            }

            // Ctrl + . to toggle suppression
            if (ctrlPressed && kbStruct->vkCode == VK_OEM_PERIOD) {
                suppressionActive = !suppressionActive; 
                std::cout << "Suppression " << (suppressionActive ? "enabled" : "disabled") << ".\n";
                return 1;
            }

            if (keyStates.count(kbStruct->vkCode)) {
                keyStates[kbStruct->vkCode] = true;
                if (suppressionActive) {
                    return 1;
                }
            }
        }

        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            if (keyStates.count(kbStruct->vkCode)) {
                keyStates[kbStruct->vkCode] = false;
                if (suppressionActive) {
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

int main() {
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!hHook) {
        std::cerr << "Failed to install hook.\n";
        return 1;
    }
    std::cout << "Press 'O', 'K', 'L', ';' to move the cursor. Toggle suppression with Ctrl + .\n";
    std::cout << "Press 'X' for left click and 'Z' for right click (yes, they are flipped).\n";
    std::cout << "Hold 'Left Shift' to move the cursor faster and 'left Ctrl' to move it slower.\n";
    std::cout << "Press Ctrl + Escape to stop the program.\n";

    std::thread cursorThread(moveCursorContinuous);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (!running) {
            break;
        }
    }

    cursorThread.join();
    UnhookWindowsHookEx(hHook);
    
    return 0;
}
