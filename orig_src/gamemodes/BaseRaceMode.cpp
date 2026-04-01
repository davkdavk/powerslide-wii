
#include "BaseRaceMode.h"

#include "../CameraMan.h"
#include "../InputHandler.h"

#include "../tools/Randomizer.h"

#include "../customs/CustomTrayManager.h"
#include "../customs/CustomOverlaySystem.h"

#include "../loaders/ExclusionLoader.h"
#include "../loaders/LLTLoader.h"
#include "../loaders/PHYLoader.h"
#include "../loaders/AILoader.h"
#include "../loaders/TerrainLoader.h"
#include "../loaders/ParticlesLoader.h"
#include "../loaders/TEXLoader.h"
#include "../loaders/TextureLoader.h"

#if defined(WII) || defined(__wii__)
#include "../WiiDebugLog.h"
#include "../WiiMemoryBudget.h"
#include "../../wii_stubs/OGRE/WiiGXRenderer.h"
#include <malloc.h>
#endif

#include "../physics/Physics.h"
#include "../physics/PhysicsVehicle.h"

#include "../listeners/LoaderListener.h"

#include "../gamelogic/GameModeSwitcher.h"

#include "../cheats/Cheats.h"

#include "../ui/UIRace.h"

#if defined(__ANDROID__)
    #include <android/log.h>

    #define LOGI(...) ((void)__android_log_write(ANDROID_LOG_INFO, "OGRE", __VA_ARGS__))
    #define LOGE(...) ((void)__android_log_write(ANDROID_LOG_ERROR, "OGRE", __VA_ARGS__)) 
#endif

BaseRaceMode::BaseRaceMode(const ModeContext& modeContext) :
    BaseMode(modeContext),
    mShadowLightDistanceFromCar(40.0f),
    mIsGlobalReset(true),
    mRearCamera(0),
    mUIRace(std::make_shared<UIRace>(modeContext)),
    mLoaderListener(NULL)
#if SHOW_DETAILS_PANEL
    ,mDetailsPanel(0)
#endif
{
    mFOVNitro.addPoint(0, 90.0f);
    mFOVNitro.addPoint(PhysicsVehicle::mNitroFrames / 2.0f, 120.0f);
    mFOVNitro.addPoint(PhysicsVehicle::mNitroFrames, 90.0f);
}

BaseRaceMode::~BaseRaceMode()
{
    mModeContext.mWindow->removeListener(this);//for arrow
}

void BaseRaceMode::initData(LoaderListener* loaderListener)
{
    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initData]: Enter");
#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(192, 0, 0, 255);
#endif

    mLuaManager.CallFunction_0_0("main", mModeContext.mPipeline);
#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(192, 32, 0, 255);
#endif

    initScene(loaderListener);
#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(192, 64, 0, 255);
#endif

    mLoaderListener = loaderListener;

    //LUTs
    if(mModeContext.mGameState.getDoUpscale())
    {
        bool lutsLoadSuccess = mModeContext.mGameState.loadLUTs();
        mModeContext.mGameState.setDoUpscale(lutsLoadSuccess);
    }

    if(loaderListener)
        loaderListener->loadState(0.2f, "scene inited");

    loadResources();
#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(192, 96, 0, 255);
#endif

    if(loaderListener)
        loaderListener->loadState(0.3f, "terrain loading");

#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(192, 104, 0, 255);
#endif

    initTerrain(loaderListener);
#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(192, 128, 0, 255);
#endif

    if(loaderListener)
        loaderListener->loadState(0.8f, "models loading");

    initModel(loaderListener);
#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(192, 160, 0, 255);
#endif

    if(loaderListener)
        loaderListener->loadState(0.9f, "misc loading");

    initMisc();
#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(192, 192, 0, 255);
#endif

    if(loaderListener)
        loaderListener->loadState(0.95f, "light list loading");

    initLightLists();
#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(192, 224, 0, 255);
#endif

    if(loaderListener)
        loaderListener->loadState(1.0f, "all loaded");

#ifndef NO_OPENAL
    const STRPowerslide& strPowerslide = mModeContext.getGameState().getSTRPowerslide();
    size_t cdTrack = strPowerslide.getCDTrack(mModeContext.getGameState().getTrackName());
    mModeContext.mMusicProcessor.initTrack("track_" + Conversions::DMToString(cdTrack) + ".ogg", mModeContext.getGameState().isStuntTrack());
    mModeContext.mMusicProcessor.play();
#endif

#if SHOW_DETAILS_PANEL
    // create a params panel for displaying sample details
    Ogre::StringVector items;
    //items.push_back("cam.pX");
    //items.push_back("cam.pY");
    //items.push_back("cam.pZ");
    items.push_back("car.pX");
    items.push_back("car.pY");
    items.push_back("car.pZ");
    items.push_back("");
    items.push_back("carSpeedLinear");
    items.push_back("carSpeedProj");
    items.push_back("carSpeedLateral");
    items.push_back("");
    items.push_back("FrontL");
    items.push_back("FrontR");
    items.push_back("BackL");
    items.push_back("BackR");
    items.push_back("");
    items.push_back("FrontL");
    items.push_back("FrontR");
    items.push_back("BackL");
    items.push_back("BackR");
    items.push_back("");
    items.push_back("FrontL");
    items.push_back("FrontR");
    items.push_back("BackL");
    items.push_back("BackR");
    items.push_back("");
    items.push_back("LapPosition");
    items.push_back("LapTime");
    items.push_back("Time");
    items.push_back("Lap");
    items.push_back("Position");
    items.push_back("Gear");
    items.push_back("RPM");
    //items.push_back("Filtering");
    //items.push_back("Poly Mode");

    mDetailsPanel = mModeContext.mTrayMgr->createParamsPanel(OgreBites::TL_LEFT, "DetailsPanel", 200, items);
    //mDetailsPanel->hide();
#endif

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initData]: Exit");
}


void BaseRaceMode::initCamera()
{

    const Ogre::int32 actualWidth = mModeContext.mWindow->getWidth();
    const Ogre::int32 actualHeight = mModeContext.mWindow->getHeight();

    mCamera = mSceneMgr->createCamera("PlayerCam");
    mCamera->setNearClipDistance(0.5f);

    Ogre::Real viewportYShift = (actualHeight / 4.73f / 1.7f) / actualHeight;//see dashHeight
    mViewPortScene = mModeContext.mWindow->addViewport(mCamera, 0, 0.0f, -viewportYShift, 1.0f, 1.0f);

    mViewPortScene->setBackgroundColour(mModeContext.mGameState.getBackgroundColor());
    mViewPortScene->setOverlaysEnabled(true);
    mModeContext.mWindow->addListener(this);//for arrow
    mCamera->setAspectRatio(1.0f * Ogre::Real(actualWidth) / Ogre::Real(actualHeight) / (640.0f / 480.0f));
    mCamera->setFOVy(Ogre::Degree(90.0f));

    mCameraMan = std::make_shared<CameraMan>(mCamera, mStaticMeshProcesser, mModeContext.getGameState().getCameraPositionType());
    mModeContext.mInputHandler->resetCameraMenPointer(mCameraMan.get());

    Ogre::int32 aiCamIndex = mLuaManager.ReadScalarInt("Scene.AICamIndex", mModeContext.mPipeline);
    bool isCamToAI = mLuaManager.ReadScalarBool("Scene.IsCamToAI", mModeContext.mPipeline);

    if(!isCamToAI)
        mModeContext.mGameState.getPlayerCar().setCameraMan(mCameraMan.get());
    else
        mModeContext.mGameState.getAICar(aiCamIndex).setCameraMan(mCameraMan.get());

    //http://www.ogre3d.org/forums/viewtopic.php?p=331138
    //http://www.ogre3d.org/forums/viewtopic.php?f=2&t=79581
    //for tacho needle
    if(mModeContext.mRoot)
    {
        mSceneMgrCarUI = mModeContext.mRoot->createSceneManager(Ogre::ST_GENERIC);
    }
    else
    {
        WiiDebugLog("[CAM] WARNING: mRoot is null, reusing main SceneManager for UI\n");
        mSceneMgrCarUI = mSceneMgr;
    }

    if(!mSceneMgrCarUI)
    {
        WiiDebugLog("[CAM] ERROR: no SceneManager available for UI camera, skipping UI camera init\n");
        return;
    }

    mSceneMgrCarUI->addRenderQueueListener(mModeContext.mOverlaySystem);
    Ogre::Camera * cameraCarUI = mSceneMgrCarUI->createCamera("PlayerUICam");
    cameraCarUI->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
    cameraCarUI->setOrthoWindow(static_cast<float>(actualWidth), static_cast<float>(actualHeight));
    cameraCarUI->setNearClipDistance(0.5f);
    cameraCarUI->setFarClipDistance(10000.0f);
    mViewPortCarUI = mModeContext.mWindow->addViewport(cameraCarUI, 1);
    mViewPortCarUI->setClearEveryFrame(true, Ogre::FBT_DEPTH);
    mViewPortCarUI->setOverlaysEnabled(true);
    mViewPortCarUI->setSkiesEnabled(false);
    cameraCarUI->setAspectRatio(static_cast<float>(actualWidth) / static_cast<float>(actualHeight));
    cameraCarUI->setFOVy(Ogre::Degree(45.0f));
    cameraCarUI->setPosition(0.0f, 0.0f, 100.0f);
    cameraCarUI->lookAt(Ogre::Vector3::ZERO);

    mModeContext.mTrayMgr->hideCursor();

    //init misc
#if !defined(WII_NATIVE_ASSET_PIPELINE)
    if(mModeContext.mGameState.getMirrorEnabled())
    {
        mRearCamera = mSceneMgr->createCamera("RearViewCamera");
        mRearCamera->setNearClipDistance(0.5f);
        Ogre::Viewport *v = mUIRace->rearViewMirrorPanelTextureAddViewport(mRearCamera);
        v->setOverlaysEnabled(false);
        v->setClearEveryFrame(true);
        v->setBackgroundColour(mModeContext.mGameState.getBackgroundColor());
        mRearCamera->setAspectRatio(mLuaManager.ReadScalarFloat("Scene.Mirror.Aspect", mModeContext.mPipeline));
        mRearCamera->setFOVy(Ogre::Degree(mLuaManager.ReadScalarFloat("Scene.Mirror.FOV", mModeContext.mPipeline)));
    }

    mCameraMan->setRearCamera(mRearCamera);
#else
    mRearCamera = NULL;
    mCameraMan->setRearCamera(NULL);
#endif
    //init misc end

#if defined(WII_NATIVE_ASSET_PIPELINE)
    WiiDebugLog("[CAM] WII_NATIVE_ASSET_PIPELINE: skipping race HUD init (tacho/mirror panels)\n");
#else
    mUIRace->initTachoNeedleAndPointer(mSceneMgrCarUI, mModeContext.mGameState);
    mUIRace->createRearViewMirrorPanel(mModeContext.mTrayMgr, mModeContext.mGameState.getMirrorEnabled());
    mUIRace->load(mModeContext.mTrayMgr, mModeContext.mGameState);
    mUIRace->setShowMiscTextRight(true);
#endif

    customInitUI();
}

