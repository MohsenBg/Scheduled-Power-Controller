#include <EEPROM.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <TimerOne.h>
#include <Wire.h>

// Constants
#define LCD_ADDRESS 0x20 // I2C address for the LCD
#define LCD_ROWS 4       // Number of rows on the LCD
#define LCD_COLUMNS 20   // Number of columns on the LCD
#define DEVICE_PIN 3     // Pin controlling the device (ON/OFF)

// Keypad configuration
const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLUMNS = 3;
const byte KEYPAD_ROW_PINS[KEYPAD_ROWS] = {7, 6, 5, 4};    // Row pins for the keypad
const byte KEYPAD_COL_PINS[KEYPAD_COLUMNS] = {11, 12, 13}; // Column pins for the keypad
const char KEYPAD_KEYS[KEYPAD_ROWS][KEYPAD_COLUMNS] = {    // Key mappings
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

// Globals
Keypad keypad = Keypad(makeKeymap(KEYPAD_KEYS), KEYPAD_ROW_PINS, KEYPAD_COL_PINS, KEYPAD_ROWS, KEYPAD_COLUMNS);
char currentPassword[5] = "0000";  // Default password
char enteredPassword[5];           // Stores entered password
unsigned long timeIntervalOn = 0;  // Duration for which the device remains ON
unsigned long timeIntervalOff = 0; // Duration for which the device remains OFF
bool isTimerOn = false;            // Indicates the current state of the timer (ON/OFF)

// LCD control class to manage LCD operations
class LCDControl {
private:
    LiquidCrystal_I2C lcd;                       // LCD object for I2C communication
    int cursorRow;                               // Current row of the cursor
    int cursorCol;                               // Current column of the cursor
    char currentText[LCD_ROWS][LCD_COLUMNS + 1]; // Buffer to store the current text displayed on the LCD

public:
    LCDControl()
        : lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS), cursorRow(0), cursorCol(0) {
        clearTextBuffer();
    }

    // Initializes the LCD, enabling the backlight and clearing the screen
    void begin() {
        lcd.init();
        lcd.backlight();
        lcd.clear();
    }

    // Displays a message on the LCD. Automatically handles line wrapping and clearing
    // when the screen is full.
    void print(const String &message) {
        for (int i = 0; i < message.length(); i++) {
            if (cursorCol >= LCD_COLUMNS) {
                cursorCol = 0;
                cursorRow++;

                if (cursorRow >= LCD_ROWS) {
                    cursorRow = 0;
                    lcd.clear();
                    clearTextBuffer();
                }
            }
            lcd.setCursor(cursorCol, cursorRow);
            lcd.print(message[i]);
            currentText[cursorRow][cursorCol] = message[i];
            cursorCol++;
        }
    }

    // Overloaded print function to handle C-style strings
    void print(const char *message) {
        print(String(message));
    }

    // Clears the LCD screen and resets the cursor to the top-left position
    void clear() {
        lcd.clear();
        cursorRow = 0;
        cursorCol = 0;
        clearTextBuffer();
    }

    // Sets the cursor position to the specified column and row
    void setCursor(int col, int row) {
        cursorRow = row;
        cursorCol = col;
        lcd.setCursor(col, row);
    }

    // Get the current column of the cursor
    int getCurrentCol() {
        return cursorCol;
    }

    // Get the current row of the cursor
    int getCurrentRow() {
        return cursorRow;
    }

    // Returns the current text displayed on the LCD as a single string
    String getCurrentText() const {
        String result = "";
        for (int row = 0; row < LCD_ROWS; row++) {
            result += String(currentText[row]) + "\n";
        }
        return result;
    }

    // Removes the last character from the display and updates the cursor position
    void removeLastCharacter() {
        // Adjust cursor position for the last character
        if (cursorCol > 0) {
            cursorCol--;
        } else if (cursorRow > 0) {
            cursorRow--;
            cursorCol = LCD_COLUMNS - 1;
        } else {
            // If at the top-left corner, nothing to remove
            return;
        }

        // Clear the last character from the display and the text buffer
        lcd.setCursor(cursorCol, cursorRow);
        lcd.print(' '); // Overwrite the character with a blank space
        currentText[cursorRow][cursorCol] = ' ';

        // Update the cursor position
        lcd.setCursor(cursorCol, cursorRow);
    }

    // Set Cursor Next Line
    void nextLine() {
        cursorRow += 1;
        cursorCol = 0;
    }

private:
    // Clears the internal text buffer used to store the displayed text
    void clearTextBuffer() {
        for (int row = 0; row < LCD_ROWS; row++) {
            for (int col = 0; col < LCD_COLUMNS; col++) {
                currentText[row][col] = ' ';
            }
            currentText[row][LCD_COLUMNS] = '\0'; // Null-terminate each row
        }
    }
};

// Global LCD control object
LCDControl lcd;

// Checks if the password is already set in the EEPROM
bool isPasswordSet() {
    for (int i = 0; i < 4; i++) {
        if (EEPROM.read(i) != currentPassword[i]) {
            return true;
        }
    }
    return false;
}

// Stores the password in EEPROM
void storePasswordInEEPROM(const char *password) {
    for (int i = 0; i < 4; i++) {
        EEPROM.write(i, password[i]);
    }
}

