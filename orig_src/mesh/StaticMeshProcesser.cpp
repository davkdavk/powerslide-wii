
#include "StaticMeshProcesser.h"

#include "../physics/Physics.h"

#include "../lua/DMLuaManager.h"

#include "../tools/OgreTools.h"

#include "../listeners/TerrainSceneObjectListener.h"
#include "../listeners/LoaderListener.h"

#include "../loaders/DE2Loader.h"
#include "../loaders/TRALoader.h"
#include "../loaders/TEXLoader.h"

#include "../tools/Conversions.h"
#include "../WiiDebugLog.h"
#include "../../wii_stubs/OGRE/WiiGXRenderer.h"

#if defined(WII) || defined(__wii__)
#define OSReport SYS_Report
#endif

#ifdef _MSC_VER
    #pragma warning(disable:4996)
#endif

Ogre::NameGenerator StaticMeshProcesser::nameGenRigidBodies("RigidBody");

StaticMeshProcesser::StaticMeshProcesser()
    : mNameGenMaterials("Scene/Material/Name"),
    mNameGenNodes("Scene/StaticNode/Name"),
    mNameGenTextures("Scene/Texture/Name"),
    mPlainIndices(0),
    mIsVertexArraySupported(false)
{
}

void StaticMeshProcesser::initParts(lua_State * pipeline, 
                                    Ogre::SceneManager* sceneMgr, 
                                    Ogre::SceneNode* mainNode,
                                    bool isGlobalReset,
                                    GameState& gameState,
                                    Physics * world,
                                    LoaderListener* loaderListener)
{
    checkIsVertexArraySupported();

#if defined(WII_NATIVE_ASSET_PIPELINE)
    WiiGX::Renderer::getInstance().clearTerrainBuffer();
#endif

    DMLuaManager luaManager;

    mIsMaskLight = luaManager.ReadScalarBool("Terrain.Material.IsMaskLights", pipeline);

    mTerrainNodes.clear();

    std::vector<MSHData> mergedMSH;

    DE2Loader de2Loader;
    std::vector<MSHData> originalParts;
    std::string pfFolderName = gameState.getSTRPowerslide().getBaseDir(gameState.getTrackName());
    std::string de2Filename = gameState.getSTRPowerslide().getValue(gameState.getTrackName() + " parameters", "de2 filename", "");
    Ogre::DataStreamPtr fileToLoad = gameState.getPFLoaderData().getFile("data/tracks/" + pfFolderName, de2Filename);
    bool loadResult = false;
#if defined(WII) || defined(__wii__)
    OSReport("[MESH] DE2 path attempt: 'data/tracks/%s' + '%s'\n", pfFolderName.c_str(), de2Filename.c_str());
    WiiDebugLog("[MESH] initParts trackDir=%s de2File=%s\n", pfFolderName.c_str(), de2Filename.c_str());
    WiiDebugLog("[MESH] de2 stream ptr=%d readable=%d\n", fileToLoad.get() ? 1 : 0, (fileToLoad.get() && fileToLoad->isReadable()) ? 1 : 0);
    OSReport("[MESH] initParts entry trackDir=%s globalReset=%d\n", pfFolderName.c_str(), isGlobalReset ? 1 : 0);
    WiiDebugLog("[MESH_PROBE] initParts entry trackDir=%s globalReset=%d\n", pfFolderName.c_str(), isGlobalReset ? 1 : 0);
#if defined(WII_NATIVE_ASSET_PIPELINE)
    if(!fileToLoad.get() || !fileToLoad->isReadable())
    {
        static const char* kTrackDe2Candidates[] = {
            "deserttrack.de2",
            "desert.de2",
            "track.de2"
        };
        for(size_t c = 0; c < (sizeof(kTrackDe2Candidates) / sizeof(kTrackDe2Candidates[0])); ++c)
        {
            Ogre::DataStreamPtr fallback = gameState.getPFLoaderData().getFile("data/tracks/" + pfFolderName, kTrackDe2Candidates[c]);
            OSReport("[MESH] de2 fallback try '%s' ok=%d\n", kTrackDe2Candidates[c], (fallback.get() && fallback->isReadable()) ? 1 : 0);
            WiiDebugLog("[MESH_PROBE] de2 fallback try '%s' ok=%d\n", kTrackDe2Candidates[c], (fallback.get() && fallback->isReadable()) ? 1 : 0);
            if(fallback.get() && fallback->isReadable())
            {
                fileToLoad = fallback;
                break;
            }
        }
    }
#endif
#endif
    if(fileToLoad.get() && fileToLoad->isReadable())
    {
#if defined(WII) || defined(__wii__)
        WiiDebugLog("[MESH] de2 stream size=%u\n", static_cast<unsigned int>(fileToLoad->size()));
#endif
        if(loaderListener)
            loaderListener->loadState(0.3f, "DE2 reading");//0.3 from BaseRaceMode::initData

        loadResult = de2Loader.load(originalParts, fileToLoad, true);
#if defined(WII) || defined(__wii__)
        WiiDebugLog("[MESH] de2 loadResult=%d parts=%u\n", loadResult ? 1 : 0, static_cast<unsigned int>(originalParts.size()));
        OSReport("[MESH] de2 loadResult=%d parts=%u\n", loadResult ? 1 : 0, static_cast<unsigned int>(originalParts.size()));
        WiiDebugLog("[MESH_PROBE] de2 loadResult=%d parts=%u\n", loadResult ? 1 : 0, static_cast<unsigned int>(originalParts.size()));
#endif

        if(loaderListener)
            loaderListener->loadState(0.31f, "DE2 loaded");//0.3 from BaseRaceMode::initData

        fileToLoad->close();

        if(loadResult)
        {
            mBoundingBoxAABB = de2Loader.getDE2().CollisionInfo_Global[0].subparts[0].aabb;
            mCollisionDetection.init(de2Loader.getDE2());
            createLights(pipeline, sceneMgr, de2Loader.getDE2(), gameState);
        }
    }
    if(loadResult)
    {

        for(size_t q = 0; q < de2Loader.getDE2().Data_TerranName.size(); ++q)
            mTerrainMapsNames.insert(de2Loader.getDE2().Data_TerranName[q]);

        std::vector<MSHData>* partsForBuild = &mergedMSH;

#if defined(WII_NATIVE_ASSET_PIPELINE)
        // Keep original DE2 part boundaries on Wii native path so per-part/per-triangle
        // texture mapping remains intact for direct terrain upload.
        partsForBuild = &originalParts;
#else
        std::map<std::string, mergedInfo> mapTexturesToMSHIndex;

        for(size_t q = 0; q < originalParts.size(); ++q)
        {
            MSHData& mshData = originalParts[q];

            Ogre::Vector3 min, max;
            Ogre::Vector3 centroid = mshData.getCentroid();
            mshData.getMinMax(min, max, centroid);

            mergeMSH(mshData, mapTexturesToMSHIndex, mergedMSH);

            mshData.clear();

        }
#endif

#if defined(WII) || defined(__wii__)
        WiiDebugLog("[MESH] mergedMSH=%u originalParts=%u using=%s\n",
            static_cast<unsigned int>(mergedMSH.size()),
            static_cast<unsigned int>(originalParts.size()),
#if defined(WII_NATIVE_ASSET_PIPELINE)
            "original"
#else
            "merged"
#endif
        );
#endif

        //create textures
        loadTextures(gameState, *partsForBuild, gameState.getPFLoaderData(), pfFolderName, gameState.getGamma(), gameState.getDoUpscale(), loaderListener);

        if(loaderListener)
            loaderListener->loadState(0.7f, "Textures loaded");

        for(size_t q = 0; q < partsForBuild->size(); ++q)
        {
            MSHData& partData = (*partsForBuild)[q];
            Ogre::Entity* terrain;
            Ogre::SceneNode* terrainNode;

            std::string groupName = gameState.getSTRPowerslide().getBaseDir(gameState.getTrackName());
            std::string nodeName = groupName + Conversions::DMToString(q);

            Ogre::Vector3 min, max;
            Ogre::Vector3 centroid = partData.getCentroid();
            partData.getMinMax(min, max, centroid);

            //create graphics entitys
            if(isGlobalReset)
            {
                partData.preallocatePlainBuffer(true);
#if defined(WII) || defined(__wii__)
                OSReport("[MESH] createMesh call idx=%u tri=%u\n", static_cast<unsigned int>(q), static_cast<unsigned int>(partData.triCount));
                WiiDebugLog("[MESH_PROBE] createMesh call idx=%u tri=%u\n", static_cast<unsigned int>(q), static_cast<unsigned int>(partData.triCount));
#endif
                terrain = createMesh(   pipeline, 
                                        sceneMgr, nodeName, 
                                        centroid, min, max, 
                                        partData,
                                        gameState.getSTRPowerslide().getTrackSkyColor(gameState.getTrackName()),
                                        gameState.getSTRPowerslide().getFogStartEnd(gameState.getTrackName()),
                                        gameState.getSTRPowerslide().getTrackAmbientColor(gameState.getTrackName()),
                                        gameState.isCastShadows());
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[MESH] createMesh idx=%u ok=%d tri=%u\n",
                    static_cast<unsigned int>(q),
                    terrain ? 1 : 0,
                    static_cast<unsigned int>(partData.triCount));
                OSReport("[MESH] createMesh result idx=%u ok=%d tri=%u\n",
                    static_cast<unsigned int>(q),
                    terrain ? 1 : 0,
                    static_cast<unsigned int>(partData.triCount));
                WiiDebugLog("[MESH_PROBE] createMesh result idx=%u ok=%d tri=%u\n",
                    static_cast<unsigned int>(q),
                    terrain ? 1 : 0,
                    static_cast<unsigned int>(partData.triCount));
#endif
            }
            else
            {
                terrain = sceneMgr->createEntity(nodeName, nodeName, TEMP_RESOURCE_GROUP_NAME);
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[MESH] createEntity cached idx=%u ok=%d\n", static_cast<unsigned int>(q), terrain ? 1 : 0);
#endif
            }
            initPart(pipeline, sceneMgr, mainNode, isGlobalReset, gameState, terrain, terrainNode, centroid);

        }
    }
    else
    {
#if defined(WII) || defined(__wii__)
        WiiDebugLog("[MESH] initParts skipped: de2 load failed\n");
#endif
    }
}