void BaseRaceMode::clearData()
{
#if SHOW_DETAILS_PANEL
    mModeContext.mTrayMgr->destroyWidget(mDetailsPanel);
#endif

    customClearUI();

    clearScene();

    unloadResources();

    mModeContext.mInputHandler->resetCameraMenPointer(NULL);
    mCameraMan.reset();

    mUIRace->destroy(mModeContext.mTrayMgr);
}

void BaseRaceMode::restart()
{
    //d.polubotko: TODO - implement restart
#if 0
    clearScene();
    mLuaManager.CallFunction_0_0("main", mModeContext.mPipeline);
    initScene(NULL);
    initTerrain(NULL);
    initModel(NULL);
    initMisc();

    initLightLists();
#endif
}

void BaseRaceMode::createBurnByPlayer()
{
    mCheats->createBurnByPlayer(mWorld->getVehicle(&mModeContext.mGameState.getPlayerCar()));

#ifndef NO_OPENAL
    mModeContext.getSoundsProcesser().playCheatBurn();
#endif
}

void BaseRaceMode::createBombByPlayer()
{
    mCheats->createBombByPlayer(mWorld->getVehicle(&mModeContext.mGameState.getPlayerCar()));

#ifndef NO_OPENAL
    mModeContext.getSoundsProcesser().playCheatBomb();
#endif
}

void BaseRaceMode::cheatByPlayer(bool isEnabled, InputKeyMapping cheat)
{
    mUIRace->setVisibleCheat(isEnabled, cheat);
}

void BaseRaceMode::mousePressed(const Ogre::Vector2& pos)
{
    mUIRace->mousePressed(pos);
}

void BaseRaceMode::mouseReleased(const Ogre::Vector2& pos)
{
    mUIRace->mouseReleased(pos);
}

void BaseRaceMode::mouseMoved(const Ogre::Vector2& pos)
{
    mUIRace->mouseMoved(pos);
}

void BaseRaceMode::initScene(LoaderListener* loaderListener)
{
#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(255, 0, 255, 255);
#endif

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initScene]: Enter");

    mShadowLightDistanceFromCar = mLuaManager.ReadScalarFloat("Model.ShadowLightDistance", mModeContext.mPipeline);

    if(mModeContext.mRoot)
    {
        mSceneMgr = mModeContext.mRoot->createSceneManager(Ogre::ST_GENERIC);
    }

#if defined(WII) || defined(__wii__)
    if(!mSceneMgr)
    {
        WiiDebugLog("[RACE] ERROR: cannot create primary SceneManager, abort initScene\n");
        return;
    }
#endif

#ifndef NO_OPENAL
    mModeContext.mSoundsProcesser.initSounds(mModeContext.mGameState.getPFLoaderData());
    mModeContext.mSoundsProcesser.setVolume(mModeContext.mGameState.getSoundsGain());
    mModeContext.mMusicProcessor.stop();
#endif

    //migration from 1.8.1 to 1.9.0
    //http://www.ogre3d.org/forums/viewtopic.php?f=2&t=78278
    mSceneMgr->addRenderQueueListener(mModeContext.mOverlaySystem);

    mSceneMgr->setAmbientLight(Ogre::ColourValue(1.0, 1.0, 1.0));

    mMainNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();

    initWorld();

    if(loaderListener)
        loaderListener->loadState(0.1f, "world inited");

    //load data
    {
        bool isDebugLLT = true;
        LLTLoader().load(mModeContext.mGameState, mSceneMgr, isDebugLLT);
    }

    if(loaderListener)
        loaderListener->loadState(0.12f, "LLT loaded");

    {
        bool isDebugExclusion = mLuaManager.ReadScalarBool("Terrain.Scene.IsDebugExclusion", mModeContext.mPipeline);
        ExclusionLoader().load(mModeContext.mGameState, mSceneMgr, isDebugExclusion);
    }

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] exclusion stage done\n");
#endif

    if(loaderListener)
        loaderListener->loadState(0.13f, "Exclusions loaded");

    PHYLoader().load(mModeContext.mGameState);

#if defined(WII) || defined(__wii__)
    {
        std::vector<InitialVehicleSetup>& setups = mModeContext.mGameState.getInitialVehicleSetup();
        if(!setups.empty())
        {
            const Ogre::Matrix4& mat = setups[0].mTrackPosition;
            const float carStartX = mat[0][3];
            const float carStartY = mat[1][3];
            const float carStartZ = mat[2][3];
            const float carForwardX = mat[0][2];
            const float carForwardY = mat[1][2];
            const float carForwardZ = mat[2][2];
            WiiGX::Renderer::getInstance().setCarStartPositionF32(carStartX, carStartY, carStartZ);
            WiiGX::Renderer::getInstance().setCarForwardDirF32(carForwardX, carForwardY, carForwardZ);
            WiiDebugLog("[RACE] car start pos=(%f,%f,%f) fwd=(%f,%f,%f)\n",
                carStartX, carStartY, carStartZ, carForwardX, carForwardY, carForwardZ);
        }
    }
    WiiDebugLog("[RACE] phy stage done\n");
#endif

    if(loaderListener)
        loaderListener->loadState(0.14f, "PHY loaded");

#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(255, 255, 0, 255);
#endif

    //init multiplayer (before AILoader().load due to getAICountInRace dependance)
    {
        customInitScene();
    }

    {
        bool isDebugAI = mLuaManager.ReadScalarBool("Terrain.Scene.IsDebugAI", mModeContext.mPipeline);

        AILoader().load(mModeContext.mGameState, mSceneMgr, isDebugAI);

        if(loaderListener)
            loaderListener->loadState(0.18f, "AI loaded");
    }

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] ai stage done\n");
#endif

    ParticlesLoader().load(mModeContext.mGameState);

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] particles stage done\n");
#endif

#if defined(WII) || defined(__wii__)
    WiiGX::Renderer::getInstance().presentProbeColor(0, 255, 255, 255);
    WiiDebugLog("[RACE] initScene exit\n");
#endif

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initScene]: Exit");
}


void BaseRaceMode::clearScene()
{
    mUIRace->clearMiscPanelText();
    mUIRace->setVisibleFinishSign(false);
    mUIRace->setVisibleCheat(false, kmEmpty);

    mModeContext.mGameState.resetGamePaused();
    mModeContext.mGameState.setRaceStarted(false);
    mModeContext.mGameState.setRaceFinished(false);

    //deinit sounds of system

    //clear sounds of cars as well
    mModeContext.mGameState.getPlayerCar().clear();
    mModeContext.mGameState.getPlayerCar().deinitSounds();
    for(size_t q = 0; q < mModeContext.mGameState.getAICountInRace(); ++q)
    {
        mModeContext.mGameState.getAICar(q).clear();
        mModeContext.mGameState.getAICar(q).deinitSounds();
    }

    customClearScene();

    deInitWorld();

#ifndef NO_OPENAL
    mModeContext.mSoundsProcesser.stopSounds();
    mModeContext.mMusicProcessor.stop();
    mModeContext.mSoundsProcesser.setListenerGain(1.0f);//restore after pause
#endif

    mModeContext.mGameState.setGlobalLight(NULL);
    mModeContext.mGameState.setShadowLight(NULL);

    mModeContext.mInputHandler->resetCameraMenPointer(NULL);
    mCameraMan.reset();

    mModeContext.mTrayMgr->getTraysLayer()->remove3D(mModeContext.mGameState.getArrowNode());

    mSceneMgr->clearScene();
    mModeContext.mWindow->removeAllViewports();
    mSceneMgr->destroyAllCameras();
    mModeContext.mRoot->destroySceneManager(mSceneMgr);
    mSceneMgrCarUI->clearScene();
    mSceneMgrCarUI->destroyAllCameras();
    mModeContext.mRoot->destroySceneManager(mSceneMgrCarUI);

#ifndef NO_OPENAL
    mModeContext.mMusicProcessor.initTrack("track_0.ogg", true);
    mModeContext.mMusicProcessor.play();
#endif
}

void BaseRaceMode::initLightLists()
{
    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initLightLists]: Enter");

    //d.polubotko: make sure light lists created during loading
    mStaticMeshProcesser.queryLights();

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initLightLists]: Exit");
}

