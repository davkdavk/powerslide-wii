#ifndef AILOADER_H
#define AILOADER_H

#include "PFLoader.h"

#include "../GameState.h"

/**
 * Loads AI trajectories
 */
class AILoader
{
public:
    AILoader();

    void load(GameState& gameState, Ogre::SceneManager* sceneMgr, bool isDebugAI) const;
    bool isLoaded() const {return mIsLoaded;}

private:

    mutable Ogre::NameGenerator mNameGenNodes;
    mutable bool mIsLoaded = false;
};

#endif
