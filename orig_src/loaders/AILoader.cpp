
#include "AILoader.h"

#include <cstring>

#include "../tools/Conversions.h"
#include "../tools/Randomizer.h"

#if defined(WII) || defined(__wii__)
#include "../WiiDebugLog.h"
#endif

namespace
{
#if defined(WII) || defined(__wii__)
    static Ogre::uint32 readLEU32(const Ogre::DataStreamPtr& stream)
    {
        unsigned char bytes[4] = {0, 0, 0, 0};
        if(!stream.get() || stream->read(bytes, 4) != 4)
            return 0;

        return static_cast<Ogre::uint32>(
            static_cast<Ogre::uint32>(bytes[0]) |
            (static_cast<Ogre::uint32>(bytes[1]) << 8) |
            (static_cast<Ogre::uint32>(bytes[2]) << 16) |
            (static_cast<Ogre::uint32>(bytes[3]) << 24));
    }

    static float readLEF32(const Ogre::DataStreamPtr& stream)
    {
        union
        {
            Ogre::uint32 u;
            float f;
        } conv;
        conv.u = readLEU32(stream);
        return conv.f;
    }

    static void readLEF32Array(const Ogre::DataStreamPtr& stream, float* out, size_t count)
    {
        for(size_t i = 0; i < count; ++i)
            out[i] = readLEF32(stream);
    }

    static void readLEU32Array(const Ogre::DataStreamPtr& stream, Ogre::uint32* out, size_t count)
    {
        for(size_t i = 0; i < count; ++i)
            out[i] = readLEU32(stream);
    }
#endif
}

AILoader::AILoader() : mNameGenNodes("Scene/Node/AI/Name")
{}