void BaseRaceMode::initTerrain(LoaderListener* loaderListener)
{
    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initTerrain]: Enter");

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] initTerrain enter\n");
#if defined(WII_NATIVE_ASSET_PIPELINE)
    if(!mIsGlobalReset)
    {
        WiiDebugLog("[RACE] forcing mIsGlobalReset=true before initParts\n");
        mIsGlobalReset = true;
    }
#endif
#endif

    mStaticMeshProcesser.initParts( mModeContext.mPipeline, mSceneMgr, mMainNode, mIsGlobalReset, 
                                    mModeContext.mGameState, mWorld.get(),
                                    loaderListener);

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] terrain initParts done\n");
#endif

    mStaticMeshProcesser.loadTerrainMaps(mModeContext.mGameState);

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] terrain maps loaded\n");
#endif

    //load terrain params
    TerrainLoader terrainLoader;
    terrainLoader.load(mModeContext.mGameState);

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] terrain loader done\n");
#endif

    mStaticMeshProcesser.setTerrainData(terrainLoader.getTerrainData());

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] terrain data assigned\n");
#endif

#ifndef NO_OPENAL
    mModeContext.mSoundsProcesser.setTerrainData(terrainLoader.getTerrainData());
#endif

    //particle (before loading cars)
    {
        //Ogre::TexturePtr particle = TextureLoader().load(mModeContext.mGameState.getPFLoaderData(), "data/deii", "particle.tga", "OriginalParticle");
        Ogre::TexturePtr particle = TEXLoader().load(mModeContext.mGameState.getPFLoaderData(), "data/deii", "d3d2display1_m_1.tex", "OriginalParticle", mModeContext.mGameState.getX2LUTs(), mModeContext.mGameState.getX4LUTs());

#if defined(WII) || defined(__wii__)
        WiiDebugLog("[RACE] particle tex loaded\n");
#endif

        Ogre::MaterialManager& materialManager = Ogre::MaterialManager::getSingleton();
        auto getOrCreateMaterial = [&materialManager](const Ogre::String& materialName) -> Ogre::MaterialPtr
        {
            Ogre::MaterialPtr material = materialManager.getByName(materialName);
            if(material.isNull())
            {
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL,
                    "[BaseRaceMode::initTerrain]: Missing material, creating fallback: " + materialName);
                material = materialManager.create(materialName, TEMP_RESOURCE_GROUP_NAME);
            }
            return material;
        };

        Ogre::MaterialPtr particleMat = getOrCreateMaterial("Test/Particle");
        particleMat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(particle);

#if defined(WII) || defined(__wii__)
        WiiDebugLog("[RACE] particle mat assigned\n");
#endif

        Ogre::MaterialPtr particleMatAlpha = getOrCreateMaterial("Test/ParticleAlpha");
        particleMatAlpha->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(particle);

        Ogre::MaterialPtr particleMatFog = getOrCreateMaterial("Test/ParticleFog");
        particleMatFog->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(particle);

        Ogre::MaterialPtr particleMatFogAlpha = getOrCreateMaterial("Test/ParticleFogAlpha");
        particleMatFogAlpha->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(particle);

#if defined(WII) || defined(__wii__)
        WiiDebugLog("[RACE] particle fog alpha assigned\n");
#endif
    }

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initTerrain]: Exit");

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] initTerrain exit\n");
#endif
}

void BaseRaceMode::initModel(LoaderListener* loaderListener)
{

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initModel]: Enter");

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] initModel enter\n");
#endif

    //arrow (before loading models)
    {
#if defined(WII) || defined(__wii__)
        WiiDebugLog("[RACE] before arrow texture load\n");
#endif
        TEXLoader().load(mModeContext.mGameState.getPFLoaderData(), "data/misc", "arrow_m_3.tex", "OriginalArrow", mModeContext.mGameState.getX2LUTs(), mModeContext.mGameState.getX4LUTs());
#if defined(WII) || defined(__wii__)
        WiiDebugLog("[RACE] after arrow texture load\n");
#endif
    }

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] before set player prev velocity\n");
#endif
    mModeContext.mGameState.setPLayerCarPrevVel(Ogre::Vector3::ZERO);

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] before read ai camera lua values\n");
#endif

    Ogre::int32 aiCamIndex = mLuaManager.ReadScalarInt("Scene.AICamIndex", mModeContext.mPipeline);
    bool isCamToAI = mLuaManager.ReadScalarBool("Scene.IsCamToAI", mModeContext.mPipeline);

    mLapController.clear();
#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] before lapController add player\n");
#endif
    mLapController.addCar(&mModeContext.mGameState.getPlayerCar());//should be first
    mModeContext.mGameState.getPlayerCar().getLapUtils().setEvents(this);

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] before models pool init\n");
#endif
    mModelsPool.initModels(mSceneMgr, mModeContext.mGameState);

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] models pool initialized\n");

#if defined(WII_NATIVE_ASSET_PIPELINE)
    WiiDebugLog("[RACE] WII_NATIVE_ASSET_PIPELINE active: stubbing vehicle/AI init\n");
    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initModel]: vehicle/AI init skipped for native Wii asset pipeline");
    if(loaderListener)
        loaderListener->loadState(0.9f, "vehicle and ai stubs active");
    WiiDebugLog("[RACE] initModel exit (stubbed)\n");
    return;
#endif
#endif

    std::vector<InitialVehicleSetup>& initialSetups = mModeContext.mGameState.getInitialVehicleSetup();
    InitialVehicleSetup fallbackSetup = InitialVehicleSetup();
    const size_t playerSetupIndex = mModeContext.mGameState.getAICountInRace();
    InitialVehicleSetup* playerSetup = &fallbackSetup;
    if(playerSetupIndex < initialSetups.size())
    {
        playerSetup = &initialSetups[playerSetupIndex];
    }
    else
    {
        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[BaseRaceMode::initModel]: missing player PHY setup, using fallback");
    }

    if(loaderListener)
        loaderListener->loadState(0.8f, "player model loading");

    mModeContext.mGameState.getPlayerCar().initModel(mModeContext.mPipeline, mModeContext.mGameState, mSceneMgr, mMainNode, &mModelsPool, mWorld.get(), mModeContext.mGameState.getPlayerCar().getCharacterName(), 
        *playerSetup,
        !isCamToAI);

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] player model initialized\n");
#endif

    mModeContext.mGameState.getPlayerCar().initSounds(mModeContext.mPipeline, mModeContext.mGameState);

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] player sounds initialized\n");
#endif

    PhysicsVehicle* playerPhysics = mModeContext.mGameState.getPlayerCar().getPhysicsVehicle();
    if(playerPhysics)
    {
        playerPhysics->getCarEngine().setTransmissionType(mModeContext.mGameState.getTransmissionType());
    }
    else
    {
        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[BaseRaceMode::initModel]: player physics vehicle is null");
    }

    if(loaderListener)
        loaderListener->loadState(0.81f, "player model loaded");

    //ghost
    if(mModeContext.getGameModeSwitcher()->getMode() == ModeRaceTimetrial)
    {
#if defined(WII) || defined(__wii__)
        WiiDebugLog("[RACE] skipping ghost init on Wii\n");
#else
        mTrialGhost.init(mModeContext.getGameState().getDataDir(), mModeContext.getGameState().getTrackNameAsOriginal(), mModeContext.getGameState().getPlayerCar().getCharacterName());

        if(mModeContext.getGameState().getGhostEnabled())
        {
            InitialVehicleSetup vehSetup = fallbackSetup;
            if(!initialSetups.empty())
                vehSetup = initialSetups[0];
            else
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[BaseRaceMode::initModel]: missing ghost PHY setup, using fallback");

            const InitialVehicleSetup& ghostUserSetup = !initialSetups.empty() ? initialSetups[0] : fallbackSetup;
            //setup suspension for non user ghost
            {
                std::string carName = mModeContext.getGameState().getSTRPowerslide().getCarFromCharacter(mTrialGhost.getCharacterName());
                mGhost.setCharacterName(mTrialGhost.getCharacterName());
                std::string carPath = mGhost.getCarPath(mModeContext.getGameState());
                STRSettings carSettings;
                carSettings.parse(mModeContext.getGameState().getPFLoaderStore(), "data/cars/" + carPath + "/data/default", "params.str");
                vehSetup.mCOG = -carSettings.getArray3Value("", "centre of gravity");
                vehSetup.mCOG.z = -vehSetup.mCOG.z;
                {
                        vehSetup.mConnectionPointWheel[0] = carSettings.getArray3Value("", "wheelbase back");
                        vehSetup.mConnectionPointWheel[0].z = -vehSetup.mConnectionPointWheel[0].z;

                        vehSetup.mConnectionPointWheel[1] = vehSetup.mConnectionPointWheel[0];
                        vehSetup.mConnectionPointWheel[1].x = -vehSetup.mConnectionPointWheel[1].x;

                        vehSetup.mConnectionPointWheel[0] += vehSetup.mCOG;
                        vehSetup.mConnectionPointWheel[1] += vehSetup.mCOG;

                        vehSetup.mConnectionPointWheel[2] = carSettings.getArray3Value("", "wheelbase front");
                        vehSetup.mConnectionPointWheel[2].z = -vehSetup.mConnectionPointWheel[2].z;

                        vehSetup.mConnectionPointWheel[3] = vehSetup.mConnectionPointWheel[2];
                        vehSetup.mConnectionPointWheel[3].x = -vehSetup.mConnectionPointWheel[3].x;

                        vehSetup.mConnectionPointWheel[2] += vehSetup.mCOG;
                        vehSetup.mConnectionPointWheel[3] += vehSetup.mCOG;

                    }
            }

            mGhost.initGraphicsModel(mModeContext.mPipeline, mModeContext.mGameState, mSceneMgr, mMainNode, &mModelsPool, mTrialGhost.getCharacterName(), vehSetup, mModeContext.getGameState().getAdvancedLightingPlayer());
            mGhost.setVisibility(mTrialGhost.isVisible());
            mGhostVisible = mTrialGhost.isVisible();

            mGhostUser.initGraphicsModel(mModeContext.mPipeline, mModeContext.mGameState, mSceneMgr, mMainNode, &mModelsPool, mModeContext.mGameState.getPlayerCar().getCharacterName(), ghostUserSetup, mModeContext.getGameState().getAdvancedLightingPlayer());
            mGhostUser.setVisibility(false);
            mGhostUserVisible = false;

#if defined(WII) || defined(__wii__)
            WiiDebugLog("[RACE] ghost models initialized\n");
#endif
        }
#endif
    }

    std::vector<std::string> aiCharacters = mModeContext.getGameState().getAICharacters();

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] before AI init loop\n");
#endif

    for(size_t q = 0; q < mModeContext.mGameState.getAICountInRace(); ++q)
    {
        PSAICar& aiCar = mModeContext.mGameState.getAICar(q);

        bool isCam = (q == aiCamIndex);
        if(!isCamToAI)
            isCam = false;

        InitialVehicleSetup* aiSetup = &fallbackSetup;
        if(q < initialSetups.size())
        {
            aiSetup = &initialSetups[q];
        }
        else
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[BaseRaceMode::initModel]: missing AI PHY setup, using fallback");
        }

        aiCar.initModel(mModeContext.mPipeline, mModeContext.mGameState, mSceneMgr, mMainNode, &mModelsPool, mWorld.get(), aiCharacters[q], 
            *aiSetup,
            isCam);