void StaticMeshProcesser::createLights(lua_State * pipeline, Ogre::SceneManager* sceneMgr, const DE2::DE2_File& de2, GameState& gameState)
{
    DMLuaManager luaManager;

    bool isDebugLights = luaManager.ReadScalarBool("Scene.TerrainScene.IsDebugLights", pipeline);
    bool IsDebugLightsSizeAsRange = luaManager.ReadScalarBool("Scene.TerrainScene.IsDebugLightsSizeAsRange", pipeline);

    for(size_t q = 0; q < de2.lights.size(); ++q)
    {
        Ogre::String nameLight = "Lamp_" + Conversions::DMToString(q + 1);
        Ogre::Light* light = sceneMgr->createLight(nameLight);

        Ogre::SceneNode * lightNode = sceneMgr->getRootSceneNode()->createChildSceneNode(nameLight);

        lightNode->attachObject(light);

        light->setCastShadows(false);
        light->setType(Ogre::Light::LT_POINT);
        light->setPosition(de2.lights[q].position.x, de2.lights[q].position.y, -de2.lights[q].position.z);
        light->setDiffuseColour(de2.lights[q].r, de2.lights[q].g, de2.lights[q].b);
        light->setSpecularColour(1.0f, 1.0f, 1.0f);

        //d.polubotko: adjusted to pass to shader light radius In/Out
        light->setAttenuation(de2.lights[q].rangeOut, de2.lights[q].rangeIn, 0.0f, 0.0f);

        if(light->getAttenuationRange() == 20000.0f)
        {
            gameState.setGlobalLight(light);
            light->setLightMask(0x01);
        }
        else
        {
            light->setLightMask(0x02);
        }

        if(isDebugLights)
        {

            Ogre::Real scale = 0.1f;

            if(IsDebugLightsSizeAsRange)
            {
                const Ogre::Real prefabRadius = 50.0f;
                Ogre::Real radius = light->getAttenuationRange();
                scale = radius / prefabRadius;
            }

            Ogre::String debugSphereName = mNameGenNodes.generate();
            Ogre::Entity * debugSphere = sceneMgr->createEntity(debugSphereName, Ogre::SceneManager::PT_SPHERE);
            debugSphere->setMaterialName("BaseWhiteNoLighting");
            Ogre::SceneNode * debugSphereNode = sceneMgr->getRootSceneNode()->createChildSceneNode(debugSphereName);
            debugSphereNode->attachObject(debugSphere);
            debugSphereNode->setPosition(light->getPosition());
            debugSphereNode->setScale(scale, scale, scale);
            debugSphere->setCastShadows(false);

            if(IsDebugLightsSizeAsRange)
                debugSphere->getSubEntity(0)->getMaterial()->getTechnique(0)->getPass(0)->setPolygonMode(Ogre::PM_WIREFRAME);
        }
    }

    if(gameState.isCastShadows())
    {
        //shadow light
        gameState.setShadowLight(sceneMgr->createLight("shadow_light"));
        //Ogre::SceneNode * lightNode = mModelNode->createChildSceneNode("shadow_light");
        Ogre::SceneNode * lightNode = sceneMgr->getRootSceneNode()->createChildSceneNode("shadow_light");
        lightNode->attachObject(gameState.getShadowLight());
        gameState.getShadowLight()->setType(Ogre::Light::LT_SPOTLIGHT);
        gameState.getShadowLight()->setCastShadows(true);
        //mShadowLight->setPosition(0.0f, 40.0f, 0.0f);
        //mShadowLight->setDirection(Ogre::Vector3::NEGATIVE_UNIT_Y);
        gameState.getShadowLight()->setSpotlightOuterAngle(Ogre::Degree(luaManager.ReadScalarFloat("Model.ShadowLightOuterAngle", pipeline)));
        gameState.getShadowLight()->setSpotlightInnerAngle(Ogre::Degree(luaManager.ReadScalarFloat("Model.ShadowLightInnerAngle", pipeline)));


        sceneMgr->setShadowTextureCount(1);
        // Shadow (after lights and camera)
        sceneMgr->setShadowTextureSelfShadow(false);
        sceneMgr->setShadowTextureCasterMaterial("Test/ShadowCaster");
        sceneMgr->setShadowTexturePixelFormat(Ogre::PF_FLOAT16_R);
        sceneMgr->setShadowCasterRenderBackFaces(false);
        //sceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);
        sceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE);
        sceneMgr->setShadowTextureSize(luaManager.ReadScalarInt("Scene.ShadowMapSize", pipeline));
        sceneMgr->setShadowCameraSetup(Ogre::ShadowCameraSetupPtr(new Ogre::DefaultShadowCameraSetup()));
    }

    if(gameState.getLLTObject())
        gameState.getLLTObject()->setVisible(true);
}

void StaticMeshProcesser::loadTextures(GameState& gameState, const std::vector<MSHData>& mergedMSH, const PFLoader& pfloader, const std::string& trackName, Ogre::Real gamma, bool doUpscale, LoaderListener* loaderListener)
{
    std::set<std::string> texturesNames;

    //get unique names
    for(size_t q = 0; q < mergedMSH.size(); ++q)
    {
        for(size_t w = 0; w < mergedMSH[q].textureNames.size(); ++w)
        {
            texturesNames.insert(mergedMSH[q].textureNames[w]);
        }
    }

#if defined(__ANDROID__)
    mTexturesNames = texturesNames;
#endif

    loadTextures(gameState, texturesNames, pfloader, trackName, gamma, doUpscale, loaderListener);
}

