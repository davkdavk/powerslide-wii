
#include "MenuMode.h"

#include "../ui/UIMainMenu.h"

#if defined(WII) || defined(__wii__)
#include "OGRE/WiiGXRenderer.h"
#endif

MenuMode::MenuMode(const ModeContext& modeContext, const GameMode gameMode, SinglePlayerMenuStates state) :
    BaseMenuMode(modeContext, true)
{
    mUIMainMenu = std::make_shared<UIMainMenu>(modeContext, gameMode, this, state);
}

void MenuMode::doInitData(LoaderListener* loaderListener)
{
#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(16, 16, 224, 255);
#endif

    mUIMainMenu->load(mModeContext.mTrayMgr, mModeContext.mGameState, loaderListener);

#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(16, 16, 192, 255);
#endif
}

void MenuMode::doClearData()
{
    mUIMainMenu->destroy(mModeContext.mTrayMgr);
}

void MenuMode::frameStarted(const Ogre::FrameEvent &evt)
{
    mUIMainMenu->frameStarted(evt);
}

void MenuMode::keyUp(MyGUI::KeyCode _key, wchar_t _char )
{
    mUIMainMenu->keyUp(_key, _char);
}

void MenuMode::mousePressed(const Ogre::Vector2& pos)
{
    mUIMainMenu->mousePressed(pos);
}

void MenuMode::mouseReleased(const Ogre::Vector2& pos, OIS::MouseButtonID id)
{
    mUIMainMenu->mouseReleased(pos, id);
}

void MenuMode::mouseMoved(const Ogre::Vector2& pos)
{
    mUIMainMenu->mouseMoved(pos);
}

SinglePlayerMenuStates MenuMode::getSubmenuState() const
{
    return mUIMainMenu->getSubmenuState();
}

bool MenuMode::isExitSubmenu() const
{
    return mUIMainMenu->isExitSubmenu();
}

void MenuMode::setSubmenu(const std::string& title)
{
    mUIMainMenu->setSubmenu(title);
}

void MenuMode::setTopmostSubmenu()
{
    mUIMainMenu->setTopmostSubmenu();
}

void MenuMode::setDefaultBackground(bool isSwitchState)
{
    mUIMainMenu->setDefaultBackground(isSwitchState);
}

void MenuMode::setPodiumSubmenu()
{
    mUIMainMenu->setPodiumSubmenu();
}

#if defined(__ANDROID__)
void MenuMode::reloadTextures()
{
    mUIMainMenu->reloadTextures(mModeContext.mGameState);
}
#endif