#if defined(WII) || defined(__wii__)
        WiiDebugLog("[RACE] ai model initialized idx=%u\n", static_cast<unsigned int>(q));
#endif

        aiCar.initSounds(mModeContext.mPipeline, mModeContext.mGameState);

#if defined(WII) || defined(__wii__)
        WiiDebugLog("[RACE] ai sounds initialized idx=%u\n", static_cast<unsigned int>(q));

        if(!aiCar.getPhysicsVehicle())
        {
            WiiDebugLog("[RACE] ai idx=%u has no physics vehicle, skip lap/event registration\n", static_cast<unsigned int>(q));
            continue;
        }
#endif

        mLapController.addCar(&aiCar);

        aiCar.getLapUtils().setEvents(NULL);

        if(loaderListener)
            loaderListener->loadState(0.81f + 0.09f * static_cast<float>(q) / static_cast<float>(mModeContext.mGameState.getAICountInRace()), "ai model loaded");
    }

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] after AI init loop\n");
#endif

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initModel]: Exit");

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[RACE] initModel exit\n");
#endif

}

void BaseRaceMode::initMisc()
{

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initMisc]: Enter");

    //rear mirror camera
    mRearCamera = NULL;

    if(mModeContext.mGameState.getMirrorEnabled())
    {
#if defined(WII_NATIVE_ASSET_PIPELINE)
        mUIRace->setRearViewMirrorPanelShow(false);
#else

        //rear mirror texture
        //http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Basic+Tutorial+7
        if(mIsGlobalReset)
        {
            int mirrorSizeW = mLuaManager.ReadScalarInt("Scene.Mirror.RTTSize.w", mModeContext.mPipeline);
            int mirrorSizeH = mLuaManager.ReadScalarInt("Scene.Mirror.RTTSize.h", mModeContext.mPipeline);

            //add this as listener
            mUIRace->createRearViewMirrorPanelTexture(this, mModeContext.mRoot, mirrorSizeW, mirrorSizeH);
            mUIRace->setRearViewMirrorPanelMaterial("Test/RearViewMirror");
        }
        else
        {
            mUIRace->rearViewMirrorPanelTextureRemoveAllViewports();
        }


        mUIRace->setRearViewMirrorPanelShow(true);
#endif
    }
    else
    {
        mUIRace->setRearViewMirrorPanelShow(false);
    }

    //as long as last procedure before draw started
    mModeContext.mGameState.resetBeforeStartTimer();

    //arrow
    {
        Ogre::Entity * arrowEntity = mModelsPool.getArrow();
        if(!arrowEntity)
        {
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[RACE] arrow model missing, skipping arrow node\n");
#endif
            return;
        }
        mModeContext.mGameState.setArrowNode(mSceneMgr->createSceneNode(arrowEntity->getName()));
        mModeContext.mGameState.getArrowNode()->attachObject(arrowEntity);
        mModeContext.mTrayMgr->getTraysLayer()->add3D(mModeContext.mGameState.getArrowNode());
        mModeContext.mGameState.getArrowNode()->setPosition(-0.75f, 0.55f, -1.0f);
        mModeContext.mGameState.getArrowNode()->setScale(0.20f, 0.20f, 0.20f);
        if (!mModeContext.mGameState.isStuntTrack() &&
            !mModeContext.mGameState.isFoxnhound1Track() &&
            !mModeContext.mGameState.isFoxnhound2Track()
            )
            mModeContext.mGameState.getArrowNode()->setVisible(true);
        else
            mModeContext.mGameState.getArrowNode()->setVisible(false);
    }


#if defined(__ANDROID__)
    //arrow helpers
    {
        {
            Ogre::Entity * arrowEntity = mModelsPool.getArrow();
            Ogre::MeshPtr msh = arrowEntity->getMesh();
            Ogre::MeshPtr cloned = msh->clone(msh->getName() + "_helperLeft", TEMP_RESOURCE_GROUP_NAME);
            Ogre::Entity * clonedArrow = mSceneMgr->createEntity(msh->getName() + "_helperLeft", msh->getName() + "_helperLeft", TEMP_RESOURCE_GROUP_NAME);
            clonedArrow->setCastShadows(false);
            mModeContext.mGameState.setArrowNodeHelperLeft(mSceneMgr->createSceneNode(clonedArrow->getName()));
            mModeContext.mGameState.getArrowNodeHelperLeft()->attachObject(clonedArrow);
            mModeContext.mTrayMgr->getTraysLayer()->add3D(mModeContext.mGameState.getArrowNodeHelperLeft());
            Ogre::Quaternion rot;
            Ogre::Quaternion rotA;
            rotA.FromAngleAxis(Ogre::Degree(-90.0f), Ogre::Vector3::UNIT_Z);
            rot.FromAngleAxis(Ogre::Degree(90.0f), Ogre::Vector3::UNIT_Y);
            rot = rot * rotA;
            mModeContext.mGameState.getArrowNodeHelperLeft()->setOrientation(rot);
            mModeContext.mGameState.getArrowNodeHelperLeft()->setPosition(-0.75f, -0.2f, -1.0f);
            mModeContext.mGameState.getArrowNodeHelperLeft()->setScale(0.20f, 0.20f, 0.20f);
            mModeContext.mGameState.getArrowNodeHelperLeft()->setVisible(true);
        }
        {
            Ogre::Entity * arrowEntity = mModelsPool.getArrow();
            Ogre::MeshPtr msh = arrowEntity->getMesh();
            Ogre::MeshPtr cloned = msh->clone(msh->getName() + "_helperRight", TEMP_RESOURCE_GROUP_NAME);
            Ogre::Entity * clonedArrow = mSceneMgr->createEntity(msh->getName() + "_helperRight", msh->getName() + "_helperLeft", TEMP_RESOURCE_GROUP_NAME);
            clonedArrow->setCastShadows(false);
            mModeContext.mGameState.setArrowNodeHelperRight(mSceneMgr->createSceneNode(clonedArrow->getName()));
            mModeContext.mGameState.getArrowNodeHelperRight()->attachObject(clonedArrow);
            mModeContext.mTrayMgr->getTraysLayer()->add3D(mModeContext.mGameState.getArrowNodeHelperRight());
            Ogre::Quaternion rot;
            Ogre::Quaternion rotA;
            rotA.FromAngleAxis(Ogre::Degree(-90.0f), Ogre::Vector3::UNIT_Z);
            rot.FromAngleAxis(Ogre::Degree(-90.0f), Ogre::Vector3::UNIT_Y);
            rot = rot * rotA;
            mModeContext.mGameState.getArrowNodeHelperRight()->setOrientation(rot);
            mModeContext.mGameState.getArrowNodeHelperRight()->setPosition(0.75f, -0.2f, -1.0f);
            mModeContext.mGameState.getArrowNodeHelperRight()->setScale(0.20f, 0.20f, 0.20f);
            mModeContext.mGameState.getArrowNodeHelperRight()->setVisible(true);
        }
        {
            Ogre::Entity * arrowEntity = mModelsPool.getArrow();
            Ogre::MeshPtr msh = arrowEntity->getMesh();
            Ogre::MeshPtr cloned = msh->clone(msh->getName() + "_helperBottom", TEMP_RESOURCE_GROUP_NAME);
            Ogre::Entity * clonedArrow = mSceneMgr->createEntity(msh->getName() + "_helperBottom", msh->getName() + "_helperLeft", TEMP_RESOURCE_GROUP_NAME);
            clonedArrow->setCastShadows(false);
            mModeContext.mGameState.setArrowNodeHelperBottom(mSceneMgr->createSceneNode(clonedArrow->getName()));
            mModeContext.mGameState.getArrowNodeHelperBottom()->attachObject(clonedArrow);
            mModeContext.mTrayMgr->getTraysLayer()->add3D(mModeContext.mGameState.getArrowNodeHelperBottom());
            Ogre::Quaternion rot;
            rot.FromAngleAxis(Ogre::Degree(-90.0f), Ogre::Vector3::UNIT_X);
            rot = rot;
            mModeContext.mGameState.getArrowNodeHelperBottom()->setOrientation(rot);
            mModeContext.mGameState.getArrowNodeHelperBottom()->setPosition(0.0f, -0.75f, -1.0f);
            mModeContext.mGameState.getArrowNodeHelperBottom()->setScale(0.20f, 0.20f, 0.20f);
            mModeContext.mGameState.getArrowNodeHelperBottom()->setVisible(true);
        }
    }