void StaticMeshProcesser::loadTextures(GameState& gameState, const std::set<std::string>& texturesNames, const PFLoader& pfloader, const std::string& trackName, Ogre::Real gamma, bool doUpscale, LoaderListener* loaderListener)
{
    //ranges from BaseRaceMode::initData
    const float loaderMin = 0.5f;
    const float loaderMax = 0.7f;
    const float loaderDistance = loaderMax - loaderMin;

    size_t loadedAmount = 0;

    for(std::set<std::string>::const_iterator i = texturesNames.begin(), j = texturesNames.end();
        i != j; ++i, ++loadedAmount)
    {
        std::string noExtFileName = (*i).substr(0, (*i).length() - 4);
        Ogre::DataStreamPtr fileToLoad;

        fileToLoad = pfloader.getFile("data/tracks/" + trackName +"/textures", noExtFileName + "_m_1.tex");
        if(!fileToLoad.get() || !fileToLoad->isReadable())
        {
            fileToLoad = pfloader.getFile("data/tracks/" + trackName +"/textures", noExtFileName + "_m_2.tex");
        }
        if(!fileToLoad.get() || !fileToLoad->isReadable())
        {
            fileToLoad = pfloader.getFile("data/tracks/" + trackName +"/textures", noExtFileName + "_m_3.tex");
        }
        if(!fileToLoad.get() || !fileToLoad->isReadable())
        {
            fileToLoad = pfloader.getFile("data/tracks/" + trackName +"/textures", noExtFileName + "_m_4.tex");
        }

        if(fileToLoad.get() && fileToLoad->isReadable())
        {
            TEXLoader().load(fileToLoad, (*i), gameState.getX2LUTs(), gameState.getX4LUTs(), TEMP_RESOURCE_GROUP_NAME, gamma, doUpscale);
            fileToLoad->close();
        }

        if(loaderListener && loadedAmount % 2)
            loaderListener->loadState(loaderMin + loaderDistance * static_cast<float>(loadedAmount) / static_cast<float>(texturesNames.size()), noExtFileName);
    }
}

#if defined(__ANDROID__)
void StaticMeshProcesser::loadTextures(GameState& gameState, const PFLoader& pfloader, const std::string& trackName, Ogre::Real gamma, bool doUpscale, LoaderListener* loaderListener)
{
    loadTextures(gameState, mTexturesNames, pfloader, trackName, gamma, doUpscale, loaderListener);
}
#endif

void StaticMeshProcesser::mergeMSH(const MSHData& mshData, std::map<std::string, mergedInfo>& mapTexturesToMSHIndex, std::vector<MSHData>& mergedMSH)const
{
    for(size_t q = 0; q < mshData.texturesCount; ++q)
    {
        std::string textureName = mshData.textureNames[q];
        bool isDecal = mshData.isDecalTexture[q];

        std::map<std::string, mergedInfo>::iterator i = mapTexturesToMSHIndex.find(textureName);

        size_t indexForBatch = 0;
        if(i != mapTexturesToMSHIndex.end() && isDecal && (*i).second.isDecalTextureHappened)
        {
            indexForBatch = (*i).second.indexForBatchDecal;
        }
        else if(i != mapTexturesToMSHIndex.end() && !isDecal && (*i).second.isLitTextureHappened)
        {
            indexForBatch = (*i).second.indexForBatchLit;
        }
        else if(i != mapTexturesToMSHIndex.end() && isDecal && (*i).second.isLitTextureHappened)
        {
            indexForBatch = mergedMSH.size();
            MSHData newMsh;
            newMsh.texturesCount = 1;
            newMsh.triCount = 0;
            newMsh.vertCount = 0;
            newMsh.textureNames.push_back(textureName);
            newMsh.isDecalTexture.push_back(isDecal);
            mergedMSH.push_back(newMsh);

            (*i).second.indexForBatchDecal = indexForBatch;
            (*i).second.isDecalTextureHappened = true;
        }
        else if(i != mapTexturesToMSHIndex.end() && !isDecal && (*i).second.isDecalTextureHappened)
        {
            indexForBatch = mergedMSH.size();
            MSHData newMsh;
            newMsh.texturesCount = 1;
            newMsh.triCount = 0;
            newMsh.vertCount = 0;
            newMsh.textureNames.push_back(textureName);
            newMsh.isDecalTexture.push_back(isDecal);
            mergedMSH.push_back(newMsh);

            (*i).second.indexForBatchLit = indexForBatch;
            (*i).second.isLitTextureHappened = true;
        }
        else
        {
            indexForBatch = mergedMSH.size();
            MSHData newMsh;
            newMsh.texturesCount = 1;
            newMsh.triCount = 0;
            newMsh.vertCount = 0;
            newMsh.textureNames.push_back(textureName);
            newMsh.isDecalTexture.push_back(isDecal);
            mergedMSH.push_back(newMsh);

            if(isDecal)
                mapTexturesToMSHIndex.insert(std::make_pair(textureName, mergedInfo(indexForBatch, true, 0, false)));
            if(!isDecal)
                mapTexturesToMSHIndex.insert(std::make_pair(textureName, mergedInfo(0, false, indexForBatch, true)));
        }

        MSHData& mergedData = mergedMSH[indexForBatch];
        
        size_t triCountAdded = 0;
        size_t vertCountAdded = 0;
        for(size_t w = 0; w < mshData.triCount; ++w)
        {
            if(mshData.textureForTriangleIndex[w] == q)
            {
                const size_t triBase = mergedData.vertexes.size();

                ++triCountAdded;
                vertCountAdded += 3;

                mergedData.plainVertices.push_back(mshData.plainVertices[w * 3 + 0]);
                mergedData.plainVertices.push_back(mshData.plainVertices[w * 3 + 1]);
                mergedData.plainVertices.push_back(mshData.plainVertices[w * 3 + 2]);

                //mergedData.plainNormals.push_back(mshData.plainNormals[w * 3 + 0]);
                //mergedData.plainNormals.push_back(mshData.plainNormals[w * 3 + 1]);
                //mergedData.plainNormals.push_back(mshData.plainNormals[w * 3 + 2]);

                mergedData.plainTexCoords.push_back(mshData.plainTexCoords[w * 3 + 0]);
                mergedData.plainTexCoords.push_back(mshData.plainTexCoords[w * 3 + 1]);
                mergedData.plainTexCoords.push_back(mshData.plainTexCoords[w * 3 + 2]);

                mergedData.plainColors.push_back(mshData.plainColors[w * 3 + 0]);
                mergedData.plainColors.push_back(mshData.plainColors[w * 3 + 1]);
                mergedData.plainColors.push_back(mshData.plainColors[w * 3 + 2]);

                mergedData.textureForTriangleIndex.push_back(0);

                mergedData.vertexes.push_back(mshData.plainVertices[w * 3 + 0]);
                mergedData.vertexes.push_back(mshData.plainVertices[w * 3 + 1]);
                mergedData.vertexes.push_back(mshData.plainVertices[w * 3 + 2]);

                MSHIndixes face;
                face.a = triBase + 0;
                face.b = triBase + 1;
                face.c = triBase + 2;
                mergedData.triIndexes.push_back(face);
            }
        }

        mergedData.triCount += triCountAdded;
        mergedData.vertCount += vertCountAdded;

        // Keep explicit parity for direct indexed terrain upload path.
        mergedData.triCount = mergedData.triIndexes.size();

    }
}

