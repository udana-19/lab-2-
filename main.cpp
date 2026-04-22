#include "mbed.h"
#include <chrono>

using namespace std::chrono;

// Requirement i: User Passcode (D2, D3, D4, D5)
const int PASSCODE[4] = {0, 1, 2, 3};   
// Requirement iv: Admin Code (D7, D6, D3, D2)
const int ADMIN_CODE[4] = {5, 4, 1, 0}; 

int main() {
    // Inputs: D2=0, D3=1, D4=2, D5=3, D6=4, D7=5
    DigitalIn buttons[] = {DigitalIn(D2), DigitalIn(D3), DigitalIn(D4), DigitalIn(D5), DigitalIn(D6), DigitalIn(D7)};
    
    // Outputs
    DigitalOut warningLed(LED1);    // Green
    DigitalOut lockdownLed(LED2);   // Blue
    DigitalOut secondaryLed(LED3);  // Red

    for(int i=0; i<6; i++) buttons[i].mode(PullDown);

    int enteredCode[4];
    int codeIndex = 0;
    int incorrectAttempts = 0;
    
    Timer timer;
    enum SystemState { NORMAL, WARNING, LOCKDOWN };
    SystemState currentState = NORMAL;
    timer.start();

    while (true) {
        auto elapsed = timer.elapsed_time();

        // --- 1. WARNING STATE (30s) ---
        if (currentState == WARNING) {
            if (elapsed < 30s) {
                warningLed = (duration_cast<milliseconds>(elapsed).count() / 500) % 2;
                lockdownLed = 0;
                secondaryLed = 0;
                continue; 
            } else {
                warningLed = 0;
                currentState = NORMAL;
                codeIndex = 0;
                timer.reset();
            }
        } 

        // --- 2. LOCKDOWN STATE (Requirement iv) ---
        else if (currentState == LOCKDOWN) {
            // Requirement iii: Blue Solid, Red Blinks
            lockdownLed = 1; 
            secondaryLed = (duration_cast<milliseconds>(elapsed).count() / 250) % 2;
            warningLed = 0; // Green must be OFF during lockdown unless pressing a button

            for (int i = 0; i < 6; i++) {
                if (buttons[i] == 1) {
                    enteredCode[codeIndex] = i;
                    codeIndex++;
                    
                    // Feedback: Green LED stays ON only while you hold the button
                    warningLed = 1; 
                    while(buttons[i] == 1); // WAIT for release
                    thread_sleep_for(100);  // DEBOUNCE
                    warningLed = 0;

                    if (codeIndex == 4) {
                        bool adminMatch = true;
                        for(int j=0; j<4; j++) {
                            if(enteredCode[j] != ADMIN_CODE[j]) adminMatch = false;
                        }

                        if (adminMatch) {
                            // --- REQUIREMENT IV & V: THE RESET ---
                            currentState = NORMAL;
                            incorrectAttempts = 0;
                            codeIndex = 0;
                            
                            // FORCE ALL OFF IMMEDIATELY
                            warningLed = 0;
                            lockdownLed = 0;
                            secondaryLed = 0;
                            
                            timer.reset();
                            break; // Kill the button loop
                        } else {
                            codeIndex = 0; // Try again
                        }
                    }
                }
            }
            if (currentState == NORMAL) continue; // Skip the rest of the loop to stay OFF
        }

        // --- 3. NORMAL STATE ---
        else if (currentState == NORMAL) {
            for (int i = 0; i < 6; i++) {
                if (buttons[i] == 1) {
                    enteredCode[codeIndex] = i;
                    codeIndex++;
                    
                    warningLed = 1; 
                    while(buttons[i] == 1); 
                    thread_sleep_for(100); 
                    warningLed = 0;

                    if (codeIndex == 4) {
                        bool match = true;
                        for(int j=0; j<4; j++) if(enteredCode[j] != PASSCODE[j]) match = false;

                        if (match) {
                            incorrectAttempts = 0;
                            codeIndex = 0;
                        } else {
                            incorrectAttempts++;
                            codeIndex = 0;
                            if (incorrectAttempts == 3) {
                                currentState = WARNING;
                                timer.reset();
                            } else if (incorrectAttempts >= 4) {
                                currentState = LOCKDOWN;
                                timer.reset();
                            }
                        }
                    }
                }
            }
        }
    }
}