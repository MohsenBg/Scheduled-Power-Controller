# Scheduled Power Controller

This project is an Arduino-based system designed to control the power of a device based on a timer. The user can set a specific time interval for the device to be on and off using a keypad for password input. The project uses an LCD screen to display options and statuses. The system also allows for changing the password stored in EEPROM.

## Features

- **Keypad Authentication:** A password is required to access the system and configure settings.
- **Timer Control:** The user can set the device to turn on and off based on a configurable timer.
- **LCD Display:** The system uses an I2C LCD to display status messages and input prompts.
- **EEPROM Storage:** The password is stored in the EEPROM, and users can change it as needed.

## Components Required

- Arduino board (e.g., Arduino Uno)
- 20x4 LCD (I2C)
- 4x3 Keypad
- Relay Module (to control the device)
- Jumper wires
- Breadboard (optional)

## Libraries Used

- `EEPROM`: To store and retrieve the password.
- `Keypad`: To handle keypad input.
- `LiquidCrystal_I2C`: To interact with the LCD.
- `TimerOne`: To manage timed events.
- `Wire`: For I2C communication.

## Pin Configuration

- **Device Pin (Relay)**: Pin 3 (controls the device's power)
- **Keypad**: Uses pins 4, 5, 6, 7 (rows) and 11, 12, 13 (columns)
- **LCD**: I2C connection (configured as `0x20`)

## Setup

1. Connect the LCD to the Arduino using the I2C interface.
2. Connect the keypad to the defined pins (4-7 for rows, 11-13 for columns).
3. Connect the relay to the device you want to control (e.g., a fan, light).
4. Upload the provided code to your Arduino board.

## Functionality

- **Password Verification:** The system will prompt for a password before granting access to configure the timers. The default password is `0000`. If the password is incorrect, the system will display an "INVALID PASSWORD" message.
- **Timer Settings:** The user can set the device to turn on and off at specified intervals (hours, minutes, and seconds).
- **Main Menu:**
  - Option 1: Set the "ON" timer.
  - Option 2: Set the "OFF" timer.
  - Option 3: Change the password.

## How to Use

1. After uploading the code to the Arduino, the LCD will display the welcome message.
2. Enter the password when prompted.
3. Once authenticated, the main menu will appear, offering options to set the timer, turn the device on or off, or change the password.
4. Use the keypad to navigate through the options and set the timers or change the password as needed.