#endif

    mIsGlobalReset = false;

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initMisc]: Exit");
}



void BaseRaceMode::initWorld()
{
    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initWorld]: Enter");

    const Ogre::Vector2& fogStartEnd = mModeContext.mGameState.getSTRPowerslide().getFogStartEnd(mModeContext.mGameState.getTrackName());
    bool isFogEnabled = fogStartEnd.x >= 1000000.0f ? false : true;

    mWorld = std::make_shared<Physics>(&mStaticMeshProcesser);
    mWorld->addListener(this);

    mCheats = std::make_shared<Cheats>(&mStaticMeshProcesser, mSceneMgr, mWorld.get(), isFogEnabled
#ifndef NO_OPENAL
        , &mModeContext.getSoundsProcesser()
#endif
        );

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::initWorld]: Exit");
}

void BaseRaceMode::deInitWorld()
{
    mCheats.reset();

    mStaticMeshProcesser.deinit();

    mWorld.reset();
}

void BaseRaceMode::frameStarted(const Ogre::FrameEvent &evt)
{

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[BaseRaceMode::frameStarted]: Enter");

#if defined(WII) || defined(__wii__)
    static unsigned int sWiiMemLogFrame = 0;
    ++sWiiMemLogFrame;
    if((sWiiMemLogFrame % 120u) == 0u)
    {
        const unsigned int arenaLo = reinterpret_cast<unsigned int>(SYS_GetArena2Lo());
        const unsigned int arenaHi = reinterpret_cast<unsigned int>(SYS_GetArena2Hi());
        const unsigned int arenaFree = (arenaHi > arenaLo) ? (arenaHi - arenaLo) : 0u;
        const unsigned int tracked = static_cast<unsigned int>(Wii_GetAllocatedBytes());
        WiiDebugLog("[MEM] frame=%u tracked=%u arena2_free=%u lo=%08X hi=%08X\n",
            sWiiMemLogFrame,
            tracked,
            arenaFree,
            arenaLo,
            arenaHi);
    }
#endif

    PhysicsVehicle* playerPhysics = mModeContext.mGameState.getPlayerCar().getPhysicsVehicle();
    if(!playerPhysics)
    {
#if defined(WII) || defined(__wii__)
        static bool sTrackOverlayUploaded = false;
        static size_t sTrackOverlayPoints = 0;
        if(!sTrackOverlayUploaded)
        {
            const std::vector<Ogre::Vector3>& trackPts = mModeContext.mGameState.getPlayerCar().getLapUtils().getTrackPositionsDebug();
            sTrackOverlayPoints = trackPts.size();
            if(sTrackOverlayPoints >= 2)
            {
                std::vector<unsigned char> be;
                be.resize(sTrackOverlayPoints * 6);
                for(size_t i = 0; i < sTrackOverlayPoints; ++i)
                {
                    const Ogre::Vector3& p = trackPts[i];
                    const short sx = static_cast<short>(p.x);
                    const short sy = static_cast<short>(p.y + 20.0f);
                    const short sz = static_cast<short>(p.z);
                    const size_t o = i * 6;
                    be[o + 0] = static_cast<unsigned char>((sx >> 8) & 0xFF);
                    be[o + 1] = static_cast<unsigned char>(sx & 0xFF);
                    be[o + 2] = static_cast<unsigned char>((sy >> 8) & 0xFF);
                    be[o + 3] = static_cast<unsigned char>(sy & 0xFF);
                    be[o + 4] = static_cast<unsigned char>((sz >> 8) & 0xFF);
                    be[o + 5] = static_cast<unsigned char>(sz & 0xFF);
                }

                if(WiiGX::Renderer::getInstance().uploadTrackBufferBE(be.data(), static_cast<u32>(sTrackOverlayPoints)))
                {
                    sTrackOverlayUploaded = true;
                    WiiDebugLog("[GXDBG] uploaded track overlay points=%u\n", static_cast<unsigned int>(sTrackOverlayPoints));
                }
                else
                {
                    WiiDebugLog("[GXDBG] failed upload track overlay points=%u\n", static_cast<unsigned int>(sTrackOverlayPoints));
                }
            }
            else
            {
                WiiDebugLog("[GXDBG] insufficient track points for overlay=%u\n", static_cast<unsigned int>(sTrackOverlayPoints));
            }
        }

        #if defined(WII_NATIVE_ASSET_PIPELINE)
        WiiGX::Renderer::getInstance().renderTerrainBuffer();
        #else
        if(sTrackOverlayUploaded)
            WiiGX::Renderer::getInstance().renderTrackBuffer();
        #endif

        WiiDebugLog("[RACE] frameStarted: player physics is null, skipping frame\n");
#endif
        return;
    }

    if(playerPhysics->isNitro())
    {
        //mModeContext.mGameState.getPlayerCar().getAlignedVelocitySpeedometer()
        mCamera->setFOVy(Ogre::Degree(mFOVNitro.getVal(playerPhysics->getNitroFrames())));
    }
    else
    {
        mCamera->setFOVy(Ogre::Degree(90.0f));
    }

#if !defined(WII_NATIVE_ASSET_PIPELINE)
    mUIRace->setPointerPosition(playerPhysics->getOriginalSteering(), mModeContext.mGameState.getPlayerCar().getBrake());
#endif

    mWorld->timeStep(mModeContext.mGameState);

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[BaseRaceMode::frameStarted]: Exit");
}

