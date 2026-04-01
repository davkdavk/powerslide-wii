
#ifndef CUSTOMTRAYMANAGER_H
#define CUSTOMTRAYMANAGER_H

#include "../includes/OgreInclude.h"
#include "../includes/OISInclude.h"
#include "../SdkTrays.h"

class CustomTrayManager : public OgreBites::SdkTrayManager
{
public :

    class TraysLayerCompat
    {
    public:
        void add3D(Ogre::SceneNode* node) { (void)node; }
        void remove3D(Ogre::SceneNode* node) { (void)node; }
    };

    CustomTrayManager(const Ogre::String& name, Ogre::RenderWindow* window, OgreBites::InputContext inputContext, OgreBites::SdkTrayListener* listener = 0)
        : OgreBites::SdkTrayManager(name, window, inputContext, listener){}


    virtual ~CustomTrayManager(){}

    void frameRenderingQueued(const Ogre::FrameEvent& evt)
    {
        frameRendered(evt.timeSinceLastFrame);
    }

    void resourceGroupScriptingStarted(const Ogre::String& groupName, size_t scriptCount);

    void scriptParseStarted(const Ogre::String& scriptName, bool& skipThisScript);
    void scriptParseEnded(const Ogre::String& scriptName, bool skipped);

    void resourceGroupLoadStarted(const Ogre::String& groupName, size_t resourceCount);

    void resourceLoadStarted(const Ogre::ResourcePtr& resource);
    void resourceLoadEnded();

    TraysLayerCompat* getTraysLayer() { return &mTraysLayer; }
    bool isDialogVisible() const { return false; }

private:

    Ogre::Timer mTimer;
    TraysLayerCompat mTraysLayer;

    void windowOnTimerUpdater();
};

#endif
