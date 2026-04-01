#ifndef OIS_INPUT_COMPAT_H
#define OIS_INPUT_COMPAT_H

#include <map>
#include <string>
#include <stdint.h>

namespace OIS
{
    typedef std::map<std::string, std::string> ParamList;

    typedef int ObjectType;
    enum TypeIds {
        OISUnknown = 0,
        OISKeyboard = 1,
        OISMouse = 2,
        OISJoyStick = 3,
        OISTablet = 4,
        OISOther = 5,
        OISMultiTouch = 6
    };

    class Object
    {
    public:
        virtual ~Object() {}
    };

    enum MouseButtonID
    {
        MB_Left = 0,
        MB_Right = 1,
        MB_Middle = 2,
        MB_Button3 = 3,
        MB_Button4 = 4,
        MB_Button5 = 5,
        MB_Button6 = 6,
        MB_Button7 = 7
    };

    enum KeyCode
    {
        KC_UNASSIGNED = 0x00,
        KC_ESCAPE = 0x01,
        KC_1 = 0x02,
        KC_2 = 0x03,
        KC_3 = 0x04,
        KC_4 = 0x05,
        KC_5 = 0x06,
        KC_6 = 0x07,
        KC_7 = 0x08,
        KC_8 = 0x09,
        KC_9 = 0x0A,
        KC_0 = 0x0B,
        KC_MINUS = 0x0C,
        KC_EQUALS = 0x0D,
        KC_BACK = 0x0E,
        KC_TAB = 0x0F,
        KC_Q = 0x10,
        KC_W = 0x11,
        KC_E = 0x12,
        KC_R = 0x13,
        KC_T = 0x14,
        KC_Y = 0x15,
        KC_U = 0x16,
        KC_I = 0x17,
        KC_O = 0x18,
        KC_P = 0x19,
        KC_RETURN = 0x1C,
        KC_NUMPADENTER = 0x9C,
        KC_LCONTROL = 0x1D,
        KC_A = 0x1E,
        KC_S = 0x1F,
        KC_D = 0x20,
        KC_F = 0x21,
        KC_G = 0x22,
        KC_H = 0x23,
        KC_J = 0x24,
        KC_K = 0x25,
        KC_L = 0x26,
        KC_LSHIFT = 0x2A,
        KC_Z = 0x2C,
        KC_X = 0x2D,
        KC_C = 0x2E,
        KC_V = 0x2F,
        KC_B = 0x30,
        KC_N = 0x31,
        KC_M = 0x32,
        KC_RSHIFT = 0x36,
        KC_LMENU = 0x38,
        KC_SPACE = 0x39,
        KC_F1 = 0x3B,
        KC_F2 = 0x3C,
        KC_F3 = 0x3D,
        KC_F4 = 0x3E,
        KC_F5 = 0x3F,
        KC_F6 = 0x40,
        KC_F7 = 0x41,
        KC_F8 = 0x42,
        KC_F9 = 0x43,
        KC_F10 = 0x44,
        KC_F11 = 0x57,
        KC_F12 = 0x58,
        KC_GRAVE = 0x29,
        KC_HOME = 0xC7,
        KC_UP = 0xC8,
        KC_LEFT = 0xCB,
        KC_RIGHT = 0xCD,
        KC_END = 0xCF,
        KC_DOWN = 0xD0,
        KC_DELETE = 0xD3,
        KC_RCONTROL = 0x9D,
        KC_RMENU = 0xB8
    };

    struct Axis
    {
        int abs;
        int rel;
        Axis() : abs(0), rel(0) {}
    };

    struct MouseState
    {
        Axis X, Y, Z;
        mutable int width, height;
        uint32_t buttons;

        MouseState() : width(640), height(480), buttons(0) {}
    };

    class KeyEvent
    {
    public:
        KeyCode key;
        uint32_t text;
        bool state;
        KeyEvent() : key(KC_UNASSIGNED), text(0), state(false) {}
        KeyEvent(Object*, KeyCode k, uint32_t t) : key(k), text(t), state(false) {}
    };

