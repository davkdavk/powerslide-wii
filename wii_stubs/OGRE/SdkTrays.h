#ifndef SDKTRAY_H
#define SDKTRAY_H

#include <string>
#include <vector>
#include "Ogre.h"
#include "../OIS/OISInputCompat.h"

namespace OgreBites
{
    enum TrayLocation
    {
        TL_NONE = 0,
        TL_TOPLEFT,
        TL_TOP,
        TL_TOPRIGHT,
        TL_CENTER,
        TL_LEFT,
        TL_RIGHT,
        TL_BOTTOMLEFT,
        TL_BOTTOM,
        TL_BOTTOMRIGHT
    };

    class Widget
    {
    public:
        Widget() : mVisible(true) {}
        virtual ~Widget() {}
        virtual void show() { mVisible = true; }
        virtual void hide() { mVisible = false; }
        virtual bool isVisible() const { return mVisible; }
        static bool isCursorOver(const void* widget, const OIS::MouseEvent& pos, int padding)
        {
            (void)widget;
            (void)pos;
            (void)padding;
            return false;
        }
        static bool isCursorOver(const void* widget, const Ogre::Vector2& pos, int padding)
        {
            (void)widget;
            (void)pos;
            (void)padding;
            return false;
        }
    private:
        bool mVisible;
    };

    class Label : public Widget
    {
    public:
        explicit Label(const Ogre::String& caption = "") : mCaption(caption) {}
        void setCaption(const Ogre::String& caption) { mCaption = caption; }
        const Ogre::String& getCaption() const { return mCaption; }
    private:
        Ogre::String mCaption;
    };

    class Button : public Widget
    {
    public:
        Button(const Ogre::String& name = "", const Ogre::String& caption = "") : mName(name), mCaption(caption) {}
        const Ogre::String& getName() const { return mName; }
        const Ogre::String& getCaption() const { return mCaption; }
    private:
        Ogre::String mName;
        Ogre::String mCaption;
    };

    class TextBox : public Widget
    {
    public:
        TextBox(const Ogre::String& name = "", const Ogre::String& caption = "") : mName(name), mCaption(caption) {}
        void setText(const Ogre::String& text) { mText = text; }
        const Ogre::String& getText() const { return mText; }
    private:
        Ogre::String mName;
        Ogre::String mCaption;
        Ogre::String mText;
    };

    class SelectMenu : public Widget
    {
    public:
        explicit SelectMenu(const Ogre::String& name = "") : mName(name), mSelectedIndex(0) {}
        void addItem(const std::string& item) { mItems.push_back(item); }
        void selectItem(int index) { mSelectedIndex = index; }
        void selectItem(const std::string& item)
        {
            for (size_t i = 0; i < mItems.size(); ++i) {
                if (mItems[i] == item) {
                    mSelectedIndex = static_cast<int>(i);
                    break;
                }
            }
        }
        int getSelectedIndex() const { return mSelectedIndex; }
    private:
        Ogre::String mName;
        std::vector<std::string> mItems;
        int mSelectedIndex;
    };

    class ParamsPanel : public Widget
    {
    public:
        explicit ParamsPanel(const Ogre::String& name = "") : mName(name) {}
        void setParamValue(const std::string& param, const std::string& value) { (void)param; (void)value; }
        void setCaption(const std::string& caption) { (void)caption; }
    private:
        Ogre::String mName;
    };

    class ProgressBar
    {
    public:
        void setCaption(const Ogre::String& caption) { (void)caption; }
        void setComment(const Ogre::String& comment) { (void)comment; }
        void setProgress(Ogre::Real progress) { mProgress = progress; }
        Ogre::Real getProgress() const { return mProgress; }
    private:
        Ogre::Real mProgress = 0.0f;
    };

    class InputContext
    {
    public:
        InputContext() : mKeyboard(0), mMouse(0) {}
        OIS::Keyboard* mKeyboard;
        OIS::Mouse* mMouse;
        void capture()
        {
            if (mKeyboard) mKeyboard->capture();
            if (mMouse) mMouse->capture();
        }
    };