void BaseRaceMode::frameRenderingQueued(const Ogre::FrameEvent& evt)
{

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[BaseRaceMode::frameRenderingQueued]: Enter");

#if !defined(WII_NATIVE_ASSET_PIPELINE)
    mUIRace->setMiscTextRight(Conversions::DMToString(static_cast<size_t>(mModeContext.mWindow->getAverageFPS())));
#endif

    customFrameRenderingQueuedDoBegining();

#if !defined(WII_NATIVE_ASSET_PIPELINE)
    if(mModeContext.getGameModeSwitcher()->getMode() != ModeRaceTimetrial)
    {
        beforeStartSequence();
    }
    else
    {
        if(!mModeContext.mGameState.getRaceStarted() && !mModeContext.mGameState.isGamePaused())
        {
            mModeContext.mGameState.getPlayerCar().raceStarted();
            mModeContext.mGameState.getPlayerCar().getLapUtils().resetLapTimer();
            mModeContext.mGameState.setRaceStarted(true);
        }
    }
#endif

#if defined(__ANDROID__)
    //arrow helpers
    if (mModeContext.mGameState.getBeforeStartTimerTime() > mModeContext.mGameState.getSTRRacecrud().getFloatValue("on-grid parameters", "set left start") * 1000.0f)
    {
        mUIRace->hideAllHelpers();
        mModeContext.mGameState.getArrowNodeHelperLeft()->setVisible(false);
        mModeContext.mGameState.getArrowNodeHelperRight()->setVisible(false);
        mModeContext.mGameState.getArrowNodeHelperBottom()->setVisible(false);
    }
#endif

    mModeContext.mInputHandler->capture();

    mModeContext.mTrayMgr->frameRenderingQueued(evt);

#if !defined(WII_NATIVE_ASSET_PIPELINE)
    mUIRace->setShowPausedPanel(mModeContext.mGameState.isGamePaused());
#endif

    if (!mModeContext.mTrayMgr->isDialogVisible())
    {
#if !defined(WII_NATIVE_ASSET_PIPELINE)
        if(!mModeContext.mGameState.isGamePaused())
            mUIRace->setEngineRPM(mWorld->getVehicle(&mModeContext.mGameState.getPlayerCar())->getCarEngine().getEngineRPM());
        mUIRace->setCarSpeed(mModeContext.mGameState.getPlayerCar().getAlignedVelocitySpeedometer());
        mUIRace->setCurrentLap(static_cast<unsigned short>(mModeContext.mGameState.getPlayerCar().getLapUtils().getCurrentLap()), static_cast<unsigned short>(mModeContext.mGameState.getLapsCount()));
        if(mModeContext.mGameState.getRaceStarted())
        {
            mUIRace->setCarGear(static_cast<unsigned char>(mWorld->getVehicle(&mModeContext.mGameState.getPlayerCar())->getCarEngine().getCurrentGear()));
        }
        else
        {
            mUIRace->setCarGear(1);
        }
        mUIRace->setCarPos(static_cast<unsigned char>(mLapController.getTotalPosition(0)), static_cast<unsigned char>(mLapController.getTotalCars()));

        mUIRace->hideAIDashboardCars();
        size_t currentPlayerLap = mModeContext.mGameState.getPlayerCar().getLapUtils().getCurrentLap();
        Ogre::Real currentPlayerLapPos = mModeContext.mGameState.getPlayerCar().getLapUtils().getLapPosition();
        mUIRace->setPlayerDashBoardSkin(mModeContext.mGameState);
        for(size_t q = 0; q < mModeContext.mGameState.getAICountInRace(); ++q)
        {
            mUIRace->setAIDashBoardSkin(mModeContext.mGameState, q, mModeContext.mGameState.getAICar(q).getCharacterName());
            mUIRace->setDashCarPos(q, currentPlayerLap, currentPlayerLapPos, mModeContext.mGameState.getAICar(q).getLapUtils().getCurrentLap(), mModeContext.mGameState.getAICar(q).getLapUtils().getLapPosition());
        }

        customFrameRenderingQueuedDo2DUI();

        if(mModeContext.mGameState.getRaceStarted())
        {
            if(mModeContext.mGameState.getPlayerCar().getLapUtils().getAfterFinishLinePassTime() < 3.0f && mModeContext.mGameState.getPlayerCar().getLapUtils().getCurrentLap() > 1)
            {
                mUIRace->setRaceTime(Tools::SecondsToString(mModeContext.mGameState.getPlayerCar().getLapUtils().getLastLapTime()));
            }
            else
            {
                mUIRace->setRaceTime(Tools::SecondsToString(mModeContext.mGameState.getPlayerCar().getLapUtils().getLapTime()));
            }
        }
        else
        {
            mUIRace->setRaceTime("00:00:00");
        }


#if SHOW_DETAILS_PANEL
        //mDetailsPanel->setParamValue(0, Ogre::StringConverter::toString(mCamera->getDerivedPosition().x));
        //mDetailsPanel->setParamValue(1, Ogre::StringConverter::toString(mCamera->getDerivedPosition().y));
        //mDetailsPanel->setParamValue(2, Ogre::StringConverter::toString(mCamera->getDerivedPosition().z));
        mDetailsPanel->setParamValue(0, Ogre::StringConverter::toString(mModeContext.mGameState.getPlayerCar().getModelNode()->getPosition().x));
        mDetailsPanel->setParamValue(1, Ogre::StringConverter::toString(mModeContext.mGameState.getPlayerCar().getModelNode()->getPosition().y));
        mDetailsPanel->setParamValue(2, Ogre::StringConverter::toString(mModeContext.mGameState.getPlayerCar().getModelNode()->getPosition().z));
        
        mDetailsPanel->setParamValue(4, "");//Ogre::StringConverter::toString(mModeContext.mGameState.getPlayerCar().getLinearVelocity().length()));
        mDetailsPanel->setParamValue(5, Ogre::StringConverter::toString(mModeContext.mGameState.getPlayerCar().getAlignedVelocity()));
        mDetailsPanel->setParamValue(6, "");//Ogre::StringConverter::toString(mModeContext.mGameState.getPlayerCar().getLateralVelocity()));
#if 0
        mDetailsPanel->setParamValue(4, Ogre::StringConverter::toString(mModeContext.mGameState.getAICar(0].getLinearVelocity().length()));
        mDetailsPanel->setParamValue(5, Ogre::StringConverter::toString(mModeContext.mGameState.getAICar(0].getAlignedVelocity()));
        mDetailsPanel->setParamValue(6, Ogre::StringConverter::toString(mModeContext.mGameState.getAICar(0].getLateralVelocity()));
#endif

        mDetailsPanel->setParamValue(8, mModeContext.mGameState.getPlayerCar().getFrontLWheelColliderString());
        mDetailsPanel->setParamValue(9, mModeContext.mGameState.getPlayerCar().getFrontRWheelColliderString());
        mDetailsPanel->setParamValue(10, mModeContext.mGameState.getPlayerCar().getBackLWheelColliderString());
        mDetailsPanel->setParamValue(11, mModeContext.mGameState.getPlayerCar().getBackRWheelColliderString());
#if 0
        mDetailsPanel->setParamValue(8, mModeContext.mGameState.getAICar(0].getFrontLWheelColliderString());
        mDetailsPanel->setParamValue(9, mModeContext.mGameState.getAICar(0].getFrontRWheelColliderString());
        mDetailsPanel->setParamValue(10, mModeContext.mGameState.getAICar(0].getBackLWheelColliderString());
        mDetailsPanel->setParamValue(11, mModeContext.mGameState.getAICar(0].getBackRWheelColliderString());
#endif

        mDetailsPanel->setParamValue(13, "");//Ogre::StringConverter::toString(mStaticMeshProcesser.getLongtitudeFriction(mModeContext.mGameState.getPlayerCar().getFrontLWheelColliderIndex())));
        mDetailsPanel->setParamValue(14, "");//Ogre::StringConverter::toString(mStaticMeshProcesser.getLongtitudeFriction(mModeContext.mGameState.getPlayerCar().getFrontRWheelColliderIndex())));
        mDetailsPanel->setParamValue(15, "");//Ogre::StringConverter::toString(mStaticMeshProcesser.getLongtitudeFriction(mModeContext.mGameState.getPlayerCar().getBackLWheelColliderIndex())));
        mDetailsPanel->setParamValue(16, "");//Ogre::StringConverter::toString(mStaticMeshProcesser.getLongtitudeFriction(mModeContext.mGameState.getPlayerCar().getBackRWheelColliderIndex())));
#if 0
        mDetailsPanel->setParamValue(13, Ogre::StringConverter::toString(mStaticMeshProcesser.getLongtitudeFriction(mModeContext.mGameState.getAICar(0].getFrontLWheelColliderIndex())));
        mDetailsPanel->setParamValue(14, Ogre::StringConverter::toString(mStaticMeshProcesser.getLongtitudeFriction(mModeContext.mGameState.getAICar(0].getFrontRWheelColliderIndex())));
        mDetailsPanel->setParamValue(15, Ogre::StringConverter::toString(mStaticMeshProcesser.getLongtitudeFriction(mModeContext.mGameState.getAICar(0].getBackLWheelColliderIndex())));
        mDetailsPanel->setParamValue(16, Ogre::StringConverter::toString(mStaticMeshProcesser.getLongtitudeFriction(mModeContext.mGameState.getAICar(0].getBackRWheelColliderIndex())));
#endif

        mDetailsPanel->setParamValue(18, "");//Ogre::StringConverter::toString(mStaticMeshProcesser.getLatitudeFriction(mModeContext.mGameState.getPlayerCar().getFrontLWheelColliderIndex())));
        mDetailsPanel->setParamValue(19, "");//Ogre::StringConverter::toString(mStaticMeshProcesser.getLatitudeFriction(mModeContext.mGameState.getPlayerCar().getFrontRWheelColliderIndex())));
        mDetailsPanel->setParamValue(20, "");//Ogre::StringConverter::toString(mStaticMeshProcesser.getLatitudeFriction(mModeContext.mGameState.getPlayerCar().getBackLWheelColliderIndex())));
        mDetailsPanel->setParamValue(21, "");//Ogre::StringConverter::toString(mStaticMeshProcesser.getLatitudeFriction(mModeContext.mGameState.getPlayerCar().getBackRWheelColliderIndex())));
#if 0
        mDetailsPanel->setParamValue(18, Ogre::StringConverter::toString(mStaticMeshProcesser.getLatitudeFriction(mModeContext.mGameState.getAICar(0].getFrontLWheelColliderIndex())));
        mDetailsPanel->setParamValue(19, Ogre::StringConverter::toString(mStaticMeshProcesser.getLatitudeFriction(mModeContext.mGameState.getAICar(0].getFrontRWheelColliderIndex())));
        mDetailsPanel->setParamValue(20, Ogre::StringConverter::toString(mStaticMeshProcesser.getLatitudeFriction(mModeContext.mGameState.getAICar(0].getBackLWheelColliderIndex())));
        mDetailsPanel->setParamValue(21, Ogre::StringConverter::toString(mStaticMeshProcesser.getLatitudeFriction(mModeContext.mGameState.getAICar(0].getBackRWheelColliderIndex())));
#endif

        mDetailsPanel->setParamValue(23, Ogre::StringConverter::toString(mModeContext.mGameState.getPlayerCar().getLapUtils().getLapPosition()));
        mDetailsPanel->setParamValue(24, Tools::SecondsToString(mModeContext.mGameState.getPlayerCar().getLapUtils().getLastLapTime()));
        mDetailsPanel->setParamValue(25, Tools::SecondsToString(mModeContext.mGameState.getPlayerCar().getLapUtils().getLapTime()));
        mDetailsPanel->setParamValue(26, Ogre::StringConverter::toString(mModeContext.mGameState.getPlayerCar().getLapUtils().getCurrentLap()));
        mDetailsPanel->setParamValue(27, Ogre::StringConverter::toString(mLapController.getTotalPosition(0)));
        mDetailsPanel->setParamValue(28, Ogre::StringConverter::toString(mModeContext.mGameState.getPlayerCar().getPhysicsVehicle()->getCarEngine().getCurrentGear()));
        mDetailsPanel->setParamValue(29, Ogre::StringConverter::toString(mModeContext.mGameState.getPlayerCar().getPhysicsVehicle()->getCarEngine().getEngineRPM()));
#endif
#endif
    }

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[BaseRaceMode::frameRenderingQueued]: Exit");
}

