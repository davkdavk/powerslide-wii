
#include "BaseMenuMode.h"

#include "../gamelogic/RacingGridGeneration.h"

#include "../customs/CustomTrayManager.h"
#include "../customs/CustomOverlaySystem.h"

#include "../listeners/LoaderListener.h"

#include "../InputHandler.h"

#if defined(WII) || defined(__wii__)
#include "OGRE/WiiGXRenderer.h"
#endif

BaseMenuMode::BaseMenuMode(const ModeContext& modeContext, bool isInitialCreationMultiplayer) :
    BaseMode(modeContext)
{
    //flag for multiplayer only, indicate switch from single menu to multi
    if(isInitialCreationMultiplayer)
    {
        mModeContext.getGameState().setMultiplayerServerPort(8800);

        std::vector<std::string> playersCharacters;
        playersCharacters.push_back(mModeContext.getGameState().getPlayerCar().getCharacterName());
        recalculateCharacterNames(playersCharacters);
    }
}

void BaseMenuMode::recalculateCharacterNames(const std::vector<std::string>& playersCharacters)
{
    //based on AICount
    std::vector<size_t> resAISlot;
    std::vector<std::string> aiCharacters = RacingGridGeneration().generate(mModeContext.getGameState(), playersCharacters, resAISlot);
    mModeContext.getGameState().setAICharacters(aiCharacters);
    for(size_t q = 0; q < mModeContext.getGameState().getAICountInRace(); ++q)
    {
        mModeContext.getGameState().getAICar(q).setCharacterName(aiCharacters[q]);
        mModeContext.getGameState().getAICar(q).setSlotIndex(resAISlot[q]);
    }
}

void BaseMenuMode::initData(LoaderListener* loaderListener)
{
#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(16, 0, 224, 255);
#endif

    Ogre::ResourceGroupManager::getSingleton().createResourceGroup(TEMP_RESOURCE_GROUP_NAME);

#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(16, 0, 192, 255);
#endif

    Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("UI");

#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(16, 0, 160, 255);
#endif

    Ogre::ResourceGroupManager::getSingleton().loadResourceGroup("UI");

#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(16, 0, 144, 255);
#endif

    //to load textures & materials
#if defined(WII) || defined(__wii__)
    // Skip this pre-menu update during Wii bring-up; it can re-enter an unstable render path here.
#else
    mModeContext.mWindow->update(false);
#endif

#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(16, 0, 128, 255);
#endif

#ifndef NO_OPENAL
    mModeContext.mSoundsProcesser.initSoundsUI(mModeContext.mGameState.getPFLoaderGameshell());
    mModeContext.mSoundsProcesser.setVolume(mModeContext.mGameState.getSoundsGain());
#endif

    doInitData(loaderListener);

#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(16, 0, 96, 255);
#endif

    if(loaderListener)
        loaderListener->loadState(1.0f, "loaded");

#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(16, 0, 64, 255);
#endif
}

void BaseMenuMode::initCamera()
{
    mSceneMgr = mModeContext.mRoot->createSceneManager(Ogre::ST_GENERIC);
    mSceneMgr->addRenderQueueListener(mModeContext.mOverlaySystem);
    mMainNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    mCamera = mSceneMgr->createCamera("MenuCam");
    Ogre::Viewport * mViewPort = mModeContext.mWindow->addViewport(mCamera);
    mCamera->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
    mCamera->setOrthoWindow(static_cast<float>(mViewPort->getActualWidth()), static_cast<float>(mViewPort->getActualHeight()));
    mCamera->setNearClipDistance(0.5f);
    mCamera->setFarClipDistance(10000.0f);
    mCamera->setFOVy(Ogre::Degree(45.0f));
    mCamera->setPosition(0.0f, 0.0f, 100.0f);
    mCamera->lookAt(Ogre::Vector3::ZERO);

    //update mouse position
    const OIS::MouseState &ms = mModeContext.mInputHandler->getInputContext().mMouse->getMouseState();

    //mModeContext.mTrayMgr->showCursor();
#if !defined(__ANDROID__)
    mModeContext.mTrayMgr->showCursor("Test/Cursor");
    Ogre::OverlayElement* cursor = mModeContext.mTrayMgr->getCursorImage();
    cursor->setWidth(24.0f);
    cursor->setHeight(24.0f);
#else
    mModeContext.mTrayMgr->hideCursor();
#endif
    //mModeContext.mTrayMgr->getCursorLayer()->setScale(0.75f, 0.75f);
    //mModeContext.mTrayMgr->getCursorContainer()->setPosition(mViewPort->getActualWidth() / 2.0f, mViewPort->getActualHeight() / 2.0f);

}

void BaseMenuMode::clearData()
{
#if defined(WII) || defined(__wii__)
    // Wii bring-up path can switch out of menu before menu camera/scene are created.
    // In that case there is nothing to tear down here.
    if(!mSceneMgr)
        return;
#endif

    doClearData();

#ifndef NO_OPENAL
    mModeContext.mSoundsProcesser.deinitSoundsUI();
#endif

    //mModeContext.mGUI->shutdown();
    //mModeContext.mPlatform->shutdown();

    mSceneMgr->clearScene();
    mModeContext.mRoot->destroySceneManager(mSceneMgr);
    mModeContext.mWindow->removeAllViewports();

    mModeContext.mTrayMgr->hideCursor();

    Ogre::ResourceGroupManager::getSingleton().clearResourceGroup("UI");

    Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup(TEMP_RESOURCE_GROUP_NAME);
}

void BaseMenuMode::frameRenderingQueued(const Ogre::FrameEvent &evt)
{
    customFrameRenderingQueued();

    mModeContext.mInputHandler->capture();
    mModeContext.mTrayMgr->frameRenderingQueued(evt);
}