void StaticMeshProcesser::initPart(lua_State * pipeline, 
                                   Ogre::SceneManager* sceneMgr, 
                                   Ogre::SceneNode* mainNode,
                                   bool isGlobalReset,
                                   GameState& gameState,
                                   Ogre::Entity* terrain,
                                   Ogre::SceneNode*& terrainNode,
                                   const Ogre::Vector3& centroid)
{

    DMLuaManager luaManager;

    mTerrainNodes.push_back(terrain);

    terrain->setCastShadows(false);

    //if mask enabled allow only global lights
    if(mIsMaskLight)
    {
        terrain->setLightMask(0x1);
    }


    terrainNode = mainNode->createChildSceneNode();
    terrainNode->attachObject(terrain);
#if defined(WII) || defined(__wii__)
    WiiDebugLog("[MESH] attached terrain entity to scene node\n");
#endif
    //use it for normal based & exclusion boxes light model (not like original one)
    //terrain->setListener(new TerrainSceneObjectListener(terrain, sceneMgr, gameState.getExclusions()));

    terrainNode->setPosition(centroid);
    
    if(isGlobalReset)
    {
        if(luaManager.ReadScalarBool("Terrain.Scene.IsAdjustNormals", pipeline))
        {
            AddjustNormals(terrain, luaManager.ReadScalarFloat("Terrain.Scene.AdjustNormalsAngleThreshold", pipeline));
        }
    }

    if(luaManager.ReadScalarBool("Terrain.Mesh.IsTangents", pipeline))
    {
        BuildTangents(terrain);
    }

    if(luaManager.ReadScalarBool("Terrain.Mesh.IsBB", pipeline))
    {
        terrainNode->showBoundingBox(true);
    }

    if(luaManager.ReadScalarBool("Terrain.Mesh.IsBS", pipeline))
    {
        const Ogre::Real prefabRadius = 50.0f;
        Ogre::Real radius = terrain->getMesh()->getBoundingSphereRadius();

        Ogre::Real scale = radius / prefabRadius;

        Ogre::String debugSphereName = mNameGenNodes.generate();
        Ogre::Entity * debugSphere = sceneMgr->createEntity(debugSphereName, Ogre::SceneManager::PT_SPHERE);
        debugSphere->setMaterialName("BaseWhiteNoLighting");
        Ogre::SceneNode * debugSphereNode = sceneMgr->getRootSceneNode()->createChildSceneNode(debugSphereName);
        debugSphereNode->attachObject(debugSphere);
        debugSphereNode->setPosition(centroid);
        debugSphereNode->setScale(scale, scale, scale);
        debugSphere->getSubEntity(0)->getMaterial()->getTechnique(0)->getPass(0)->setPolygonMode(Ogre::PM_WIREFRAME);
        debugSphere->setCastShadows(false);
    }

}

void StaticMeshProcesser::checkIsVertexArraySupported()
{
    mIsVertexArraySupported = false;

    //TODO(d.polubotko): implement check for GL_EXT_texture_array extension

#if !defined(__ANDROID__)
    if (    !Ogre::GpuProgramManager::getSingleton().isSyntaxSupported("vs_4_0") &&
            !Ogre::GpuProgramManager::getSingleton().isSyntaxSupported("ps_2_0") && 
            !Ogre::GpuProgramManager::getSingleton().isSyntaxSupported("glsl") &&
            !Ogre::GpuProgramManager::getSingleton().isSyntaxSupported("gp4fp"))
    {
        // not supported?
    }
    else
    {
        //mIsVertexArraySupported = true;
    }
#endif

}

Ogre::Entity* StaticMeshProcesser::createMesh(  lua_State * pipeline, 
                                                Ogre::SceneManager* sceneMgr, 
                                                const Ogre::String& entityName, 
                                                const Ogre::Vector3& centroid,
                                                const Ogre::Vector3& min, 
                                                const Ogre::Vector3& max,
                                                MSHData& mshData,
                                                const Ogre::ColourValue& skyColor,
                                                const Ogre::Vector2& fogStartEnd,
                                                const Ogre::ColourValue& ambient,
                                                bool isShadow)
{
    Ogre::Entity * res = NULL;

#if defined(WII) || defined(__wii__)
    OSReport("[MESH] createMesh entry name=%s\n", entityName.c_str());
    WiiDebugLog("[MESH_PROBE] createMesh entry name=%s\n", entityName.c_str());
#endif



    DMLuaManager luaManager;

    Ogre::MaterialPtr overrideMaterial = Ogre::MaterialManager::getSingleton().getByName(luaManager.ReadScalarString("Terrain.Material.SingleSubMaterial", pipeline));
    Ogre::MaterialPtr overrideMaterialFog = Ogre::MaterialManager::getSingleton().getByName(luaManager.ReadScalarString("Terrain.Material.SingleSubMaterialFog", pipeline));
    Ogre::MaterialPtr overrideMaterialArray = Ogre::MaterialManager::getSingleton().getByName(luaManager.ReadScalarString("Terrain.Material.SingleSubMaterialArray", pipeline));

    if(isShadow)
    {
        overrideMaterial = Ogre::MaterialManager::getSingleton().getByName(luaManager.ReadScalarString("Terrain.Material.SingleSubMaterial", pipeline));
        overrideMaterialFog = Ogre::MaterialManager::getSingleton().getByName(luaManager.ReadScalarString("Terrain.Material.SingleSubMaterialFog", pipeline));
        overrideMaterialArray = Ogre::MaterialManager::getSingleton().getByName(luaManager.ReadScalarString("Terrain.Material.SingleSubMaterialArray", pipeline));
    }
    else
    {
        overrideMaterial = Ogre::MaterialManager::getSingleton().getByName(luaManager.ReadScalarString("Terrain.Material.SingleSubMaterialNoShadow", pipeline));
        overrideMaterialFog = Ogre::MaterialManager::getSingleton().getByName(luaManager.ReadScalarString("Terrain.Material.SingleSubMaterialNoShadowFog", pipeline));
        overrideMaterialArray = Ogre::MaterialManager::getSingleton().getByName(luaManager.ReadScalarString("Terrain.Material.SingleSubMaterialArray", pipeline));
    }

    std::string defaultTextureName = luaManager.ReadScalarString("Terrain.Material.DefaultTextureName", pipeline);

    bool isOverrideDefault = luaManager.ReadScalarBool("Terrain.Material.IsOverrideDefaultTextureWithTransparentForSubMaterials", pipeline);

    bool isFogEnabled = fogStartEnd.x >= 1000000.0f ? false : true;
    if(isFogEnabled)
    {
        sceneMgr->setFog(Ogre::FOG_LINEAR, skyColor, 0.0f, fogStartEnd.x, fogStartEnd.y);
    }

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[MESH_PROBE] materials single=%d fog=%d array=%d fogEnabled=%d vertexArray=%d\n",
        overrideMaterial.isNull() ? 0 : 1,
        overrideMaterialFog.isNull() ? 0 : 1,
        overrideMaterialArray.isNull() ? 0 : 1,
        isFogEnabled ? 1 : 0,
        mIsVertexArraySupported ? 1 : 0);
#endif

#if defined(WII_NATIVE_ASSET_PIPELINE)
    if(overrideMaterial.isNull())
    {
        overrideMaterial = Ogre::MaterialManager::getSingleton().getByName("Basewhite");
    }
    if(overrideMaterialFog.isNull())
    {
        overrideMaterialFog = overrideMaterial;
    }