void BaseRaceMode::beforeStartSequence()
{
    if( !mModeContext.mGameState.getRaceStarted() && 
        mModeContext.mGameState.getBeforeStartTimerTime() > mModeContext.mGameState.getSTRRacecrud().getFloatValue("on-grid parameters", "ready left start") * 1000.0f && 
        !mModeContext.mGameState.isGamePaused())
    {
        mUIRace->setRearViewMirrorPanelShow(false);
        mUIRace->hideAllStart();
        mUIRace->showBeforeStart1();
#ifndef NO_OPENAL
        mModeContext.mSoundsProcesser.playBeforeStart1();
#endif
    }

    if(!mModeContext.mGameState.getRaceStarted() && 
        mModeContext.mGameState.getBeforeStartTimerTime() > mModeContext.mGameState.getSTRRacecrud().getFloatValue("on-grid parameters", "set left start") * 1000.0f && 
        !mModeContext.mGameState.isGamePaused())
    {
        mUIRace->hideAllStart();
        mUIRace->showBeforeStart2();
#ifndef NO_OPENAL
        mModeContext.mSoundsProcesser.playBeforeStart2();
#endif
    }

    if(!mModeContext.mGameState.getRaceStarted() && 
        mModeContext.mGameState.getBeforeStartTimerTime() > mModeContext.mGameState.getSTRRacecrud().getFloatValue("on-grid parameters", "go left start") * 1000.0f && 
        !mModeContext.mGameState.isGamePaused())
    {
        mUIRace->hideAllStart();
        mUIRace->showBeforeStart3();
#ifndef NO_OPENAL
        mModeContext.mSoundsProcesser.playBeforeStart3();
#endif

        mModeContext.mGameState.getPlayerCar().raceStarted();
        for(size_t q = 0; q < mModeContext.mGameState.getAICountInRace(); ++q)
        {
            mModeContext.mGameState.getAICar(q).raceStarted();
        }

        customFrameRenderingQueuedDoRaceStarted();

        //reset lap timer before race started (for every human player include self, and ai)
        mModeContext.mGameState.getPlayerCar().getLapUtils().resetLapTimer();
        for(int q = 0; q < mModeContext.mGameState.getMaxMultiplayerHumans(); ++q)
        {
            mModeContext.mGameState.getMultiplayerCarHuman(q).getLapUtils().resetLapTimer();
        }
        for(size_t q = 0; q < mModeContext.mGameState.getAICountInRace(); ++q)
        {
            mModeContext.mGameState.getAICar(q).getLapUtils().resetLapTimer();
        }

        mModeContext.mGameState.setRaceStarted(true);
    }

    if(mModeContext.mGameState.getBeforeStartTimerTime() > (mModeContext.mGameState.getSTRRacecrud().getFloatValue("on-grid parameters", "go left start") * 1000.0f + mModeContext.mGameState.getSTRRacecrud().getFloatValue("on-grid parameters", "go left length") * 1000.0f) && !mModeContext.mGameState.isGamePaused())
    {
        mUIRace->hideAllStart();

        if(mModeContext.mGameState.getMirrorEnabled() && !mModeContext.mGameState.getRaceFinished())
        {
            mUIRace->setRearViewMirrorPanelShow(true);
        }
    }
}

void BaseRaceMode::timeStepBefore(Physics * physics)
{
    Ogre::SceneNode* playerNode = mModeContext.mGameState.getPlayerCar().getModelNode();
    if(!playerNode)
    {
#if defined(WII) || defined(__wii__)
        WiiDebugLog("[RACE] timeStepBefore: player model node is null, skipping frame\n");
#endif
        return;
    }

    Ogre::Vector3 playerPos = playerNode->getPosition();

    Ogre::Vector3 playerDir = mModeContext.mGameState.getPlayerCar().getForwardAxis();

#ifndef NO_OPENAL
    mModeContext.mSoundsProcesser.setListenerPos(playerPos, mModeContext.mGameState.getPlayerCar().getUpAxis(), playerDir);
    if(!mModeContext.mGameState.isGamePaused())
    {
        if(mModeContext.mMusicProcessor.isPaused())
        {
            mModeContext.mMusicProcessor.play();
        }

        if(mModeContext.mSoundsProcesser.getListenerGain() == 0.0f)
        {
            mModeContext.mSoundsProcesser.setListenerGain(1.0f);
        }
    }
    else
    {
        if(!mModeContext.mMusicProcessor.isPaused())
        {
            mModeContext.mMusicProcessor.pause();
        }

        if(mModeContext.mSoundsProcesser.getListenerGain() == 1.0f)
        {
            mModeContext.mSoundsProcesser.setListenerGain(0.0f);
        }
    }
#endif

    if(!mModeContext.mGameState.getRaceFinished())
        mModeContext.mGameState.getPlayerCar().getLapUtils().checkCheckPoints(playerPos, mModeContext.mGameState.getSpeedCheatUsed());

    mModeContext.mGameState.getArrowNode()->setOrientation(mModeContext.mGameState.getPlayerCar().getLapUtils().getArrowOrientation(playerPos, playerDir));

    if(!mModeContext.mGameState.isGamePaused())
    {
        mModeContext.mGameState.getPlayerCar().processFrameBeforePhysics(mStaticMeshProcesser, mModeContext.mGameState.getRaceStarted());

        for(size_t q = 0; q < mModeContext.mGameState.getAICountInRace(); ++q)
        {
            Ogre::SceneNode* aiNode = mModeContext.mGameState.getAICar(q).getModelNode();
            if(!aiNode)
            {
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[RACE] timeStepBefore: ai idx=%u model node is null, skipping lap check\n", static_cast<unsigned int>(q));
#endif
            }
            else if(!mModeContext.mGameState.getRaceFinished())
            {
                mModeContext.mGameState.getAICar(q).getLapUtils().checkCheckPoints(aiNode->getPosition(), false);
            }

            mModeContext.mGameState.getAICar(q).processFrameBeforePhysics(mStaticMeshProcesser, mModeContext.mGameState.getRaceStarted());
        }

        customFrameStartedDoProcessFrameBeforePhysics();
    }

    mLapController.calculateLapPositions();
}

void BaseRaceMode::timeStepAfter(Physics * physics)
{
    if(!mModeContext.mGameState.isGamePaused())
    {
        mModeContext.mGameState.getPlayerCar().processSounds();
        processSounds();
    }


    if(mModeContext.getGameModeSwitcher()->getMode() == ModeRaceTimetrial)
    {
        if(!mModeContext.mGameState.isGamePaused() && mModeContext.mGameState.getRaceStarted())
        {
            Ogre::Real lapTime = mModeContext.mGameState.getPlayerCar().getLapUtils().getLapTimer().getMilliseconds() / 1000.0f;
            GhostPos ghostPos(mModeContext.mGameState.getPlayerCar());
            mTrialGhost.storePoint(ghostPos, lapTime);

            if(mTrialGhost.isVisible() && mModeContext.mGameState.getGhostEnabled())
            {
                GhostPos ghostPoint = mTrialGhost.getInterpolatedPoint(lapTime);
                if(mGhostVisible)
                {
                    mGhost.repositionVehicle(ghostPoint.chassisPos, ghostPoint.chassisRot, ghostPoint.wheelPos, ghostPoint.wheelRot);
                    if(mModeContext.mGameState.getGhostVisibility())
                    {
                        mGhost.setVisibility(true);
                    }
                }
                if(mGhostUserVisible)
                {
                    mGhostUser.repositionVehicle(ghostPoint.chassisPos, ghostPoint.chassisRot, ghostPoint.wheelPos, ghostPoint.wheelRot);
                    if(mModeContext.mGameState.getGhostVisibility())
                    {
                        mGhostUser.setVisibility(true);
                    }
                }

                if(!mModeContext.mGameState.getGhostVisibility())
                {
                    mGhost.setVisibility(false);
                    mGhostUser.setVisibility(false);
                }
            }
        }
    }

    customFrameStartedDoProcessFrameAfterPhysics();
}


#if defined(__ANDROID__)
void BaseRaceMode::reloadTextures()
{
    TEXLoader().load(mModeContext.mGameState.getPFLoaderData(), "data/deii", "d3d2display1_m_1.tex", "OriginalParticle", mModeContext.mGameState.getX2LUTs(), mModeContext.mGameState.getX4LUTs());
    TEXLoader().load(mModeContext.mGameState.getPFLoaderData(), "data/misc", "arrow_m_3.tex", "OriginalArrow", mModeContext.mGameState.getX2LUTs(), mModeContext.mGameState.getX4LUTs());

    mModeContext.mGameState.getPlayerCar().reloadTextures(mModeContext.mGameState);

    for(size_t q = 0; q < mModeContext.mGameState.getAICountInRace(); ++q)
    {
        mModeContext.mGameState.getAICar(q).reloadTextures(mModeContext.mGameState);
    }

    std::string pfFolderName = mModeContext.mGameState.getSTRPowerslide().getBaseDir(mModeContext.mGameState.getTrackName());
    mStaticMeshProcesser.loadTextures(mModeContext.mGameState, mModeContext.mGameState.getPFLoaderData(), pfFolderName, mModeContext.mGameState.getGamma(), mModeContext.mGameState.getDoUpscale(), NULL);

    mUIRace->reloadTextures(mModeContext.mGameState);
}
#endif

