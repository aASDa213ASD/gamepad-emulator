#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <thread>
#include <chrono>

#include "keys.h"

/*
    Tried to hide the cursor, didn't quite work.
    if (keyboard_event.code == KEY_PAUSE && keyboard_event.value == 1) {
        this->cursor_hide = !this->cursor_hide;

        if (this->cursor_hide) {
            this->move_mouse_to(9999, 9999);
        }
        else {
            this->move_mouse_to(-512, -256);
        }

        printf("cursor_hide is %d\n",this->cursor_hide);
    }

    if (keyboard_event.code == KEY_SCROLLLOCK && keyboard_event.value == 1) {
        this->move_mouse_to(-512, -256);

        printf("Trying to restore mouse, move further...\n");
    }
*/

/*
    The path to the keyboard and mouse (should look something like /dev/input/eventX)
    sudo libinput list-devices

    SONiX USB DEVICE => /dev/input/event2
    Logitech G102 LIGHTSYNC Gaming Mouse => /dev/input/event5
*/

/*
struct MousePosition {
    int x;
    int y;
};
*/

class Gamepad {
private:
    const char* gamepad_name = "Gamepad";
    bool pause = true;
    bool bomb_impact = false;
    std::chrono::high_resolution_clock::time_point destroy_timestamp;
    int destroy_flag = 0;

public:
    const char* mouse_path = "/dev/input/event5";
    const char* keyboard_path = "/dev/input/event2";
    int rcode = 0;
    int mouse_fd = 0;
    int keyboard_fd = 0;
    int gamepad_fd = 0;

    struct input_event keyboard_event;
    struct input_event mouse_event;
    struct input_event gamepad_ev;
    struct uinput_user_dev uidev;

    Gamepad() {
        this->destroy();
        this->hook();
        this->create();
    }