#endif

    Ogre::String overrideMaterialNameSafe = "Basewhite";
    Ogre::String overrideMaterialFogNameSafe = "Basewhite";
    if(!overrideMaterial.isNull())
    {
        overrideMaterialNameSafe = overrideMaterial->getName();
    }
    if(!overrideMaterialFog.isNull())
    {
        overrideMaterialFogNameSafe = overrideMaterialFog->getName();
    }
    else
    {
        overrideMaterialFogNameSafe = overrideMaterialNameSafe;
    }

    if(!overrideMaterial.isNull()
#if defined(WII_NATIVE_ASSET_PIPELINE)
        || true
#endif
        )
    {

        std::vector<std::string> materialNames;

        if(
#if defined(WII_NATIVE_ASSET_PIPELINE)
            false
#else
            mIsVertexArraySupported && !overrideMaterialArray.isNull()
#endif
            )
        {
            materialNames = loadWithVertexArray(    isOverrideDefault, 
                                                    defaultTextureName, 
                                                    overrideMaterialNameSafe,
                                                    overrideMaterialArray->getName(),
                                                    mshData,
                                                    ambient);
        }
        else
        {
            materialNames = loadWithoutVertexArray( isOverrideDefault,
                                                    defaultTextureName,
                                                    isFogEnabled ? overrideMaterialFogNameSafe : overrideMaterialNameSafe,
                                                    mshData,
                                                    skyColor,
                                                    fogStartEnd,
                                                    ambient,
                                                    isFogEnabled);

#if defined(WII_NATIVE_ASSET_PIPELINE)
            if(materialNames.empty())
            {
                materialNames.push_back("Basewhite");
            }
#endif
        }


        //create interleaved buffer
        mshData.makePlain(centroid, true);

#if defined(WII) || defined(__wii__)
        OSReport("[MESH] createMesh triCount=%d verts=%d\n", static_cast<int>(mshData.triCount), static_cast<int>(mshData.plainBuffer.size()));
        WiiDebugLog("[MESH_PROBE] createMesh triCount=%d verts=%d\n", static_cast<int>(mshData.triCount), static_cast<int>(mshData.plainBuffer.size()));
#endif

#if defined(WII_NATIVE_ASSET_PIPELINE)
        {
            if(mshData.vertCount == 0 || mshData.triCount == 0)
            {
                WiiDebugLog("[GXDIR] skipping chunk with no vertices or triangles\n");
            }
            else if(mshData.vertexes.size() < mshData.vertCount)
            {
                WiiDebugLog("[GXDIR] skipping chunk: vertexes size %u < vertCount %u\n",
                    static_cast<unsigned int>(mshData.vertexes.size()),
                    static_cast<unsigned int>(mshData.vertCount));
            }
            else if(mshData.triIndexes.size() < mshData.triCount)
            {
                WiiDebugLog("[GXDIR] skipping chunk: triIndexes size %u < triCount %u\n",
                    static_cast<unsigned int>(mshData.triIndexes.size()),
                    static_cast<unsigned int>(mshData.triCount));
            }
            else
            {
                if(!mshData.textureNames.empty() && !mshData.textureNames[0].empty())
                {
                    WiiGX::Renderer::getInstance().setTerrainTextureName(mshData.textureNames[0]);
                }

                const u32 vertexCount = static_cast<u32>(mshData.vertCount);
                WiiGX::Renderer::getInstance().setTerrainBuildProbe(static_cast<u32>(mshData.triCount));

                static std::vector<float> terrainVerts;
                static std::vector<u32> terrainIndices;
                static std::vector<float> terrainUvByIndex;
                terrainVerts.resize(vertexCount * 3);
                for(u32 v = 0; v < vertexCount; ++v)
                {
                    terrainVerts[v * 3 + 0] = mshData.vertexes[v].x;
                    terrainVerts[v * 3 + 1] = mshData.vertexes[v].y;
                    terrainVerts[v * 3 + 2] = mshData.vertexes[v].z;
                }
                const u32 indexCount = static_cast<u32>(mshData.triCount * 3);
                terrainIndices.resize(indexCount);
                terrainUvByIndex.resize(indexCount * 2);
                u32 maxIndex = 0;
                u32 invalidIndexCount = 0;
                u32 invalidUvCount = 0;
                for(u32 t = 0; t < mshData.triCount; ++t)
                {
                    const u32 ia = static_cast<u32>(mshData.triIndexes[t].a);
                    const u32 ib = static_cast<u32>(mshData.triIndexes[t].b);
                    const u32 ic = static_cast<u32>(mshData.triIndexes[t].c);
                    terrainIndices[t * 3 + 0] = ia;
                    terrainIndices[t * 3 + 1] = ib;
                    terrainIndices[t * 3 + 2] = ic;
                    if(ia > maxIndex) maxIndex = ia;
                    if(ib > maxIndex) maxIndex = ib;
                    if(ic > maxIndex) maxIndex = ic;
                    if(ia >= vertexCount) ++invalidIndexCount;
                    if(ib >= vertexCount) ++invalidIndexCount;
                    if(ic >= vertexCount) ++invalidIndexCount;

                    Ogre::Vector3 uva = Ogre::Vector3::ZERO;
                    Ogre::Vector3 uvb = Ogre::Vector3::ZERO;
                    Ogre::Vector3 uvc = Ogre::Vector3::ZERO;
                    const bool hasPlainUv = mshData.plainTexCoords.size() >= ((static_cast<size_t>(t) + 1u) * 3u);
                    if(hasPlainUv)
                    {
                        uva = mshData.plainTexCoords[t * 3 + 0];
                        uvb = mshData.plainTexCoords[t * 3 + 1];
                        uvc = mshData.plainTexCoords[t * 3 + 2];
                    }
                    else
                    {
                        const bool hasTexTri = t < mshData.texCoordsIndexes.size();
                        const u32 ta = hasTexTri ? static_cast<u32>(mshData.texCoordsIndexes[t].a) : 0u;
                        const u32 tb = hasTexTri ? static_cast<u32>(mshData.texCoordsIndexes[t].b) : 0u;
                        const u32 tc = hasTexTri ? static_cast<u32>(mshData.texCoordsIndexes[t].c) : 0u;
                        const bool validTa = hasTexTri && (ta < mshData.texcoords.size());
                        const bool validTb = hasTexTri && (tb < mshData.texcoords.size());
                        const bool validTc = hasTexTri && (tc < mshData.texcoords.size());
                        if(!validTa) ++invalidUvCount;
                        if(!validTb) ++invalidUvCount;
                        if(!validTc) ++invalidUvCount;
                        uva = validTa ? mshData.texcoords[ta] : Ogre::Vector3::ZERO;
                        uvb = validTb ? mshData.texcoords[tb] : Ogre::Vector3::ZERO;
                        uvc = validTc ? mshData.texcoords[tc] : Ogre::Vector3::ZERO;
                    }
                    terrainUvByIndex[(t * 3 + 0) * 2 + 0] = uva.x;
                    terrainUvByIndex[(t * 3 + 0) * 2 + 1] = uva.y;
                    terrainUvByIndex[(t * 3 + 1) * 2 + 0] = uvb.x;
                    terrainUvByIndex[(t * 3 + 1) * 2 + 1] = uvb.y;
                    terrainUvByIndex[(t * 3 + 2) * 2 + 0] = uvc.x;
                    terrainUvByIndex[(t * 3 + 2) * 2 + 1] = uvc.y;
                }

                float minX = 1.0e30f, minY = 1.0e30f, minZ = 1.0e30f;
                float maxX = -1.0e30f, maxY = -1.0e30f, maxZ = -1.0e30f;
                u32 nonFiniteVerts = 0;
                for(u32 v = 0; v < vertexCount; ++v)
                {
                    const float x = terrainVerts[v * 3 + 0];
                    const float y = terrainVerts[v * 3 + 1];
                    const float z = terrainVerts[v * 3 + 2];
                    const bool finite = (x == x) && (y == y) && (z == z);
                    if(!finite)
                    {
                        ++nonFiniteVerts;
                        continue;
                    }
                    if(x < minX) minX = x; if(x > maxX) maxX = x;
                    if(y < minY) minY = y; if(y > maxY) maxY = y;
                    if(z < minZ) minZ = z; if(z > maxZ) maxZ = z;
                }

                static int sContractMeshLogs = 0;
                if(sContractMeshLogs < 128)
                {
                    const char* chunkTex = (mshData.textureNames.empty() || mshData.textureNames[0].empty()) ? "<none>" : mshData.textureNames[0].c_str();
                    WiiDebugLog("[CONTRACT][MESH] name=%s verts=%u idx=%u maxIdx=%u invalid=%u invalidUV=%u nonFinite=%u bbox=(%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f)\n",
                        entityName.c_str(),
                        static_cast<unsigned int>(vertexCount),
                        static_cast<unsigned int>(indexCount),
                        static_cast<unsigned int>(maxIndex),
                        static_cast<unsigned int>(invalidIndexCount),
                        static_cast<unsigned int>(invalidUvCount),
                        static_cast<unsigned int>(nonFiniteVerts),
                        minX, minY, minZ, maxX, maxY, maxZ);
                    WiiDebugLog("[CHUNK_TEXMAP] name=%s tex=%s tris=%u bbox=(%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f)\n",
                        entityName.c_str(),
                        chunkTex,
                        static_cast<unsigned int>(mshData.triCount),
                        minX, minY, minZ, maxX, maxY, maxZ);
                    sContractMeshLogs++;
                }

                if(invalidIndexCount > 0)
                {
                    WiiDebugLog("[CONTRACT][MESH][FAIL] name=%s invalid=%u maxIdx=%u verts=%u\n",
                        entityName.c_str(),
                        static_cast<unsigned int>(invalidIndexCount),
                        static_cast<unsigned int>(maxIndex),
                        static_cast<unsigned int>(vertexCount));
                }

                const bool hasPerTriTextureMap =
                    mshData.textureForTriangleIndex.size() >= mshData.triCount &&
                    !mshData.textureNames.empty();

                if(hasPerTriTextureMap)
                {
                    std::vector< std::vector<u32> > indicesByTexture(mshData.textureNames.size());
                    std::vector< std::vector<float> > uvByTexture(mshData.textureNames.size());

                    for(u32 t = 0; t < mshData.triCount; ++t)
                    {
                        u32 texSlot = static_cast<u32>(mshData.textureForTriangleIndex[t]);
                        if(texSlot >= mshData.textureNames.size())
                            texSlot = 0;

                        const u32 ia = terrainIndices[t * 3 + 0];
                        const u32 ib = terrainIndices[t * 3 + 1];
                        const u32 ic = terrainIndices[t * 3 + 2];
                        indicesByTexture[texSlot].push_back(ia);
                        indicesByTexture[texSlot].push_back(ib);
                        indicesByTexture[texSlot].push_back(ic);

                        const size_t uvBase = static_cast<size_t>(t) * 6u;
                        uvByTexture[texSlot].push_back(terrainUvByIndex[uvBase + 0]);
                        uvByTexture[texSlot].push_back(terrainUvByIndex[uvBase + 1]);
                        uvByTexture[texSlot].push_back(terrainUvByIndex[uvBase + 2]);
                        uvByTexture[texSlot].push_back(terrainUvByIndex[uvBase + 3]);
                        uvByTexture[texSlot].push_back(terrainUvByIndex[uvBase + 4]);
                        uvByTexture[texSlot].push_back(terrainUvByIndex[uvBase + 5]);
                    }

                    for(size_t texSlot = 0; texSlot < mshData.textureNames.size(); ++texSlot)
                    {
                        if(indicesByTexture[texSlot].empty())
                            continue;

                        if(!mshData.textureNames[texSlot].empty())
                        {
                            WiiGX::Renderer::getInstance().setTerrainTextureName(mshData.textureNames[texSlot]);
                        }

                        WiiGX::Renderer::getInstance().uploadTerrainIndexedDataWithUV(
                            terrainVerts.data(),
                            vertexCount,
                            indicesByTexture[texSlot].data(),
                            static_cast<u32>(indicesByTexture[texSlot].size()),
                            uvByTexture[texSlot].data());

                        WiiDebugLog("[GXDIR] uploading sub-batch: triIndexes=%u verts=%u indices=%u texSlot=%u tex='%s'\n",
                            static_cast<unsigned int>(mshData.triIndexes.size()),
                            static_cast<unsigned int>(vertexCount),
                            static_cast<unsigned int>(indicesByTexture[texSlot].size()),
                            static_cast<unsigned int>(texSlot),
                            mshData.textureNames[texSlot].c_str());
                    }
                }
                else
                {
                    WiiGX::Renderer::getInstance().uploadTerrainIndexedDataWithUV(
                        terrainVerts.data(), vertexCount, terrainIndices.data(), indexCount, terrainUvByIndex.data());

                    WiiDebugLog("[GXDIR] uploading chunk: triIndexes size %u == triCount %u verts=%u indices=%u\n",
                        static_cast<unsigned int>(mshData.triIndexes.size()),
                        static_cast<unsigned int>(mshData.triCount),
                        static_cast<unsigned int>(vertexCount),
                        static_cast<unsigned int>(indexCount));
                }
            }
        }
#endif

        //http://www.ogre3d.org/tikiwiki/Generating+A+Mesh
        Ogre::MeshPtr pMesh = Ogre::MeshManager::getSingleton().createManual(entityName, TEMP_RESOURCE_GROUP_NAME);
#if defined(WII) || defined(__wii__)
        WiiDebugLog("[MESH_PROBE] createManual ok=%d name=%s\n", pMesh.get() ? 1 : 0, entityName.c_str());
#endif

        pMesh->sharedVertexData = new Ogre::VertexData();
        pMesh->sharedVertexData->vertexCount = mshData.triCount * 3;
        Ogre::VertexDeclaration* decl = pMesh->sharedVertexData->vertexDeclaration;
        size_t offset = 0;
        
        decl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
        offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);

        //decl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
        //offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);

        decl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_TEXTURE_COORDINATES);
        offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);

        decl->addElement(0, offset, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
        offset += Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR);

        Ogre::HardwareVertexBufferSharedPtr vbuf = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
            offset, pMesh->sharedVertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC);