void AILoader::load(GameState& gameState, Ogre::SceneManager* sceneMgr, bool isDebugAI) const
{
    mIsLoaded = false;

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[AI] load start\n");
#endif

    std::string folderWithData = "chareasy";

    switch(gameState.getAIStrength())
    {
    case Medium :
        folderWithData = "charmedium";
        break;
    case Hard :
        folderWithData = "charhard";
        break;
    case Insane : case UltraInsane:
        folderWithData = "charinsane";
        break;
    }

    std::vector<AIWhole> aiWhole;
    for(size_t w = 0; w < GameState::mRaceGridCarsMax; ++w)
    {
        aiWhole.push_back(AIWhole());

        //read SLOT
        {
            Ogre::DataStreamPtr fileToLoad = gameState.getPFLoaderData().getFile("data/tracks/" + gameState.getSTRPowerslide().getBaseDir(gameState.getTrackName()) + "/ai/" + folderWithData, "slot" + Conversions::DMToString(w));
            if(fileToLoad.get() && fileToLoad->isReadable())
            {

                Ogre::uint32 someBuf[8];
#if defined(WII) || defined(__wii__)
                readLEU32Array(fileToLoad, someBuf, 8);
#else
                fileToLoad->read(someBuf, 4 * 8);
#endif

                Ogre::uint32 slotMatrixSize = someBuf[2];

#if defined(WII) || defined(__wii__)
                if(slotMatrixSize == 0 || slotMatrixSize > 512)
                {
                    WiiDebugLog("[AI] slot%u invalid slotMatrixSize=%u\n", static_cast<unsigned int>(w), static_cast<unsigned int>(slotMatrixSize));
                    fileToLoad->close();
                    continue;
                }
#endif


                aiWhole[w].slotMatrix.resize(slotMatrixSize);
                aiWhole[w].activation.resize(5);
                aiWhole[w].remapper.resize(slotMatrixSize);

                for(size_t q = 0; q < slotMatrixSize; ++q)
                {
                    aiWhole[w].slotMatrix[q].resize(39);
#if defined(WII) || defined(__wii__)
                    readLEF32Array(fileToLoad, &aiWhole[w].slotMatrix[q][0], 39);
#else
                    fileToLoad->read(&aiWhole[w].slotMatrix[q][0], 4 * 39);
#endif
                    aiWhole[w].slotMatrix[q][0] += Randomizer::GetInstance().GetRandomFloat(-1.0f, 1.0f);
                }

                for(size_t q = 0; q < 5; ++q)
                {
                    aiWhole[w].activation[q].resize(31);
#if defined(WII) || defined(__wii__)
                    readLEF32Array(fileToLoad, &aiWhole[w].activation[q][0], 31);
#else
                    fileToLoad->read(&aiWhole[w].activation[q][0], 4 * 31);
#endif
                }

                for(size_t q = 0; q < 2; ++q)
                {
                    float someBufMore[48];
#if defined(WII) || defined(__wii__)
                    readLEF32Array(fileToLoad, someBufMore, 48);
#else
                    fileToLoad->read(someBufMore, 4 * 48);
#endif
                }

                for(size_t q = 0; q < 5; ++q)
                {
                    float someBufMore[12];
#if defined(WII) || defined(__wii__)
                    readLEF32Array(fileToLoad, someBufMore, 12);
#else
                    fileToLoad->read(someBufMore, 4 * 12);
#endif
                }

                for(size_t q = 0; q < 3; ++q)
                {
                    float someBufMore[3];
#if defined(WII) || defined(__wii__)
                    readLEF32Array(fileToLoad, someBufMore, 3);
#else
                    fileToLoad->read(someBufMore, 4 * 3);
#endif
                }

                {
                    float someData;
#if defined(WII) || defined(__wii__)
                    someData = readLEF32(fileToLoad);
#else
                    fileToLoad->read(&someData, 4);
#endif
                }

                {
#if defined(WII) || defined(__wii__)
                    aiWhole[w].multiplier.x = readLEF32(fileToLoad);
                    aiWhole[w].multiplier.y = readLEF32(fileToLoad);
                    aiWhole[w].multiplier.z = readLEF32(fileToLoad);
#else
                    fileToLoad->read(&aiWhole[w].multiplier, 4 * 3);
#endif
                }

                for(size_t q = 0; q < slotMatrixSize; ++q)
                {
                    aiWhole[w].remapper[q].resize(6);
#if defined(WII) || defined(__wii__)
                    readLEU32Array(fileToLoad, &aiWhole[w].remapper[q][0], 6);
#else
                    fileToLoad->read(&aiWhole[w].remapper[q][0], 4 * 6);
#endif
                }


                fileToLoad->close();
            }
        }//read SLOT END

        //read REC
        {
            Ogre::DataStreamPtr fileToLoad = gameState.getPFLoaderData().getFile("data/tracks/" + gameState.getSTRPowerslide().getBaseDir(gameState.getTrackName()) + "/ai/" + folderWithData, "rec" + Conversions::DMToString(w));

            if(fileToLoad.get() && fileToLoad->isReadable())
            {
                //gameState.getAICar(w).clearAIData();

                float someBuf[9];
#if defined(WII) || defined(__wii__)
                readLEF32Array(fileToLoad, someBuf, 9);
#else
                fileToLoad->read(someBuf, 4 * 9);
#endif

                //someBuf[0] == 12345
                int hackType = 0;
                std::memcpy(&hackType, &someBuf[1], sizeof(hackType));
                aiWhole[w].hackType = static_cast<size_t>(hackType);
                aiWhole[w].accelerationCoeff = someBuf[2];
                aiWhole[w].velocityScale = someBuf[3];
                aiWhole[w].tractionCoeff = someBuf[4];
                aiWhole[w].aiImpulseScale = someBuf[5];
                aiWhole[w].hack1 = someBuf[6];
                aiWhole[w].hack2 = someBuf[7];
                aiWhole[w].hackMultiplier = someBuf[8];

                Ogre::uint32 someData;
#if defined(WII) || defined(__wii__)
                someData = readLEU32(fileToLoad);
#else
                fileToLoad->read(&someData, 4);
#endif

                size_t recPointCount = 0;

                while(someData)
                {
                    Ogre::uint32 someData2;
#if defined(WII) || defined(__wii__)
                    someData2 = readLEU32(fileToLoad);
#else
                    fileToLoad->read(&someData2, 4);
#endif

#if defined(WII) || defined(__wii__)
                    if(someData2 > 8192)
                    {
                        WiiDebugLog("[AI] rec%u invalid block count=%u\n", static_cast<unsigned int>(w), static_cast<unsigned int>(someData2));
                        break;
                    }
#endif

                    for(size_t q = 0; q < someData2; ++q)
                    {
                        AIData aiData;

                        float someBuf2[9];//x, y, z, normx?, normy?, normz?, unknown, unknown, unknown
#if defined(WII) || defined(__wii__)
                        readLEF32Array(fileToLoad, someBuf2, 9);
#else
                        fileToLoad->read(someBuf2, 36);
#endif

                        aiData.pos.x = someBuf2[0];
                        aiData.pos.y = someBuf2[1];
                        aiData.pos.z = -someBuf2[2];//original data is left hand

                        aiData.tangent.x = someBuf2[3];
                        aiData.tangent.y = someBuf2[4];
                        aiData.tangent.z = -someBuf2[5];//original data is left hand

                        aiData.magic.x = someBuf2[6];
                        aiData.magic.y = someBuf2[7];
                        aiData.magic.z = -someBuf2[8];//original data is left hand

                        aiWhole[w].aiData.push_back(aiData);
                        ++recPointCount;
                    }

#if defined(WII) || defined(__wii__)
                    someData = readLEU32(fileToLoad);
#else
                    fileToLoad->read(&someData, 4);
#endif
                }

#if defined(WII) || defined(__wii__)
                WiiDebugLog("[AI] rec%u points=%u\n", static_cast<unsigned int>(w), static_cast<unsigned int>(recPointCount));
#endif

                if(isDebugAI)
                {
                    Ogre::SimpleSpline spline;

                    for(size_t q = 0; q < aiWhole[w].aiData.size(); ++q)
                    {
                        //spheres
                        Ogre::Real scale = 0.1f;
                        Ogre::String debugSphereName = mNameGenNodes.generate();
                        Ogre::Entity * debugSphere = sceneMgr->createEntity(debugSphereName, Ogre::SceneManager::PT_SPHERE);
                        debugSphere->setMaterialName("BaseWhiteNoLighting");
                        Ogre::SceneNode * debugSphereNode = sceneMgr->getRootSceneNode()->createChildSceneNode(debugSphereName);
                        debugSphereNode->attachObject(debugSphere);
                        debugSphereNode->setPosition(aiWhole[w].aiData[q].pos);
                        debugSphereNode->setScale(scale, scale, scale);
                        debugSphere->setCastShadows(false);

                        //dirs
                        Ogre::Real length = 15.0f;
                        Ogre::String debugDirName = mNameGenNodes.generate();

                        Ogre::ManualObject * manual =  sceneMgr->createManualObject(debugDirName); 
                        Ogre::SceneNode* lltDirNode = sceneMgr->getRootSceneNode()->createChildSceneNode(debugDirName); 
                         
                        manual->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_LIST); 

                        spline.addPoint(aiWhole[w].aiData[q].pos);

                        manual->position(aiWhole[w].aiData[q].pos);
                        manual->position(aiWhole[w].aiData[q].pos + aiWhole[w].aiData[q].tangent * length);

                        manual->position(aiWhole[w].aiData[q].pos);
                        manual->position(aiWhole[w].aiData[q].pos + aiWhole[w].aiData[q].magic * length);

                        manual->end(); 
                        manual->setCastShadows(false);
                        lltDirNode->attachObject(manual);

                    }

                    spline.addPoint(aiWhole[w].aiData[0].pos);

                    //lines
                    Ogre::String debugDirName = mNameGenNodes.generate();
                    Ogre::ManualObject * manual =  sceneMgr->createManualObject(debugDirName); 
                    Ogre::SceneNode* lltDirNode = sceneMgr->getRootSceneNode()->createChildSceneNode(debugDirName); 
                    manual->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_LINE_LIST); 
                    for(size_t q = 0; q < aiWhole[w].aiData.size() - 1; ++q)
                    {
#if 1
                        manual->position(aiWhole[w].aiData[q].pos);
                        manual->position(aiWhole[w].aiData[q + 1].pos);
#else
                        for(float w = 0; w < 1.0f; w += 0.1f)
                        {
                            manual->position(spline.interpolate(q, w));
                            manual->position(spline.interpolate(q, w + 0.1f));
                        }
#endif
                    }
#if 1
                    manual->position(aiWhole[w].aiData[aiWhole[w].aiData.size() - 1].pos);
                    manual->position(aiWhole[w].aiData[0].pos);
#else
                    for(float w = 0; w < 1.0f; w += 0.1f)
                    {
                        manual->position(spline.interpolate(aiWhole[w].aiData.size() - 1, w));
                        manual->position(spline.interpolate(aiWhole[w].aiData.size() - 1, w + 0.1f));
                    }
#endif
                    manual->end(); 
                    manual->setCastShadows(false);
                    lltDirNode->attachObject(manual);
                }

                fileToLoad->close();

            }
        }//read REC END

    }

    for(size_t w = 0; w < gameState.getAICountInRace(); ++w)
    {
        size_t slotIndex = gameState.getAICar(w).getSlotIndex();
        if(slotIndex < aiWhole.size() && !aiWhole[slotIndex].aiData.empty())
        {
            gameState.getAICar(w).setAIData(aiWhole[slotIndex], sceneMgr, isDebugAI);
        }
        else if(!aiWhole.empty() && !aiWhole[0].aiData.empty())
        {
            gameState.getAICar(w).setAIData(aiWhole[0], sceneMgr, isDebugAI);
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[AI] slot %u invalid/empty, fallback to slot0\n", static_cast<unsigned int>(slotIndex));
#endif
        }
        else
        {
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[AI] slot %u invalid and no fallback aiData\n", static_cast<unsigned int>(slotIndex));
#endif
        }
    }

    mIsLoaded = true;

#if defined(WII) || defined(__wii__)
    WiiDebugLog("[AI] load end aiCount=%u\n", static_cast<unsigned int>(gameState.getAICountInRace()));
#endif
}