    void hook() {
        printf("Hooking pereferals ...\n");

        char keyboard_name[256] = "Unnamed";
        int keyboard_fd = open(this->keyboard_path, O_RDONLY | O_NONBLOCK);
        if (keyboard_fd == -1)
        {
            printf("Failed to open keyboard.\n");
            exit(1);
        }
        this->rcode = ioctl(keyboard_fd, EVIOCGNAME(sizeof(keyboard_name)), keyboard_name);
        printf("\t%s hooked.\n", keyboard_name);

        char mouse_name[256] = "Unnamed";
        this->mouse_fd = open(this->mouse_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (this->mouse_fd == -1)
        {   
            printf("Failed on mouse hook.\n");
            exit(1);
        }

        this->rcode = ioctl(this->mouse_fd, EVIOCGNAME(sizeof(mouse_name)), mouse_name);
        printf("\t%s hooked.\n", mouse_name);
    }

    int create() {
        /* 
            Perhaps there's a better and more elegant way
            to setup this 
        */
        printf("Creating Gamepad device ...\n");

        this->gamepad_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (this->gamepad_fd < 0)
        {
            printf("Opening of input failed! \n");
            return 1;
        }

        // Setting gamepad keys <"keys.h">
        ioctl(this->gamepad_fd, UI_SET_EVBIT, EV_KEY);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_A);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_B);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_X);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_Y);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_TL);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_TR);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_TL2);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_TR2);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_START);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_SELECT);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_THUMBL);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_THUMBR);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_DPAD_UP);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_DPAD_DOWN);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_DPAD_LEFT);
        ioctl(this->gamepad_fd, UI_SET_KEYBIT, BTN_DPAD_RIGHT);

        // Setting gamepad thumbsticks <"keys.h">
        ioctl(this->gamepad_fd, UI_SET_EVBIT, EV_ABS);
        ioctl(this->gamepad_fd, UI_SET_ABSBIT, ABS_X);
        ioctl(this->gamepad_fd, UI_SET_ABSBIT, ABS_Y);
        ioctl(this->gamepad_fd, UI_SET_ABSBIT, ABS_RX);
        ioctl(this->gamepad_fd, UI_SET_ABSBIT, ABS_RY);
        ioctl(this->gamepad_fd, UI_SET_ABSBIT, ABS_TILT_X);
        ioctl(this->gamepad_fd, UI_SET_ABSBIT, ABS_TILT_Y);

        memset(&uidev, 0, sizeof(uidev));
        snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, this->gamepad_name);
        uidev.id.bustype = BUS_USB;
        uidev.id.vendor = 0x3;
        uidev.id.product = 0x3;
        uidev.id.version = 2;

        // Thumbsticks parameters
        uidev.absmax[ABS_X] = 32767; 
        uidev.absmin[ABS_X] = -32768;
        uidev.absfuzz[ABS_X] = 0;
        uidev.absflat[ABS_X] = 15;

        uidev.absmax[ABS_Y] = 32767;
        uidev.absmin[ABS_Y] = -32768;
        uidev.absfuzz[ABS_Y] = 0;
        uidev.absflat[ABS_Y] = 15;

        uidev.absmax[ABS_RX] = 512;
        uidev.absmin[ABS_RX] = -512;
        uidev.absfuzz[ABS_RX] = 0;
        uidev.absflat[ABS_RX] = 16;

        uidev.absmax[ABS_RY] = 512;
        uidev.absmin[ABS_RY] = -512;
        uidev.absfuzz[ABS_RY] = 0;
        uidev.absflat[ABS_RY] = 16;

        if (write(this->gamepad_fd, &uidev, sizeof(uidev)) < 0) {
            printf("Failed on uidev write.\n");
            return 1;
        }

        if (ioctl(this->gamepad_fd, UI_DEV_CREATE) < 0) {
            printf("Failed on gamepad creation.\n");
            return 1;
        }

        printf("Ready.\n");
        return 0;
    }
    
    void send_sync_event(struct input_event gamepad_event) {
        memset(&gamepad_event, 0, sizeof(struct input_event));
        gamepad_event.type = EV_SYN;
        gamepad_event.code = 0;
        gamepad_event.value = 0;

        if(write(this->gamepad_fd, &gamepad_event, sizeof(struct input_event)) < 0)
            printf("Error: Couldn't write sync event.\n");
    }

    void send_mouse_event(struct input_event gamepad_event, int TYPE, int CODE, int VALUE) {
        memset(&gamepad_event, 0, sizeof(struct input_event));
        gamepad_event.type = TYPE;
        gamepad_event.code = CODE;
        gamepad_event.value = VALUE;

        if(write(this->gamepad_fd, &gamepad_event, sizeof(struct input_event)) < 0)
            printf("Error: Couldn't write mouse event to gamepad.\n");
    }

    // TYPE is the event to write to the gamepad and CODE is an integer value for the button on the gamepad 
    void send_keyboard_event(struct input_event gamepad_event, int TYPE, int CODE, int VALUE) {
        memset(&gamepad_event, 0, sizeof(struct input_event));
        gamepad_event.type = TYPE;
        gamepad_event.code = CODE;
        gamepad_event.value = VALUE;

        if(write(this->gamepad_fd, &gamepad_event, sizeof(struct input_event)) < 0)
            printf("Error: Couldn't write keyboard event to gamepad.\n");
    }

    void write_mouse_event(struct input_event mouse_event) {
        if(write(this->mouse_fd, &mouse_event, sizeof(struct input_event)) < 0)
            printf("Error: Couldn't write mouse event.\n");
    }

    void move_mouse_to(int x, int y) {
        write_mouse_event({{}, EV_REL, REL_X, x});
        write_mouse_event({{}, EV_REL, REL_Y, y});
        write_mouse_event({{}, EV_SYN, SYN_REPORT, 0});
    }

    void run() {
        int accumulatedMouseX = 0;
        int accumulatedMouseY = 0;
        const float sensitivity = 4.0;

        printf("Listening.\n");
        while(true) {
            if (int sz = read(this->mouse_fd, &mouse_event, sizeof(struct input_event))) {
                if (sz != -1 && !this->pause) {
                    // printf("Mouse event: %d %d %d \n", mouse_event.code, mouse_event.type, mouse_event.value);
                    switch (mouse_event.type) {
                        case EV_REL:
                            if (mouse_event.code == REL_X) {
                                accumulatedMouseX += mouse_event.value;
                                //int toWrite = mouse_event.value * 299;
                                float toWrite = accumulatedMouseX * sensitivity;

                                // send mouse event to gamepad
                                send_mouse_event(this->gamepad_ev, EV_ABS, ABS_RX, toWrite);

                                // send sync event
                                send_sync_event(this->gamepad_ev);
                            }

                            if (mouse_event.code == REL_Y) {
                                accumulatedMouseY += mouse_event.value;
                                //int toWrite = mouse_event.value * 299;
                                float toWrite = accumulatedMouseY * sensitivity;

                                // send mouse event to gamepad
                                send_mouse_event(this->gamepad_ev, EV_ABS, ABS_RY, toWrite);

                                // send sync event
                                send_sync_event(this->gamepad_ev);
                            }
                            break;
                        
                        case EV_KEY:
                            if (mouse_event.code == BTN_LEFT) {
                                // send mouse event to gamepad
                                send_mouse_event(this->gamepad_ev, EV_KEY, BTN_Y, mouse_event.value);
                                // send sync event
                                send_sync_event(this->gamepad_ev);
                            }

                            // Mouse [RMB] to Gamepad [ZR]
                            if (mouse_event.code == BTN_RIGHT) {
                                // send mouse event to gamepad
                                send_mouse_event(this->gamepad_ev, EV_KEY, BTN_TL2, mouse_event.value);
                                
                                // send sync event
                                send_sync_event(this->gamepad_ev);
                            }

                            // Mouse [MMD] to Gamepad [ZL]
                            if (mouse_event.code == BTN_MIDDLE) {
                                // send mouse event to gamepad
                                send_mouse_event(this->gamepad_ev, EV_KEY, BTN_TR2, mouse_event.value);
                                
                                // send sync event
                                send_sync_event(this->gamepad_ev);
                            }

                            // Reset controller state
                            if (mouse_event.code == BTN_SIDE) {
                                printf("Resetting controller state ...\n");
                                
                                accumulatedMouseX = 0;
                                accumulatedMouseY = 0;

                                // send mouse event to gamepad
                                send_mouse_event(this->gamepad_ev, EV_ABS, ABS_RX, 0);

                                // send mouse event to gamepad
                                send_mouse_event(this->gamepad_ev, EV_ABS, ABS_RY, 0);

                                // send one sync event for both axes
                                send_sync_event(this->gamepad_ev);
                            }
                            break;
                    }
                }
            }

            if (read(this->keyboard_fd, &keyboard_event, sizeof(keyboard_event)) != -1) {
                //printf("keyboard event: type %d code %d value %d  \n", keyboard_event.type, keyboard_event.code, keyboard_event.value);
                /* :::::: Custom :::::: */

                if (keyboard_event.code == KEY_CAPSLOCK && keyboard_event.value == 1) {
                    this->pause = !this->pause;
                }
                
                if (!this->pause) {
                    if (keyboard_event.code == KEY_BACKSPACE && keyboard_event.value == 1) {
                        auto current_time = std::chrono::high_resolution_clock::now();
                        auto time_difference = std::chrono::duration_cast<std::chrono::seconds>(current_time - this->destroy_timestamp).count();
                        if (time_difference > 1)
                            this->destroy_flag = 0;
                        
                        this->destroy_flag++;
                        if (this->destroy_flag >= 5)
                            printf("[Gamepad] Press [ENTER] to destroy gamepad.\n");

                        this->destroy_timestamp = current_time;
                    }

                    if (keyboard_event.code == KEY_ENTER && keyboard_event.value == 1 && this->destroy_flag >= 5) {
                        auto current_time = std::chrono::high_resolution_clock::now();
                        auto time_difference = std::chrono::duration_cast<std::chrono::seconds>(current_time - this->destroy_timestamp).count();
                        if (time_difference > 1)
                            continue;

                        this->destroy();
                        break;
                    }
                    if (keyboard_event.code == KEY_T && keyboard_event.value == 1) {
                        this->bomb_impact = !this->bomb_impact;
                    }

                    if (keyboard_event.code == KEY_T && keyboard_event.value != 2 && this->bomb_impact) {
                        printf("[Gamepad] Bomb Impacting through the Hyrule ...\n");
                        // Move forward
                        send_keyboard_event(this->gamepad_ev, EV_ABS, ABS_Y, -32768);
                        send_sync_event(this->gamepad_ev);
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        send_keyboard_event(this->gamepad_ev, EV_ABS, ABS_Y, 0);
                        send_sync_event(this->gamepad_ev);

                        // Jump
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_X, keyboard_event.value);
                        send_sync_event(this->gamepad_ev);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_X, 0);
                        send_sync_event(this->gamepad_ev);

                        // Spawn round bomb
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_TL, keyboard_event.value);
                        send_sync_event(this->gamepad_ev);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_TL, 0);
                        send_sync_event(this->gamepad_ev);

                        // Aim with bow
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        send_mouse_event(this->gamepad_ev, EV_KEY, BTN_TR2, keyboard_event.value);
                        send_sync_event(this->gamepad_ev);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        send_mouse_event(this->gamepad_ev, EV_KEY, BTN_TR2, 0);
                        send_sync_event(this->gamepad_ev);
                        
                        // Switch to square bomb
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_DPAD_UP, 1);
                        send_sync_event(this->gamepad_ev);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_TR, 1);
                        send_sync_event(this->gamepad_ev);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_TR, 0);
                        send_sync_event(this->gamepad_ev);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_DPAD_UP, 0);
                        send_sync_event(this->gamepad_ev);

                        // Spawn square bomb
                        std::this_thread::sleep_for(std::chrono::milliseconds(500));
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_TL, keyboard_event.value);
                        send_sync_event(this->gamepad_ev);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_TL, 0);
                        send_sync_event(this->gamepad_ev);
                        
                        // Switch back to round bomb
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_DPAD_UP, 1);
                        send_sync_event(this->gamepad_ev);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_TL, 1);
                        send_sync_event(this->gamepad_ev);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_TL, 0);
                        send_sync_event(this->gamepad_ev);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_DPAD_UP, 0);
                        send_sync_event(this->gamepad_ev);

                        // [Blow the round bomb] Disabled so you can select the timing by urself.
                        /*
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        printf("[Bomb Impact] Bombing...\n");
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_TL, keyboard_event.value);
                        send_sync_event(this->gamepad_ev);
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_TL, 0);
                        send_sync_event(this->gamepad_ev);
                        */
                        this->bomb_impact = false;
                    }


                    /* :::::: General :::::: */

                    // Keyboard [E] to Gamepad [A]
                    if (keyboard_event.code == KEY_E && keyboard_event.value != 2) {
                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_A, keyboard_event.value);
                        
                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }

                    // Keyboard [LEFTSHIFT] to Gamepad [B]
                    if (keyboard_event.code == KEY_LEFTSHIFT && keyboard_event.value != 2) {
                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_B, keyboard_event.value);

                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }

                    // Keyboard [SPACE] to Gamepad [X]
                    if (keyboard_event.code == KEY_SPACE && keyboard_event.value != 2) {
                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_X, keyboard_event.value);

                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }

                    // Mouse [LMB] to Gamepad [Y] => Look this->mouse_fd
                    
                    // Keyboard [LEFTCTRL] to Gamepad [L]
                    if (keyboard_event.code == KEY_LEFTCTRL) {
                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_THUMBL, keyboard_event.value);

                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }

                    // Keyboard [RIGHTCTRL] to Gamepad [R]
                    if (keyboard_event.code == KEY_RIGHTCTRL) {
                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_THUMBR, keyboard_event.value);

                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }

                    // Keyboard [Q] to Gamepad [LEFTSHOULDER]
                    if (keyboard_event.code == KEY_Q) {
                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_TL, keyboard_event.value);

                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }

                    // Keyboard [R] to Gamepad [RIGHTSHOULDER]
                    if (keyboard_event.code == KEY_R) {
                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_TR, keyboard_event.value);

                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }

                    // Keyboard [LEFTALT] to Gamepad [START], ~ is KEY_GRAVE
                    if (keyboard_event.code == KEY_LEFTALT && keyboard_event.value != 2) {
                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_START, keyboard_event.value);
                        
                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }

                    // Keyboard [M] to Gamepad [SELECT]
                    if (keyboard_event.code == KEY_M && keyboard_event.value != 2) {
                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_SELECT, keyboard_event.value);
                        
                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }

                    // Reset view joystick on left control
                    if (keyboard_event.code == KEY_LEFTCTRL) {
                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_ABS, ABS_RX, 0);

                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_ABS, ABS_RY, 0);

                        // send one sync event for both axes
                        send_sync_event(this->gamepad_ev);
                    }

                    /* :::::: Left Axis :::::: */

                    // Keyboard [W] to  Gamepad [Y axis] move up (forward) full speed
                    if (keyboard_event.code == KEY_W) {
                        int toWrite = 0;
                        if (keyboard_event.value == 2 || keyboard_event.value == 1) // on pressed or on hold key
                            toWrite = -32768;

                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_ABS, ABS_Y, toWrite);

                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }

                    // Keyboard [S] to Gamepad [Y] move down (backwards) full speed
                    if (keyboard_event.code == KEY_S) {
                        int toWrite = 0;
                        if (keyboard_event.value == 2 || keyboard_event.value == 1) // on pressed or on hold key
                            toWrite = 32767;

                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_ABS, ABS_Y, toWrite);

                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }
                    
                    // Keyboard [A] to Gamepad [X] move left full speed
                    if (keyboard_event.code == KEY_A) {
                        int toWrite = 0;
                        if (keyboard_event.value == 2 || keyboard_event.value == 1) // on pressed or on hold key
                            toWrite = -32768;

                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_ABS, ABS_X, toWrite);

                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }

                    // Keyboard [D] to Gamepad [X] move right full speed
                    if (keyboard_event.code == KEY_D) {
                        int toWrite = 0;
                        if (keyboard_event.value == 2 || keyboard_event.value == 1) // on pressed or on hold key
                            toWrite = 32767;

                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_ABS, ABS_X, toWrite);

                        // send sync 
                        send_sync_event(this->gamepad_ev);
                    }

                    /* :::::: Right Axis => Look this->mouse_fd :::::: */ 

                    /* :::::: D-Pad :::::: */

                    // Z to _
                    if (keyboard_event.code == KEY_Z) {
                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_DPAD_UP, keyboard_event.value);

                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }
                    
                    // X to _
                    if (keyboard_event.code == KEY_X) {
                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_DPAD_DOWN, keyboard_event.value);

                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }

                    // C to _
                    if (keyboard_event.code == KEY_C) {
                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_DPAD_LEFT, keyboard_event.value);

                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }

                    // V to _
                    if (keyboard_event.code == KEY_V) {
                        //send keyboard event to gamepad
                        send_keyboard_event(this->gamepad_ev, EV_KEY, BTN_DPAD_RIGHT, keyboard_event.value);

                        // send sync event
                        send_sync_event(this->gamepad_ev);
                    }
                }
            }
        }

        printf("Breaking the listener loop ...\n");
        this->rcode = ioctl(this->keyboard_fd, EVIOCGRAB, 1);
        close(this->keyboard_fd);
        this->rcode = ioctl(this->mouse_fd, EVIOCGRAB, 1);
        close(this->mouse_fd);
    }

    void destroy()
    {
        printf("Destroying gamepad ...\n");
        close(this->gamepad_fd);
        //if (ioctl(this->gamepad_fd, UI_DEV_DESTROY) < 0)
        //   printf("Error destroying gamepad! \n");
    }

};

int main(int argc, char *argv[])
{
    Gamepad gamepad;
    gamepad.run();

    return 0;
}