#if defined(WII) || defined(__wii__)
        WiiDebugLog("[MESH_PROBE] vbuf ok=%d bytes=%u verts=%u stride=%u\n",
            vbuf ? 1 : 0,
            vbuf ? static_cast<unsigned int>(vbuf->getSizeInBytes()) : 0u,
            static_cast<unsigned int>(pMesh->sharedVertexData->vertexCount),
            static_cast<unsigned int>(offset));
#endif

        vbuf->writeData(0, vbuf->getSizeInBytes(), &mshData.plainBuffer[0], true);

        Ogre::VertexBufferBinding* bind = pMesh->sharedVertexData->vertexBufferBinding; 
        bind->setBinding(0, vbuf);

        for(size_t q = 0; q < mshData.submeshesTriangleIndixesDiffuse.size(); ++q)
        {
            Ogre::SubMesh* sub = pMesh->createSubMesh();

#if defined(WII) || defined(__wii__)
            OSReport("[MESH] submesh register idx=%u triIndices=%u\n", static_cast<unsigned int>(q), static_cast<unsigned int>(mshData.submeshesTriangleIndixesDiffuse[q].size()));
            WiiDebugLog("[MESH_PROBE] submesh register idx=%u triIndices=%u\n", static_cast<unsigned int>(q), static_cast<unsigned int>(mshData.submeshesTriangleIndixesDiffuse[q].size()));
#endif
            
            sub->setMaterialName(materialNames[q]);

            Ogre::HardwareIndexBufferSharedPtr ibuf = Ogre::HardwareBufferManager::getSingleton().
            createIndexBuffer(
            Ogre::HardwareIndexBuffer::IT_16BIT, 
            mshData.submeshesTriangleIndixesDiffuse[q].size(), 
            Ogre::HardwareBuffer::HBU_STATIC);
#if defined(WII) || defined(__wii__)
            if(q == 0)
            {
                WiiDebugLog("[MESH_PROBE] sub0 ptr=%d ibuf=%d idxCount=%u\n",
                    sub ? 1 : 0,
                    ibuf ? 1 : 0,
                    static_cast<unsigned int>(mshData.submeshesTriangleIndixesDiffuse[q].size()));
            }
#endif
     
            /// Upload the index data to the card
            ibuf->writeData(0, ibuf->getSizeInBytes(), &mshData.submeshesTriangleIndixesDiffuse[q][0], true);

            sub->useSharedVertices = true;
            sub->indexData->indexBuffer = ibuf;
            sub->indexData->indexCount = mshData.submeshesTriangleIndixesDiffuse[q].size();
            sub->indexData->indexStart = 0;
        }

        pMesh->_setBounds(Ogre::AxisAlignedBox(min, max),false);
        pMesh->load();