    class MouseEvent
    {
    public:
        MouseState state;
    };

    class MultiTouchEvent : public MouseEvent
    {
    public:
        int touchId;
        MultiTouchEvent() : touchId(0) {}
    };

    class KeyListener
    {
    public:
        virtual ~KeyListener() {}
        virtual bool keyPressed(const KeyEvent& arg) = 0;
        virtual bool keyReleased(const KeyEvent& arg) = 0;
    };

    class MouseListener
    {
    public:
        virtual ~MouseListener() {}
        virtual bool mouseMoved(const MouseEvent& arg) = 0;
        virtual bool mousePressed(const MouseEvent& arg, MouseButtonID id) = 0;
        virtual bool mouseReleased(const MouseEvent& arg, MouseButtonID id) = 0;
    };

    class MultiTouchListener
    {
    public:
        virtual ~MultiTouchListener() {}
        virtual bool touchMoved(const MultiTouchEvent& arg) = 0;
        virtual bool touchPressed(const MultiTouchEvent& arg) = 0;
        virtual bool touchReleased(const MultiTouchEvent& arg) = 0;
    };

    class Keyboard : public Object
    {
    public:
        enum Modifier { Shift = 1, Ctrl = 2, Alt = 4 };
        Keyboard() : mListener(0) {}
        virtual ~Keyboard() {}
        virtual void setEventCallback(KeyListener* listener) { mListener = listener; }
        virtual bool isKeyDown(KeyCode key) const { (void)key; return false; }
        virtual void capture() {}
        virtual bool isModifierDown(Modifier mod) const { (void)mod; return false; }
    private:
        KeyListener* mListener;
    };

    class Mouse : public Object
    {
    public:
        Mouse() : mListener(0) {}
        virtual ~Mouse() {}
        virtual void setEventCallback(MouseListener* listener) { mListener = listener; }
        virtual void capture() {}
        virtual const MouseState& getMouseState() const { return mState; }
    private:
        MouseListener* mListener;
        MouseState mState;
    };

    class MultiTouch : public Object
    {
    public:
        MultiTouch() : mListener(0) {}
        virtual ~MultiTouch() {}
        virtual void setEventCallback(MultiTouchListener* listener) { mListener = listener; }
        virtual void capture() {}
        virtual const MouseState& getMouseState() const { return mState; }
    private:
        MultiTouchListener* mListener;
        MouseState mState;
    };

    class InputManager
    {
    public:
        static InputManager* createInputSystem(const std::map<std::string, std::string>& paramList)
        {
            (void)paramList;
            return new InputManager();
        }

        static InputManager* createInputSystem(size_t windowHnd)
        {
            (void)windowHnd;
            return new InputManager();
        }

        static void destroyInputSystem(InputManager* manager)
        {
            delete manager;
        }

        virtual ~InputManager() {}

        virtual Object* createInputObject(ObjectType type, bool bufferMode)
        {
            (void)bufferMode;
            if (type == OISKeyboard) return new Keyboard();
            if (type == OISMouse) return new Mouse();
            if (type == OISMultiTouch) return new MultiTouch();
            return 0;
        }

        virtual Keyboard* createInputObjectKeyboard(ObjectType type, bool bufferMode)
        {
            (void)type;
            (void)bufferMode;
            return new Keyboard();
        }

        virtual Mouse* createInputObjectMouse(ObjectType type, bool bufferMode)
        {
            (void)type;
            (void)bufferMode;
            return new Mouse();
        }

        virtual void destroyInputObject(Object* obj) { delete obj; }

        virtual bool getParam(const std::string& name, int& value) { (void)name; (void)value; return false; }
        virtual bool getParam(const std::string& name, std::string& value) { (void)name; (void)value; return false; }
    };
}

#endif