// Retrieves user input from the keypad with optional masking
String getUserInput(String prompt, int maxLength, bool hideInput, bool newLine) {
    lcd.print(prompt);
    if (newLine)
        lcd.nextLine();

    String input = "";
    while (true) {
        char key = keypad.getKey();
        if (!key)
            continue;

        if (isdigit(key) && input.length() < maxLength) {
            input += key;
            lcd.print(hideInput ? "*" : String(key));
        } else if (key == '#') {
            int countOfCharToRemove = input.length() + prompt.length();
            for (int i = 0; i < countOfCharToRemove; i++) {
                lcd.removeLastCharacter();
            }
            return input;
        } else if (key == '*' && input.length() > 0) {
            input.remove(input.length() - 1);
            lcd.removeLastCharacter();
        }
    }
}

// Calculates the timer value in microseconds from hours, minutes, and seconds
unsigned long calculateTimerValue(unsigned long h, unsigned long m, unsigned long s) {
    int timescale = 2;
    return (h * 3600 + m * 60 + s) * 1000000UL / timescale;
}

// Resets and reinitializes Timer1 with the specified interval and callback
void resetTimer1(unsigned long timerInterval, void (*timerCallback)()) {
    Timer1.detachInterrupt();
    Timer1.initialize(timerInterval);
    Timer1.attachInterrupt(timerCallback);
}

// Handles the timer interrupt event for toggling the device state
void timerEvent() {
    if (!isTimerOn) {
        digitalWrite(DEVICE_PIN, HIGH);           // Turn device ON
        resetTimer1(timeIntervalOff, timerEvent); // Switch to OFF timer
        isTimerOn = true;
    } else {
        digitalWrite(DEVICE_PIN, LOW);           // Turn device OFF
        resetTimer1(timeIntervalOn, timerEvent); // Switch to ON timer
        isTimerOn = false;
    }
}

// Verifies the entered password against the stored password in EEPROM
bool verifyPassword() {
    String pass = getUserInput("ENTER PASSWORD:", 4, true, false);

    for (int i = 0; i < 4; i++) {
        enteredPassword[i] = pass[i];
    }
    enteredPassword[4] = '\0';

    for (int i = 0; i < 4; i++) {
        if (EEPROM.read(i) != enteredPassword[i]) {
            return false;
        }
    }
    return true;
}

// Changes the stored password to a new password
void changePassword() {
    if (!verifyPassword()) {
        lcd.print("INVALID PASSWORD");
        delay(1500);
        lcd.clear();
        return;
    }

    String newPass = getUserInput("NEW PASSWORD:", 4, true, false);

    for (int i = 0; i < 4; i++) {
        currentPassword[i] = newPass[i];
    }
    storePasswordInEEPROM(currentPassword);
    lcd.print("PASSWORD CHANGED");
    delay(1000);
}

// Displays a welcome message on the LCD
void showWelcomeMessage() {
    lcd.print("WELCOME!");
    delay(1000);
}

// Displays the main menu and waits for the user to select an option
int showMainMenu() {
    lcd.print("SELECT OPTION");

    lcd.nextLine();
    lcd.print("1) TIMER ON");

    lcd.nextLine();
    lcd.print("2) TIMER OFF");

    lcd.nextLine();
    lcd.print("3) CHANGE PASS");

    while (true) {
        char key = keypad.getKey();
        if (key == '1')
            return 1;
        if (key == '2')
            return 2;
        if (key == '3')
            return 3;
    }
}

// Sets the timer interval for turning the device on
void setTimerOn() {
    lcd.print("    SET TIMER ON");
    lcd.setCursor(0, lcd.getCurrentRow() + 1);

    int hours = getUserInput("HOURS: ", 2, false, false).toInt();
    int minutes = getUserInput("MINUTES: ", 2, false, false).toInt();
    int seconds = getUserInput("SECONDS: ", 2, false, false).toInt();

    timeIntervalOn = calculateTimerValue(hours, minutes, seconds);
}

// Sets the timer interval for turning the device off
void setTimerOff() {
    lcd.print("    SET TIMER OFF");
    lcd.setCursor(0, lcd.getCurrentRow() + 1);

    int hours = getUserInput("HOURS: ", 2, false, false).toInt();
    int minutes = getUserInput("MINUTES: ", 2, false, false).toInt();
    int seconds = getUserInput("SECONDS: ", 2, false, false).toInt();

    timeIntervalOff = calculateTimerValue(hours, minutes, seconds);
}

// Updates the timer with the current ON and OFF intervals
void updateTimer() {
    if (timeIntervalOn > 0 && timeIntervalOff > 0) {
        resetTimer1(timeIntervalOn, timerEvent);
    }
}

// Initializes the system and prepares it for operation
void setup() {
    lcd.begin();
    Serial.begin(9600);
    pinMode(DEVICE_PIN, OUTPUT);
    digitalWrite(DEVICE_PIN, LOW);

    if (!isPasswordSet()) {
        storePasswordInEEPROM(currentPassword);
    }
}

// Main loop to verify password and manage user interactions
void loop() {
    if (!verifyPassword()) {
        lcd.print("INVALID PASSWORD");
        delay(1500);
        lcd.clear();
        return;
    }

    showWelcomeMessage();

    while (true) {
        lcd.clear();
        int option = showMainMenu();
        lcd.clear();
        if (option == 1) {
            setTimerOn();
            updateTimer();
        }
        if (option == 2) {
            setTimerOff();
            updateTimer();
        }
        if (option == 3)
            changePassword();
    }
}