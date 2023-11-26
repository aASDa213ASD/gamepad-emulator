> # Gamepad:
Controller emulator for `Linux`.

# Requirements:
| Prerequisite    | Description       |
| --------------- | ----------------- |
| `Linux x86_64`  | Operating System  |
| `GCC`           | Compiler          |

> # Build from source:
## Current method
`g++ -std=c++11 gamepad.cpp -o gamepad`

## Deprecated method
`gcc gamepad.cpp -o gamepad`

> # Usage:
Step 1: `sudo ./gamepad` to launch the emulator. <br>
Step 2: Activate <kbd>Capslock</kbd> to activate the input listener. <br>
Step 3: While <kbd>Capslock</kbd> is active press <kbd>Backspace</kbd> 5 times and finally <kbd>Enter</kbd> to destroy the gamepad. <br>