    class SdkTrayListener
    {
    public:
        virtual ~SdkTrayListener() {}
    };

    class TrayListener : public SdkTrayListener
    {
    public:
        virtual ~TrayListener() {}
    };

    class SdkTrayManager
    {
    public:
        SdkTrayManager()
            : mWindow(0), mListener(0), mLoadBar(0), mLoadInc(0.0f), mGroupInitProportion(0.5f), mGroupLoadProportion(0.5f) {}
        SdkTrayManager(const Ogre::String& name, Ogre::RenderWindow* window, InputContext inputContext, SdkTrayListener* listener = 0)
            : mName(name), mWindow(window), mInput(inputContext), mListener(listener), mLoadBar(0), mLoadInc(0.0f), mGroupInitProportion(0.5f), mGroupLoadProportion(0.5f) {}
        virtual ~SdkTrayManager() {}

        void setListener(SdkTrayListener* listener) { mListener = listener; }
        void hideCursor() {}
        bool injectMouseMove(const OIS::MouseEvent& arg) { (void)arg; return false; }
        bool injectMouseDown(const OIS::MouseEvent& arg, OIS::MouseButtonID id) { (void)arg; (void)id; return false; }
        bool injectMouseUp(const OIS::MouseEvent& arg, OIS::MouseButtonID id) { (void)arg; (void)id; return false; }
        void frameRendered(const Ogre::Real t) { (void)t; }
        InputContext getInputContext() const { return mInput; }

        Label* createLabel(TrayLocation loc, const Ogre::String& name, const Ogre::String& caption, Ogre::Real width = 0)
        {
            (void)loc;
            (void)name;
            (void)width;
            return new Label(caption);
        }

        Button* createButton(TrayLocation loc, const Ogre::String& name, const Ogre::String& caption, Ogre::Real width = 0)
        {
            (void)loc;
            (void)width;
            return new Button(name, caption);
        }

        TextBox* createTextBox(TrayLocation loc, const Ogre::String& name, const Ogre::String& caption, Ogre::Real width = 0, Ogre::Real height = 0)
        {
            (void)loc;
            (void)width;
            (void)height;
            return new TextBox(name, caption);
        }

        SelectMenu* createThickSelectMenu(TrayLocation loc, const Ogre::String& name, const Ogre::String& caption, Ogre::Real width, unsigned int maxItemsShown, const std::vector<Ogre::String>& items)
        {
            (void)loc;
            (void)caption;
            (void)width;
            (void)maxItemsShown;
            SelectMenu* m = new SelectMenu(name);
            for (size_t i = 0; i < items.size(); ++i) {
                m->addItem(items[i]);
            }
            return m;
        }

        ParamsPanel* createParamsPanel(TrayLocation loc, const Ogre::String& name, Ogre::Real width, const std::vector<Ogre::String>& items)
        {
            (void)loc;
            (void)width;
            (void)items;
            return new ParamsPanel(name);
        }

        Ogre::OverlayElement* getTrayContainer(TrayLocation loc)
        {
            (void)loc;
            static Ogre::OverlayElement elem;
            return &elem;
        }

        void showCursor(const Ogre::String& materialName = "") { (void)materialName; }
        Ogre::OverlayElement* getCursorImage()
        {
            static Ogre::OverlayElement cursor;
            return &cursor;
        }

        void destroyWidget(Widget* w) { delete w; }
        void windowUpdate() {}

    protected:
        Ogre::String mName;
        Ogre::RenderWindow* mWindow;
        InputContext mInput;
        SdkTrayListener* mListener;
        ProgressBar* mLoadBar;
        Ogre::Real mLoadInc;
        Ogre::Real mGroupInitProportion;
        Ogre::Real mGroupLoadProportion;
    };
}

#endif