void BaseRaceMode::processSounds()
{
    //customProcessCollision(triIndex);

#ifndef NO_OPENAL
    //surface sound
    bool isSoundPlay = false;
    if(mModeContext.mGameState.getPlayerCar().getPhysicsVehicle()->getFrontCollision() || mModeContext.mGameState.getPlayerCar().getPhysicsVehicle()->getBackCollision())
    {
        //if(mModeContext.mGameState.getPlayerCar().getAcceleration() || mModeContext.mGameState.getPlayerCar().getBrake())
        {
            char backLWheel = mModeContext.mGameState.getPlayerCar().getPhysicsVehicle()->getWheelBackLTerrainIndex();
            if(backLWheel != -1)
            {
                isSoundPlay = true;
                Ogre::Real velDiff = Ogre::Math::Abs(
                    Ogre::Math::Abs(mModeContext.mGameState.getPlayerCar().getPhysicsVehicle()->getWheelBackLVelocity()) - 
                    Ogre::Math::Abs(mModeContext.mGameState.getPlayerCar().getPhysicsVehicle()->getAlignedVelocitySpeedometer())
                    );
                
                mModeContext.mSoundsProcesser.playSurface(backLWheel, velDiff, Ogre::Math::Abs(mModeContext.mGameState.getPlayerCar().getPhysicsVehicle()->getAlignedVelocitySpeedometer()));
            }
        }
    }
    if(!isSoundPlay)
    {
        mModeContext.mSoundsProcesser.stopSoundSurfaces();
    }
#if 0
    //surface crash sound
    Ogre::Vector3 playerVel = mModeContext.mGameState.getPlayerCar().getLinearVelocitySpeedometer();
    Ogre::Real playerVelDiff = playerVel.distance(mModeContext.mGameState.getPLayerCarPrevVel());
    mModeContext.mGameState.setPLayerCarPrevVel(playerVel);
    if(playerVelDiff > 10.0f)
    {
        if(mModeContext.mGameState.getPlayerCar().checkFrontCollision() || mModeContext.mGameState.getPlayerCar().checkRearCollision() || mModeContext.mGameState.getPlayerCar().checkChassisCollision())
        {

            unsigned char collisionSurface = 0;

            if(mModeContext.mGameState.getPlayerCar().checkFrontCollision())
            {
                collisionSurface = mModeContext.mGameState.getPlayerCar().getFrontLWheelColliderIndex();
            }

            if(mModeContext.mGameState.getPlayerCar().checkRearCollision())
            {
                collisionSurface = mModeContext.mGameState.getPlayerCar().getBackLWheelColliderIndex();
            }

            if(mModeContext.mGameState.getPlayerCar().checkChassisCollision())
            {
                collisionSurface = mModeContext.mGameState.getPlayerCar().getChassisColliderIndex();
            }

            mModeContext.mSoundsProcesser.playSurfaceCrash(collisionSurface);
        }
    }

    //ai with player crash sounds
    for(size_t q = 0; q < mModeContext.mGameState.getAICountInRace(); ++q)
    {
        if(mModeContext.mGameState.getPlayerCar().isCollideChassis(mModeContext.mGameState.getAICar(q)))
        {
            Ogre::Vector3 aiVel = mModeContext.mGameState.getAICar(q).getLinearVelocitySpeedometer();
            Ogre::Real velDiff = playerVel.distance(aiVel);
            if(velDiff > 20.0f && velDiff <= 50.0f)
            {
                mModeContext.mSoundsProcesser.playCrash1();
                break;
            }
            else if(velDiff > 50.0f)
            {
                mModeContext.mSoundsProcesser.playCrash2();
                break;
            }
            
        }
    }
#endif
#endif
}

void BaseRaceMode::loadResources()
{
    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::loadResources]: Enter");

#if defined(__ANDROID__)
    LOGI("BaseApp[loadResources]: Begin"); 
#endif

    Ogre::ResourceGroupManager::getSingleton().createResourceGroup(TEMP_RESOURCE_GROUP_NAME);

    //http://ogre3d.org/forums/viewtopic.php?f=2&t=54469
    //mModeContext.mTrayMgr->showLoadingBar(2, 2);
    //Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

    Ogre::ResourceGroupManager::getSingleton().addResourceGroupListener(this);

    Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Popular");
    Ogre::ResourceGroupManager::getSingleton().loadResourceGroup("Popular");

    Ogre::ResourceGroupManager::getSingleton().removeResourceGroupListener(this);


    //mModeContext.mTrayMgr->hideLoadingBar();

#if defined(__ANDROID__)
    LOGI("BaseApp[loadResources]: End"); 
#endif

    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[BaseRaceMode::loadResources]: Exit");
}

void BaseRaceMode::resourceGroupScriptingStarted(const Ogre::String& groupName, size_t scriptCount)
{
}

void BaseRaceMode::scriptParseEnded(const Ogre::String& scriptName, bool skipped)
{
}

void BaseRaceMode::resourceGroupLoadStarted(const Ogre::String& groupName, size_t resourceCount)
{
}

void BaseRaceMode::resourceLoadEnded()
{
}

void BaseRaceMode::unloadResources()
{

    mUIRace->setRearViewMirrorPanelMaterial("BaseWhite");
    mUIRace->setRearViewMirrorPanelShow(false);

    //deinit sounds of system

    //clear sounds of cars as well
    mModeContext.mGameState.getPlayerCar().deinitSounds();
    for(size_t q = 0; q < mModeContext.mGameState.getAICountInRace(); ++q)
        mModeContext.mGameState.getAICar(q).deinitSounds();

    customUnloadResources();

#ifndef NO_OPENAL
    mModeContext.mSoundsProcesser.stopSounds();
#endif

    Ogre::ParticleSystemManager::getSingleton().removeAllTemplates();

    Ogre::ResourceGroupManager::getSingleton().clearResourceGroup("Popular");

    Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup(TEMP_RESOURCE_GROUP_NAME);

    //Ogre::MaterialManager::getSingleton().removeUnreferencedResources();
    //Ogre::MaterialManager::getSingleton().initialise();
}

void BaseRaceMode::onLapFinished()
{
    GameState& gameState = mModeContext.getGameState();

    size_t lap = gameState.getPlayerCar().getLapUtils().getCurrentLap() - 1;

    if(lap <= gameState.getLapsCount() || mModeContext.getGameModeSwitcher()->getMode() == ModeRaceTimetrial)
    {
        Ogre::Real lastLapTime = gameState.getPlayerCar().getLapUtils().getLastLapTime();
        std::string time = Tools::SecondsToString(lastLapTime);
        std::string lapS = Conversions::DMToString(lap);
        mUIRace->addMiscPanelText("Lap finished " + Ogre::String(lapS) + ": [" + Ogre::String(time) + "]", gameState.getSTRPowerslide().getTrackTimeTrialColor(gameState.getTrackName()));

        if(mModeContext.getGameModeSwitcher()->getMode() == ModeRaceTimetrial)
        {
            Ogre::Real bestLapTime = gameState.getPlayerCar().getLapUtils().getBestLapTime();
            std::string bestTime = Tools::SecondsToString(bestLapTime);
            mUIRace->setMiscText(bestTime);
            mUIRace->setShowMiscText(true);

            if(gameState.getGhostEnabled())
            {
                bool isBestBeaten = mTrialGhost.lapFinished(bestLapTime, gameState.getDataDir(), gameState.isSupercar(), mGhostUser.getCharacterName());
                if(mTrialGhost.isVisible())
                {
                    if(isBestBeaten)
                    {
                        mGhostVisible = false;
                        mGhostUserVisible = true;
                        mGhost.setVisibility(false);
                        mGhostUser.setVisibility(true);
                    }
                    else
                    {
                        if(!mGhostUserVisible)
                        {
                            mGhostVisible = true;
                            mGhost.setVisibility(true);
                        }
                    }
                }
            }

            if (gameState.isLugeTrack())
            {
                gameState.getPlayerCar().getPhysicsVehicle()->repositionToStart();
            }
        }
    }

    
    if(mModeContext.getGameModeSwitcher()->getMode() != ModeRaceTimetrial)
    {
        //race finished
        if(lap == gameState.getLapsCount())
        {
            gameState.resetAfterFinishTimer();

            std::string totalTime = Tools::SecondsToString(gameState.getPlayerCar().getLapUtils().getTotalTime());
            mUIRace->addMiscPanelText("Race finished: [" + Ogre::String(totalTime) + "]", gameState.getSTRPowerslide().getTrackTimeTrialColor(gameState.getTrackName()));

            gameState.getPlayerCar().setDisableMouse(false);
            mCamera->setPosition(mModeContext.mGameState.getSTRPowerslide().getFinishCameraPos(gameState.getTrackName()));
            mUIRace->setRearViewMirrorPanelShow(false);
            mUIRace->setVisibleFinishSign(true, mLapController.getTotalPosition(0));
            mModeContext.mGameState.setRaceFinished(true);

            //update hiscores
            {
                bool isBestBeaten = gameState.updateHiscores(
                    gameState.getPlayerCar().getCharacterName(),
                    gameState.getPlayerName(),
                    gameState.getPlayerCar().getLapUtils().getBestLapTime());

                if(isBestBeaten)
                {
                    std::string bestTime = Tools::SecondsToString(gameState.getPlayerCar().getLapUtils().getBestLapTime());
                    mUIRace->addMiscPanelText("Track record updated: [" + bestTime + "]", Ogre::ColourValue::Red);
                }
            }
        }
    }
}

void BaseRaceMode::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    if(evt.source == mModeContext.mWindow)//for arrow
    {
#if SHOW_DETAILS_PANEL
        mDetailsPanel->show();
#endif
    }
    else//rear view mirror
    {
#if SHOW_DETAILS_PANEL
        mDetailsPanel->hide();
#endif
        //http://www.ogre3d.org/forums/viewtopic.php?t=45499
        if(mCameraMan->getCameraPositionType() != CameraPosition_Bumper)
            mModeContext.mGameState.getPlayerCar().setVisibility(false);

        mModeContext.mGameState.getPlayerCar().setParticlesVisibility(false);

        mUIRace->setVisibleTachoNeedleAndPointer(false);
    }
}

void BaseRaceMode::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    if(evt.source == mModeContext.mWindow)//for arrow
    {
    }
    else//rear view mirror
    {
        if(mCameraMan->getCameraPositionType() != CameraPosition_Bumper)
            mModeContext.mGameState.getPlayerCar().setVisibility(true);

        mModeContext.mGameState.getPlayerCar().setParticlesVisibility(true);

        mUIRace->setVisibleTachoNeedleAndPointer(true);
    }
}

void BaseRaceMode::preViewportUpdate(const Ogre::RenderTargetViewportEvent& evt)
{
    if(evt.source == mViewPortCarUI)//for arrow
    {
#if SHOW_DETAILS_PANEL
        mDetailsPanel->show();
#endif
        mModeContext.mGameState.getArrowNode()->setVisible(false);
        mUIRace->setVisible(true);
    }
    if(evt.source == mViewPortScene)//for arrow
    {
#if SHOW_DETAILS_PANEL
        mDetailsPanel->hide();
#endif
        if( !mModeContext.mGameState.isStuntTrack()         &&
            !mModeContext.mGameState.isFoxnhound1Track()    &&
            !mModeContext.mGameState.isFoxnhound2Track()
            )
            mModeContext.mGameState.getArrowNode()->setVisible(true);

        mUIRace->setVisible(false);
    }
}
