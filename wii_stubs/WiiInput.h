/*
 * Wii Input Handler - WPAD controller support
 */

#ifndef WIIINPUT_H
#define WIIINPUT_H

#include <wiiuse/wpad.h>
#include <gccore.h>
#include <vector>
#include <string>

namespace WiiInput
{
    struct ControllerState
    {
        WPADData wpad;
        
        float stickX;
        float stickY;
        float stickAngle;
        float stickMag;
        
        float accelX;
        float accelY;
        float accelZ;
        
        float irX;
        float irY;
        bool irValid;
        
        uint32_t buttons;
        uint32_t buttonsDown;
        uint32_t buttonsUp;
        
        ControllerState() : stickX(0), stickY(0), stickAngle(0), stickMag(0),
                          accelX(0), accelY(0), accelZ(0), irX(0), irY(0), irValid(false),
                          buttons(0), buttonsDown(0), buttonsUp(0) {}
    };
    
    class InputManager
    {
    public:
        static InputManager& getInstance();
        
        void init();
        void update();
        
        bool isButtonPressed(uint32_t controller, uint32_t button);
        bool isButtonJustPressed(uint32_t controller, uint32_t button);
        bool isButtonJustReleased(uint32_t controller, uint32_t button);
        
        float getStickX(uint32_t controller);
        float getStickY(uint32_t controller);
        float getStickAngle(uint32_t controller);
        float getStickMagnitude(uint32_t controller);
        
        float getAccelerometerX(uint32_t controller);
        float getAccelerometerY(uint32_t controller);
        float getAccelerometerZ(uint32_t controller);
        
        bool isIRValid(uint32_t controller);
        float getIRX(uint32_t controller);
        float getIRY(uint32_t controller);
        
        void rumble(uint32_t controller, bool on);
        
        enum Buttons
        {
            BTN_A = 0x0001,
            BTN_B = 0x0002,
            BTN_1 = 0x0004,
            BTN_2 = 0x0008,
            BTN_MINUS = 0x0010,
            BTN_HOME = 0x0100,
            BTN_LEFT = 0x0200,
            BTN_RIGHT = 0x0400,
            BTN_DOWN = 0x0800,
            BTN_UP = 0x1000,
            BTN_PLUS = 0x2000,
            
            NUNCHUK_BTN_C = 0x0004,
            NUNCHUK_BTN_Z = 0x0001,
            
            CLASSIC_BTN_A = 0x0001,
            CLASSIC_BTN_B = 0x0002,
            CLASSIC_BTN_X = 0x0004,
            CLASSIC_BTN_Y = 0x0008,
            CLASSIC_BTN_L = 0x0010,
            CLASSIC_BTN_R = 0x0020,
            CLASSIC_BTN_ZL = 0x0040,
            CLASSIC_BTN_ZR = 0x0080,
            CLASSIC_BTN_MINUS = 0x0100,
            CLASSIC_BTN_HOME = 0x0200,
            CLASSIC_BTN_PLUS = 0x0400,
            CLASSIC_BTN_LEFT = 0x0800,
            CLASSIC_BTN_RIGHT = 0x1000,
            CLASSIC_BTN_DOWN = 0x2000,
            CLASSIC_BTN_UP = 0x4000,
        };
        
    private:
        InputManager();
        ~InputManager();
        
        ControllerState mControllers[4];
        uint32_t mPrevButtons[4];
    };
    
    void Init();
    void Update();
}

#endif
