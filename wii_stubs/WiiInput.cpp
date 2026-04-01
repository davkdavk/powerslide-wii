#include "WiiInput.h"
#include <cstring>

namespace WiiInput
{
    InputManager& InputManager::getInstance()
    {
        static InputManager instance;
        return instance;
    }
    
    InputManager::InputManager()
    {
        memset(mPrevButtons, 0, sizeof(mPrevButtons));
    }
    
    InputManager::~InputManager()
    {
    }
    
    void InputManager::init()
    {
#ifdef WII
        // Skip WPAD init in Dolphin - causes crash without real Wiimotes.
        // Input will be added later.
#endif
    }
    
    void InputManager::update()
    {
#ifdef WII
        // WPAD scanning disabled for Dolphin stability until real hardware pass.
        memset(mControllers, 0, sizeof(mControllers));
        memset(mPrevButtons, 0, sizeof(mPrevButtons));
        return;
#endif
        
        for (int i = 0; i < 4; i++) {
            WPADData* data = WPAD_Data(i);
            if (data) {
                mControllers[i].buttons = data->btns_h;
                mControllers[i].buttonsDown = data->btns_d;
                mControllers[i].buttonsUp = mPrevButtons[i] & ~data->btns_d;
                mPrevButtons[i] = data->btns_h;
                
                if (data->exp.type == WPAD_EXP_NUNCHUK) {
                    mControllers[i].stickX = data->exp.nunchuk.js.pos.x / 128.0f;
                    mControllers[i].stickY = data->exp.nunchuk.js.pos.y / 128.0f;
                    mControllers[i].stickMag = data->exp.nunchuk.js.mag;
                    mControllers[i].stickAngle = data->exp.nunchuk.js.ang;
                }
            }
        }
    }
    
    bool InputManager::isButtonPressed(uint32_t controller, uint32_t button)
    {
        if (controller >= 4) return false;
        return (mControllers[controller].buttons & button) != 0;
    }
    
    bool InputManager::isButtonJustPressed(uint32_t controller, uint32_t button)
    {
        if (controller >= 4) return false;
        return (mControllers[controller].buttonsDown & button) != 0;
    }
    
    bool InputManager::isButtonJustReleased(uint32_t controller, uint32_t button)
    {
        if (controller >= 4) return false;
        return (mControllers[controller].buttonsUp & button) != 0;
    }
    
    float InputManager::getStickX(uint32_t controller)
    {
        if (controller >= 4) return 0;
        return mControllers[controller].stickX;
    }
    
    float InputManager::getStickY(uint32_t controller)
    {
        if (controller >= 4) return 0;
        return mControllers[controller].stickY;
    }
    
    float InputManager::getStickAngle(uint32_t controller)
    {
        if (controller >= 4) return 0;
        return mControllers[controller].stickAngle;
    }
    
    float InputManager::getStickMagnitude(uint32_t controller)
    {
        if (controller >= 4) return 0;
        return mControllers[controller].stickMag;
    }
    
    float InputManager::getAccelerometerX(uint32_t controller)
    {
        if (controller >= 4) return 0;
        return mControllers[controller].accelX;
    }
    
    float InputManager::getAccelerometerY(uint32_t controller)
    {
        if (controller >= 4) return 0;
        return mControllers[controller].accelY;
    }
    
    float InputManager::getAccelerometerZ(uint32_t controller)
    {
        if (controller >= 4) return 0;
        return mControllers[controller].accelZ;
    }
    
    bool InputManager::isIRValid(uint32_t controller)
    {
        if (controller >= 4) return false;
        return mControllers[controller].irValid;
    }
    
    float InputManager::getIRX(uint32_t controller)
    {
        if (controller >= 4) return 0;
        return mControllers[controller].irX;
    }
    
    float InputManager::getIRY(uint32_t controller)
    {
        if (controller >= 4) return 0;
        return mControllers[controller].irY;
    }
    
    void InputManager::rumble(uint32_t controller, bool on)
    {
        if (controller >= 4) return;
#ifdef WII
        (void)on;
        return;
#endif
        WPAD_Rumble(controller, on ? 1 : 0);
    }
    
    void Init()
    {
        InputManager::getInstance().init();
    }
    
    void Update()
    {
        InputManager::getInstance().update();
    }
}