#if defined(WII) || defined(__wii__)
        WiiDebugLog("[MESH_PROBE] pMesh loaded name=%s submeshes=%u\n",
            entityName.c_str(),
            static_cast<unsigned int>(pMesh->getNumSubMeshes()));
#endif

        res = sceneMgr->createEntity(entityName, pMesh);

#if defined(WII) || defined(__wii__)
        WiiDebugLog("[MESH_PROBE] createEntity(meshptr) ok=%d name=%s\n", res ? 1 : 0, entityName.c_str());
#endif

    }

    return res;
}

std::vector<std::string> StaticMeshProcesser::loadWithVertexArray(bool isOverrideDefault, 
                                                                  std::string defaultTextureName, 
                                                                  const Ogre::String& ovverideMaterialName,
                                                                  const Ogre::String& ovverideMaterialArrayName,
                                                                  MSHData& mshData,
                                                                  const Ogre::ColourValue& ambient)
{
    std::vector<std::string> materialNames;
    const size_t texturesCountSafe = std::min(mshData.texturesCount, mshData.textureNames.size());

    //pass for submeshes materials
    if(texturesCountSafe == 1)
    {
        std::string textureName = mshData.textureNames[0];

        bool isTransparentMaterial = false;

        if(isOverrideDefault)
        {
            if(textureName == defaultTextureName)
            {
                isTransparentMaterial = true;
            }
        }

        std::string materialName = "Basewhite";

        if(isTransparentMaterial)
        {
            materialName = "Test/Transparent";
        }
        else if(!mshData.isDecalTexture.empty() && mshData.isDecalTexture[0])
        {
            std::vector<Ogre::String> texturesSubMat;

            texturesSubMat.push_back(textureName);

            materialName = mNameGenMaterials.generate();

            Ogre::MaterialPtr newMat = CloneMaterial(  materialName, 
                        "Test/Diffuse", 
                        texturesSubMat, 
                        1.0f,
                        TEMP_RESOURCE_GROUP_NAME);
        }
        else
        {
            std::vector<Ogre::String> texturesSubMat;

            texturesSubMat.push_back(textureName);

            materialName = mNameGenMaterials.generate();

            Ogre::MaterialPtr newMat = CloneMaterial(  materialName, 
                ovverideMaterialName, 
                texturesSubMat, 
                1.0f,
                TEMP_RESOURCE_GROUP_NAME);

            newMat->setAmbient(ambient);
        }

        materialNames.push_back(materialName);
    }

    if(texturesCountSafe > 1)
    {
        std::string materialName = "Basewhite";

        Ogre::String texName = mNameGenTextures.generate();

        Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().createManual(texName, 
        TEMP_RESOURCE_GROUP_NAME, 
        Ogre::TEX_TYPE_2D_ARRAY, 
        256, 256, texturesCountSafe,
        0,
        Ogre::PF_R8G8B8,
        0);

        for (size_t i = 0; i < texturesCountSafe; i++)
        {
            Ogre::Image terrainTex;
            terrainTex.load(mshData.textureNames[i], TEMP_RESOURCE_GROUP_NAME);
            Ogre::HardwarePixelBufferSharedPtr pixelBufferBuf = tex->getBuffer(0);
            const Ogre::PixelBox&  currImage = pixelBufferBuf->lock(Ogre::Box(0,0,i,terrainTex.getHeight(), terrainTex.getHeight(), i+1), Ogre::HardwareBuffer::HBL_DISCARD);
            Ogre::PixelUtil::bulkPixelConversion(terrainTex.getPixelBox(), currImage);
            pixelBufferBuf->unlock();
        }

        std::vector<Ogre::String> texturesSubMat;

        texturesSubMat.push_back(texName);

        materialName = mNameGenMaterials.generate();

        Ogre::MaterialPtr newMat = CloneMaterial(  materialName, 
            ovverideMaterialArrayName, 
            texturesSubMat, 
            1.0f,
            TEMP_RESOURCE_GROUP_NAME);

        newMat->setAmbient(ambient);

        materialNames.push_back(materialName);
    }


    //textures data indices
    for(size_t q = 0; q < texturesCountSafe; ++q)
    {
        for(size_t w = 0; w < mshData.triCount; ++w)
        {
            if(mshData.textureForTriangleIndex[w] == q)
            {
                //texture index as w coordinate for vertex array
                mshData.plainTexCoords[w * 3 + 0].z = static_cast<float>(q);
                mshData.plainTexCoords[w * 3 + 1].z = static_cast<float>(q);
                mshData.plainTexCoords[w * 3 + 2].z = static_cast<float>(q);
            }
        }
    }

    //all indices for one texture
    {
        std::vector<unsigned short> triPlainIndixes;

        for(size_t w = 0; w < mshData.triCount; ++w)
        {
            {
                triPlainIndixes.push_back(static_cast<Ogre::uint16>(w * 3 + 0));
                triPlainIndixes.push_back(static_cast<Ogre::uint16>(w * 3 + 1));
                triPlainIndixes.push_back(static_cast<Ogre::uint16>(w * 3 + 2));
            }
        }

        mshData.submeshesTriangleIndixesDiffuse.push_back(triPlainIndixes);
    }

    return materialNames;
}

std::vector<std::string> StaticMeshProcesser::loadWithoutVertexArray(bool isOverrideDefault, 
                                                                     std::string defaultTextureName, 
                                                                     const Ogre::String& ovverideMaterialName,
                                                                     MSHData& mshData,
                                                                     const Ogre::ColourValue& skyColor,
                                                                     const Ogre::Vector2& fogStartEnd,
                                                                     const Ogre::ColourValue& ambient,
                                                                     bool isFogEnabled)
{
    std::vector<std::string> materialNames;
    const size_t texturesCountSafe = std::min(mshData.texturesCount, mshData.textureNames.size());

    //pass for submeshes materials
    for (size_t i = 0; i < texturesCountSafe; i++)
    {
        std::string textureName = mshData.textureNames[i];

        bool isTransparentMaterial = false;

        if(isOverrideDefault)
        {
            if(textureName == defaultTextureName)
            {
                isTransparentMaterial = true;
            }
        }

        std::string materialName = "Basewhite";

        if(isTransparentMaterial)
        {
            materialName = "Test/Transparent";
        }
        else if(i < mshData.isDecalTexture.size() && mshData.isDecalTexture[i])
        {
            std::vector<Ogre::String> texturesSubMat;

            texturesSubMat.push_back(textureName);

            materialName = mNameGenMaterials.generate();

            if(!isFogEnabled)
            {
                Ogre::MaterialPtr newMat = CloneMaterial(  materialName, 
                            "Test/Diffuse", 
                            texturesSubMat, 
                            1.0f,
                            TEMP_RESOURCE_GROUP_NAME);
            }
            else
            {
                Ogre::MaterialPtr newMat = CloneMaterial(  materialName, 
                            "Test/DiffuseFog", 
                            texturesSubMat, 
                            1.0f,
                            TEMP_RESOURCE_GROUP_NAME);

                //newMat->setFog(true, Ogre::FOG_LINEAR, skyColor, 0.0f, fogStartEnd.x, fogStartEnd.y);
            }
        }
        else
        {
            std::vector<Ogre::String> texturesSubMat;

            texturesSubMat.push_back(textureName);

            materialName = mNameGenMaterials.generate();

            Ogre::MaterialPtr newMat = CloneMaterial(  materialName, 
                ovverideMaterialName, 
                texturesSubMat, 
                1.0f,
                TEMP_RESOURCE_GROUP_NAME);

            if(!newMat.isNull())
            {
                newMat->setAmbient(ambient);
            }

            //if(isFogEnabled)
            //{
                //newMat->setFog(true, Ogre::FOG_LINEAR, skyColor, 0.0f, fogStartEnd.x, fogStartEnd.y);
            //}
        }

        materialNames.push_back(materialName);
    }

        for(size_t q = 0; q < texturesCountSafe; ++q)
        {
            std::vector<unsigned short> triPlainIndixes;

        for(size_t w = 0; w < mshData.triCount; ++w)
        {
            if(mshData.textureForTriangleIndex[w] == q)
            {
                triPlainIndixes.push_back(static_cast<Ogre::uint16>(w * 3 + 0));
                triPlainIndixes.push_back(static_cast<Ogre::uint16>(w * 3 + 1));
                triPlainIndixes.push_back(static_cast<Ogre::uint16>(w * 3 + 2));
            }
        }

            mshData.submeshesTriangleIndixesDiffuse.push_back(triPlainIndixes);

#if defined(WII) || defined(__wii__)
            static int sSubmeshTexMapLogs = 0;
            if(sSubmeshTexMapLogs < 256)
            {
                const char* slotTex = (q < mshData.textureNames.size()) ? mshData.textureNames[q].c_str() : "<out-of-range>";
                WiiDebugLog("[SUBMESH_TEXMAP] texSlot=%u tex=%s triIdxCount=%u tris=%u decal=%d\n",
                    static_cast<unsigned int>(q),
                    slotTex,
                    static_cast<unsigned int>(triPlainIndixes.size()),
                    static_cast<unsigned int>(triPlainIndixes.size() / 3),
                    (q < mshData.isDecalTexture.size() && mshData.isDecalTexture[q]) ? 1 : 0);
                sSubmeshTexMapLogs++;
            }
#endif
        }

    return materialNames;
}

