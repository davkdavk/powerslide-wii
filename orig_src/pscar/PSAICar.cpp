
#include "PSAICar.h"

#include "../physics/Physics.h"
#include "../physics/PhysicsVehicle.h"
#include "../mesh/StaticMeshProcesser.h"

#include "../tools/Conversions.h"

#if defined(WII) || defined(__wii__)
#include "../WiiDebugLog.h"
#endif

PSAICar::PSAICar()
{
}

void PSAICar::initModel(    lua_State * pipeline, 
                            const GameState& gameState,
                            Ogre::SceneManager* sceneMgr, Ogre::SceneNode* mainNode,
                            ModelsPool* modelsPool,
                            Physics * world,
                            const std::string& characterName,
                            InitialVehicleSetup& initialVehicleSetup,
                            bool isPossesCamera)
{
    PSControllableCar::initModel(pipeline, gameState, sceneMgr, mainNode, modelsPool, world, characterName, 
        initialVehicleSetup,
        isPossesCamera);

#if defined(WII) || defined(__wii__)
    if(!mPhysicsVehicle)
    {
        WiiDebugLog("[MODEL] PSAICar::initModel no physics vehicle, skip setVehicleType\n");
        return;
    }
#endif

    mPhysicsVehicle->setVehicleType(AIVehicle);
}

void PSAICar::performAICorrection(const GameState& gameState, PhysicsVehicleAI* physicsAICar, const InitialVehicleSetup& initialVehicleSetup, Ogre::int32 afterStartCounter)
{
    mAIUtils.performAICorrection(this, physicsAICar, gameState, initialVehicleSetup, afterStartCounter);
}

void PSAICar::setAIData(const AIWhole& aiWhole, Ogre::SceneManager* sceneMgr, bool isDebugAI)
{
    mAIUtils.setAIData(aiWhole, sceneMgr, isDebugAI);
}

void PSAICar::raceStarted()
{
    mPhysicsVehicle->setRaceStarted();
    mPhysicsVehicle->gearUp();
    mAIUtils.raceStarted();
}