void StaticMeshProcesser::queryLights()
{
    //d.polubotko: make sure light lists created during loading
    for(size_t q = 0; q < mTerrainNodes.size(); ++q)
    {
        mTerrainNodes[q]->queryLights();
    }
}

void StaticMeshProcesser::loadTerrainMaps(GameState& gameState)
{
    typedef std::set<std::string> maps;
    for(maps::iterator i = mTerrainMapsNames.begin(), j = mTerrainMapsNames.end();
        i !=j; ++i)
    {
        std::string mapName = (*i);

        Ogre::DataStreamPtr fileToLoad = gameState.getPFLoaderData().getFile("data/tracks/" + gameState.getSTRPowerslide().getBaseDir(gameState.getTrackName()) +"/terrains", mapName);

        if(fileToLoad.get() && fileToLoad->isReadable())
        {
            CommonIncludes::shared_ptr<Ogre::Image> img = std::make_shared<Ogre::Image>();
            bool isLoaded = TRALoader().load(fileToLoad, img);
            if(isLoaded)
            {
                //http://www.ogre3d.org/tikiwiki/Loading+Image+from+Disk
                //Ogre::DataStreamPtr data_stream(new Ogre::FileStreamDataStream(texture_path, &ifs, false));
                //Ogre::DataStreamPtr pStream = Ogre::ResourceGroupManager::getSingleton().openResource( mapName, TEMP_RESOURCE_GROUP_NAME );
                //img->load(mapName, TEMP_RESOURCE_GROUP_NAME);

                mTerrainMaps.insert(std::make_pair(mapName, img));
            }

            fileToLoad->close();
        }
    }
}

void StaticMeshProcesser::setTerrainData(const std::vector<TerrainData>& terrainData)
{
    assert(terrainData.size() == TerrainData::mTerrainsAmount);

    mTerrainData = terrainData;
}

const TerrainData& StaticMeshProcesser:: getTerrainData(size_t index) const
{
    assert(index < mTerrainData.size());

    return mTerrainData[index];
}

void StaticMeshProcesser::performCollisionDetection(const Ogre::Vector3& pos, const Ogre::Vector3& coreBaseGlobal, Ogre::Real collisionDistance)
{
    Ogre::Vector3 posL = pos;
    posL.z = -posL.z;//original data is left hand

    Ogre::Vector3 coreBaseGlobalL = coreBaseGlobal;
    coreBaseGlobalL.z = -coreBaseGlobalL.z;//original data is left hand

    mCollisionDetection.performCollisionDetection(posL, coreBaseGlobalL, collisionDistance);
}

bool StaticMeshProcesser::collideSphere(const Ogre::Vector3& spherePos, Ogre::Real radius, Ogre::Real tol, 
                                        const Ogre::Vector3& averagedPos, Ogre::Real averageLen,
                                        size_t& foundIndex, Ogre::Real& minDist) const
{
    return mCollisionDetection.collideSphere(spherePos, radius, tol, 
        averagedPos, averageLen, 
        foundIndex, minDist);
}

const FoundCollision& StaticMeshProcesser::getCollision(size_t index) const
{
    return mCollisionDetection.getCollision(index);
}

const std::vector<size_t>& StaticMeshProcesser::getArrayOfCollisions() const
{
    return mCollisionDetection.getArrayOfCollisions();
}

bool StaticMeshProcesser::performPointCollisionDetection(const Ogre::Ray& ray,
        Ogre::Vector3& collisionPoint,
        short& partIndex, short& triangleIndex)
{
    return mCollisionDetection.performPointCollisionDetection(ray, collisionPoint, partIndex, triangleIndex);
}

void StaticMeshProcesser::getGeoverts(short partIndex, short triangleIndex, Ogre::Vector3& pA, Ogre::Vector3& pC, Ogre::Vector3& pB) const
{
    mCollisionDetection.getGeoverts(partIndex, triangleIndex, pA, pC, pB);
}

void StaticMeshProcesser::getGeoverts(const FoundCollision& collision, Ogre::Vector3& pA, Ogre::Vector3& pC, Ogre::Vector3& pB) const
{
    mCollisionDetection.getGeoverts(collision, pA, pC, pB);
}

void StaticMeshProcesser::getGeovertsTexture(const FoundCollision& collision, Ogre::Vector2& pA, Ogre::Vector2& pC, Ogre::Vector2& pB) const
{
    mCollisionDetection.getGeovertsTexture(collision, pA, pC, pB);
}

const std::string& StaticMeshProcesser::getTerrainName(const FoundCollision& collision) const
{
    return mCollisionDetection.getTerrainName(collision);
}

const Ogre::Image * StaticMeshProcesser::getTerrainMap(const std::string& terrainMapName) const
{
    const Ogre::Image * ret = NULL;
    if(terrainMapName != "")
    {
        std::map<std::string, CommonIncludes::shared_ptr<Ogre::Image> >::const_iterator im = mTerrainMaps.find(terrainMapName);
        CommonIncludes::shared_ptr<Ogre::Image> imagePtr = (*im).second;
        ret = imagePtr.get();
    }
    return ret;
}

char StaticMeshProcesser::getTerrainType(const Ogre::Image * terrainMap, Ogre::Vector2 texCoord) const
{
    char ret = -1;

    if(terrainMap != NULL)
    {
        float pixel_xf = (terrainMap->getWidth() - 1.0f) * texCoord.x;
        float pixel_yf = (terrainMap->getHeight() - 1.0f) * texCoord.y;

        double integer;
        float frac = static_cast<float>(modf(pixel_xf, &integer));

        size_t pixel_x = static_cast<size_t>(frac > 0.5f ? pixel_xf + 0.5f : pixel_xf);

        frac = static_cast<float>(modf(pixel_yf, &integer));
        size_t pixel_y = static_cast<size_t>(frac > 0.5f ? pixel_yf + 0.5f : pixel_yf);

        Ogre::ColourValue color = terrainMap->getColourAt(pixel_x, pixel_y, 0);

        ret = static_cast<char>(color.r * 255.0f);

        /*
        if(pixel_x & 1)//odd
            ret &= 0xF;
        else
            ret >>= 4;
        */

        ret = Ogre::Math::Clamp<char>(ret, 0, 15);
    }

    return ret;
}
