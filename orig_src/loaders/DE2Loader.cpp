
#include <set>
#include <map>

#include "DE2Loader.h"
#include "../tools/Conversions.h"
#include "../WiiDebugLog.h"
#include <cstring>

namespace DE2
{
#if defined(WII) || defined(__wii__)
    static inline Ogre::uint32 bswap32(Ogre::uint32 v)
    {
        return __builtin_bswap32(v);
    }
    static inline Ogre::uint16 bswap16(Ogre::uint16 v)
    {
        return __builtin_bswap16(v);
    }
#endif

    Ogre::uint16 readU16LE(const Ogre::DataStreamPtr& stream)
    {
#if defined(WII) || defined(__wii__)
        Ogre::uint16 raw = 0;
        stream->read(&raw, 2);
        return bswap16(raw);
#else
        unsigned char bytes[2] = {0, 0};
        stream->read(bytes, 2);
        return static_cast<Ogre::uint16>(bytes[0] | (bytes[1] << 8));
#endif
    }

    Ogre::uint32 readU32LE(const Ogre::DataStreamPtr& stream)
    {
#if defined(WII) || defined(__wii__)
        Ogre::uint32 raw = 0;
        stream->read(&raw, 4);
        return bswap32(raw);
#else
        unsigned char bytes[4] = {0, 0, 0, 0};
        stream->read(bytes, 4);
        return static_cast<Ogre::uint32>(bytes[0]) |
               (static_cast<Ogre::uint32>(bytes[1]) << 8) |
               (static_cast<Ogre::uint32>(bytes[2]) << 16) |
               (static_cast<Ogre::uint32>(bytes[3]) << 24);
#endif
    }

    float readF32LE(const Ogre::DataStreamPtr& stream)
    {
        Ogre::uint32 raw = readU32LE(stream);
        float value = 0.0f;
        std::memcpy(&value, &raw, sizeof(value));
        return value;
    }

    void readVec3LE(const Ogre::DataStreamPtr& stream, float& x, float& y, float& z)
    {
        x = readF32LE(stream);
        y = readF32LE(stream);
        z = readF32LE(stream);
    }

    void readAABBLE(const Ogre::DataStreamPtr& stream, AABB& aabb)
    {
        readVec3LE(stream, aabb.min.x, aabb.min.y, aabb.min.z);
        readVec3LE(stream, aabb.max.x, aabb.max.y, aabb.max.z);
    }

    DE2_File::DE2_File()
    {
        Parts=0;
        Vertexes=0;
        TexCoords=0;
        TexCoordsDecal=0;
        TexturePathCount=0;
        TerranPathCount=0;
    }

    DE2_File::~DE2_File()
    {
        Clear();
    }

    void DE2_File::Clear()
    {
        Parts=0;
        Vertexes=0;
        TexCoords=0;
        TexCoordsDecal=0;
        TexturePathCount=0;
        TerranPathCount=0;
                
        Data_Vertexes.clear();
        Data_Texture_Coord.clear();
        Data_Texture_Coord_Decal.clear();
        Data_Parts.clear();
        Data_TexturePath.clear();
        Data_TerranName.clear();
        CollisionInfo_Parts.clear();
        CollisionInfo_Global.clear();
    }

    bool Find_xV4(const Ogre::DataStreamPtr& stream, int count)
    {
        Ogre::uint32 found = 0;
        const Ogre::uint32 xV4 = 0x12345678;//305419896
        const size_t streamSize = stream->size();

        while(found != static_cast<Ogre::uint32>(count))
        {
            if(stream->tell() + 4 > streamSize)
            {
                return false;
            }
            const Ogre::uint32 temp = readU32LE(stream);
            if(temp == xV4)
            {
                ++found;
            }
            else
            {
                stream->seek(stream->tell() - 3);
            }
        }

        return true;
    }

#if defined(WII) || defined(__wii__)
    void dumpNextWords(const Ogre::DataStreamPtr& stream, const char* tag, int wordCount)
    {
        const size_t pos = stream->tell();
        const size_t streamSize = stream->size();
        WiiDebugLog("[DE2] %s tell=%u\n", tag, static_cast<unsigned int>(pos));
        for(int i = 0; i < wordCount; ++i)
        {
            if(pos + static_cast<size_t>((i + 1) * 4) > streamSize)
                break;
            Ogre::uint32 word = 0;
            stream->read(&word, 4);
            WiiDebugLog("[DE2] %s word[%d]=%08x\n", tag, i, static_cast<unsigned int>(word));
        }
        stream->seek(pos);
    }
#endif

    int loadCollisionInfo(DE2_File & DE2, DE2_CollisionInfo& collisionInfo, const AABB& aabbPrev, const Ogre::DataStreamPtr& stream, int curTri)
    {
        typedef unsigned char BYTE;
        typedef unsigned short WORD;


        float ss_1;
        BYTE ss_2;
        BYTE ss_3;
        BYTE ss_4;
        BYTE ss_5;
        BYTE ss_6;
        BYTE ss_7;
        WORD subCount;

        ss_1 = readF32LE(stream);
        stream->read(&ss_2,1);
        stream->read(&ss_3,1);
        stream->read(&ss_4,1);
        stream->read(&ss_5,1);
        stream->read(&ss_6,1);
        stream->read(&ss_7,1);
        subCount = readU16LE(stream);

        DE2_CollisionInfo collisionInfoLocal;
        collisionInfoLocal.aabb.min.x = aabbPrev.min.x + ss_1 * static_cast<float>(ss_2);
        collisionInfoLocal.aabb.min.y = aabbPrev.min.y + ss_1 * static_cast<float>(ss_3);
        collisionInfoLocal.aabb.min.z = aabbPrev.min.z + ss_1 * static_cast<float>(ss_4);
        collisionInfoLocal.aabb.max.x = aabbPrev.min.x + ss_1 * static_cast<float>(ss_5);
        collisionInfoLocal.aabb.max.y = aabbPrev.min.y + ss_1 * static_cast<float>(ss_6);
        collisionInfoLocal.aabb.max.z = aabbPrev.min.z + ss_1 * static_cast<float>(ss_7);

        int totalsum = 0;

        if(subCount)
        {
            for(int q = 0; q < subCount; ++q)
                totalsum += loadCollisionInfo(DE2, collisionInfoLocal, collisionInfoLocal.aabb, stream, q);
        }
        else
        {
            collisionInfoLocal.triIndex = static_cast<short>(readU16LE(stream));
            totalsum = 1;
        }

        //collisionInfoLocal.aabb.min.z = -collisionInfoLocal.aabb.min.z;//original data is left hand
        //collisionInfoLocal.aabb.max.z = -collisionInfoLocal.aabb.max.z;//original data is left hand
        //std::swap(collisionInfoLocal.aabb.min.z, collisionInfoLocal.aabb.max.z);
        collisionInfo.subparts.push_back(collisionInfoLocal);

        return totalsum;
    }

    void loadViewHierInfo(DE2_File & DE2, DE2_CollisionInfo& collisionInfo, const Ogre::DataStreamPtr& stream)
    {

        typedef unsigned short WORD;

        DE2_CollisionInfo collisionInfoLocal;

        readAABBLE(stream, collisionInfoLocal.aabb);

        WORD subCount;
        subCount = readU16LE(stream);

        if(subCount)
        {
            for(int q = 0; q < subCount; ++q)
                loadViewHierInfo(DE2, collisionInfoLocal, stream);
        }
        else
        {
            collisionInfoLocal.triIndex = static_cast<short>(readU16LE(stream));//part index here
        }

        //collisionInfoLocal.aabb.min.z = -collisionInfoLocal.aabb.min.z;//original data is left hand
        //collisionInfoLocal.aabb.max.z = -collisionInfoLocal.aabb.max.z;//original data is left hand
        //std::swap(collisionInfoLocal.aabb.min.z, collisionInfoLocal.aabb.max.z);
        collisionInfo.subparts.push_back(collisionInfoLocal);
    }

    void readDE2(const Ogre::DataStreamPtr& stream, DE2_File & DataDE2)
    {
        typedef unsigned char BYTE;
        typedef unsigned short WORD;

        if(stream.get() && stream->isReadable())
        {
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[DE2] read start size=%u\n", static_cast<unsigned int>(stream->size()));
#endif
            BYTE ch=1;
            Ogre::uint32 temp;

            const size_t streamSize = stream->size();
            while(ch!=0)
            {
                if(stream->tell() + 1 > streamSize)
                {
#if defined(WII) || defined(__wii__)
                    WiiDebugLog("[DE2] failed while scanning for header zero\n");
#endif
                    return;
                }
                stream->read(&ch,1);
            }
            temp = readU32LE(stream);

            DataDE2.Vertexes = readU32LE(stream);
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[DE2] vertexCount=%u\n", static_cast<unsigned int>(DataDE2.Vertexes));
#endif
            DataDE2.Data_Vertexes.clear();
            DataDE2.Data_Vertexes.resize(DataDE2.Vertexes);

            for(Ogre::uint32 q=0;q<DataDE2.Vertexes;q++)
            {
                readVec3LE(stream, DataDE2.Data_Vertexes[q].x, DataDE2.Data_Vertexes[q].y, DataDE2.Data_Vertexes[q].z);
            }

            //Decal TexVerts
            if(!Find_xV4(stream,1))
            {
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[DE2] Find_xV4 failed at decal texverts\n");
#endif
                return;
            }
            DataDE2.TexCoordsDecal = readU32LE(stream);
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[DE2] texDecal=%u\n", static_cast<unsigned int>(DataDE2.TexCoordsDecal));
#endif
            if(DataDE2.TexCoordsDecal!=0)
            {
                DataDE2.Data_Texture_Coord_Decal.clear();
                DataDE2.Data_Texture_Coord_Decal.resize(DataDE2.TexCoordsDecal);
                for(Ogre::uint32 q=0;q<DataDE2.TexCoordsDecal;q++)
                {
                    DataDE2.Data_Texture_Coord_Decal[q].uv = readF32LE(stream);
                    DataDE2.Data_Texture_Coord_Decal[q].uw = readF32LE(stream);
                    DataDE2.Data_Texture_Coord_Decal[q].uz = 0.0f;
                    DataDE2.Data_Texture_Coord_Decal[q].r = 0xFF;
                    DataDE2.Data_Texture_Coord_Decal[q].g = 0xFF;
                    DataDE2.Data_Texture_Coord_Decal[q].b = 0xFF;
                }
            }

            //Prelit TexVerts
            if(!Find_xV4(stream,1))
            {
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[DE2] Find_xV4 failed at prelit texverts\n");
#endif
                return;
            }
            DataDE2.TexCoords = readU32LE(stream);
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[DE2] texCoordsPrelit=%u\n", static_cast<unsigned int>(DataDE2.TexCoords));
#endif
            if(DataDE2.TexCoords==0)
            {
                DataDE2.TexCoorTypeOne=false;
            }
            else
            {
                DataDE2.TexCoorTypeOne=true;
                DataDE2.Data_Texture_Coord.clear();
                DataDE2.Data_Texture_Coord.resize(DataDE2.TexCoords);

#if defined(WII) || defined(__wii__)
                WiiDebugLog("[DE2] prelit loop begin count=%u\n", static_cast<unsigned int>(DataDE2.TexCoords));
#endif

                for(Ogre::uint32 q=0;q<DataDE2.TexCoords;q++)
                {
                    DataDE2.Data_Texture_Coord[q].uv = readF32LE(stream);
                    DataDE2.Data_Texture_Coord[q].uw = readF32LE(stream);
                    DataDE2.Data_Texture_Coord[q].uz = readF32LE(stream);
                    Ogre::uint32 colorMask = 0;
                    std::memcpy(&colorMask, &DataDE2.Data_Texture_Coord[q].uz, sizeof(colorMask));
                    DataDE2.Data_Texture_Coord[q].r = colorMask & 0xFF;
                    DataDE2.Data_Texture_Coord[q].g = (colorMask >> 8) & 0xFF;
                    DataDE2.Data_Texture_Coord[q].b = (colorMask >> 16 ) & 0xFF;

#if defined(WII) || defined(__wii__)
                    if(q == 0)
                    {
                        WiiDebugLog("[DE2] prelit first uv=(%.3f,%.3f,%.3f) mask=%08x\n",
                            DataDE2.Data_Texture_Coord[q].uv,
                            DataDE2.Data_Texture_Coord[q].uw,
                            DataDE2.Data_Texture_Coord[q].uz,
                            static_cast<unsigned int>(colorMask));
                    }
                    if(q + 1 == DataDE2.TexCoords)
                    {
                        WiiDebugLog("[DE2] prelit loop end\n");
                    }
#endif
               }
            }

            //Lit TexVerts
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[DE2] before lit Find_xV4 tell=%u\n", static_cast<unsigned int>(stream->tell()));
            dumpNextWords(stream, "after_prelit", 8);
#endif
            if(!Find_xV4(stream,1))
            {
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[DE2] lit texverts marker missing, assuming zero and continuing\n");
#endif
                temp = 0;
            }
            else
            {
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[DE2] after lit Find_xV4 tell=%u\n", static_cast<unsigned int>(stream->tell()));
#endif
                temp = readU32LE(stream);
            }
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[DE2] texCoordsLit=%u\n", static_cast<unsigned int>(temp));
#endif
            if(!DataDE2.TexCoorTypeOne)
            {
                DataDE2.TexCoords=temp;
                DataDE2.Data_Texture_Coord.clear();
                DataDE2.Data_Texture_Coord.resize(DataDE2.TexCoords);

                for(Ogre::uint32 q=0;q<DataDE2.TexCoords;q++)
                {
                    DataDE2.Data_Texture_Coord[q].uv = readF32LE(stream);
                    DataDE2.Data_Texture_Coord[q].uw = readF32LE(stream);
                    DataDE2.Data_Texture_Coord[q].uz = readF32LE(stream);
                    stream->seek(stream->tell() + 3);
                }
            }

            //Lights
#if defined(WII) || defined(__wii__)
            dumpNextWords(stream, "before_lights", 8);
#endif
            if(!Find_xV4(stream,1))
            {
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[DE2] Find_xV4 failed at lights\n");
#endif
                return;
            }
            Ogre::uint32 ligthsCount = readU32LE(stream);
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[DE2] lights=%u\n", static_cast<unsigned int>(ligthsCount));
#endif
            for(Ogre::uint32 q = 0; q < ligthsCount; ++q)
            {
                DE2_Light light;
                light.position.x = readF32LE(stream);
                light.position.y = readF32LE(stream);
                light.position.z = readF32LE(stream);
                light.r = readF32LE(stream);
                light.g = readF32LE(stream);
                light.b = readF32LE(stream);
                light.rangeIn = readF32LE(stream);
                light.rangeOut = readF32LE(stream);
                Ogre::uint32 someCount = readU32LE(stream);
                (void)someCount;
                DataDE2.lights.push_back(light);
            }

            //Meshes
#if defined(WII) || defined(__wii__)
            dumpNextWords(stream, "before_meshes", 8);
#endif
            if(!Find_xV4(stream,1))
            {
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[DE2] Find_xV4 failed at meshes\n");
#endif
                return;
            }
            DataDE2.Parts = readU32LE(stream);
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[DE2] parts=%u\n", static_cast<unsigned int>(DataDE2.Parts));
#endif
            DataDE2.Data_Parts.clear();
            DataDE2.Data_Parts.resize(DataDE2.Parts);
            DataDE2.CollisionInfo_Parts.clear();
            DataDE2.CollisionInfo_Parts.resize(DataDE2.Parts);

            for(Ogre::uint32 q=0;q<DataDE2.Parts;q++)
            {
                if(q!=0 && !Find_xV4(stream,1))
                {
#if defined(WII) || defined(__wii__)
                    WiiDebugLog("[DE2] Find_xV4 failed at part %u\n", static_cast<unsigned int>(q));
#endif
                    return;
                }

                DataDE2.Data_Parts[q].IsMainPart = readU32LE(stream);
                DataDE2.Data_Parts[q].Triangles = readU32LE(stream);
#if defined(WII) || defined(__wii__)
                if(q == 0)
                {
                    WiiDebugLog("[DE2] part0 header main=%u tris=%u tell=%u\n",
                        static_cast<unsigned int>(DataDE2.Data_Parts[q].IsMainPart),
                        static_cast<unsigned int>(DataDE2.Data_Parts[q].Triangles),
                        static_cast<unsigned int>(stream->tell()));
                }
#endif
                DataDE2.Data_Parts[q].Data_Triangles.clear();
                DataDE2.Data_Parts[q].Data_Triangles.resize(DataDE2.Data_Parts[q].Triangles);

                for(Ogre::uint32 w=0;w<DataDE2.Data_Parts[q].Triangles;w++)
                {
                    DataDE2.Data_Parts[q].Data_Triangles[w].v0 = readU16LE(stream);
                    DataDE2.Data_Parts[q].Data_Triangles[w].v1 = readU16LE(stream);
                    DataDE2.Data_Parts[q].Data_Triangles[w].v2 = readU16LE(stream);

                    DataDE2.Data_Parts[q].Data_Triangles[w].t0 = readU16LE(stream);
                    DataDE2.Data_Parts[q].Data_Triangles[w].t1 = readU16LE(stream);
                    DataDE2.Data_Parts[q].Data_Triangles[w].t2 = readU16LE(stream);

                    DataDE2.Data_Parts[q].Data_Triangles[w].hz0 = readU16LE(stream);
                    DataDE2.Data_Parts[q].Data_Triangles[w].hz1 = readU16LE(stream);
                }
#if defined(WII) || defined(__wii__)
                if(q == 0)
                {
                    WiiDebugLog("[DE2] part0 triangles parsed tell=%u\n", static_cast<unsigned int>(stream->tell()));
                }
#endif

                Ogre::uint32 switcher = readU32LE(stream);
#if defined(WII) || defined(__wii__)
                if(q == 0)
                {
                    WiiDebugLog("[DE2] part0 collision switcher=%u tell=%u\n",
                        static_cast<unsigned int>(switcher),
                        static_cast<unsigned int>(stream->tell()));
                }
#endif

                if(switcher)
                {
                    DE2_CollisionInfo collisionInfo;
                    readAABBLE(stream, collisionInfo.aabb);

                    WORD triCount = readU16LE(stream);

                    if(triCount)
                    {
                        int totalSum = 0;
                        for(int ll = 0; ll < triCount; ++ll)
                        {
                            totalSum += loadCollisionInfo(DataDE2, collisionInfo, collisionInfo.aabb, stream, ll);
                        }
                    }
                    else
                    {
                        collisionInfo.triIndex = static_cast<short>(readU16LE(stream));
                }

                    DataDE2.CollisionInfo_Parts[q] = collisionInfo;
                }
#if defined(WII) || defined(__wii__)
                if(q == 0)
                {
                    WiiDebugLog("[DE2] part0 end tell=%u\n", static_cast<unsigned int>(stream->tell()));
                }
#endif
            }

            //viewHierarchy
            if(!Find_xV4(stream,2))
            {
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[DE2] Find_xV4 failed at viewHierarchy\n");
#endif
                return;
            }
            DataDE2.CollisionInfo_Global.clear();
            DataDE2.CollisionInfo_Global.resize(1);//root element
            loadViewHierInfo(DataDE2, DataDE2.CollisionInfo_Global[0], stream);
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[DE2] viewHierarchy parsed tell=%u\n", static_cast<unsigned int>(stream->tell()));
#endif

            //texture names
            if(!Find_xV4(stream,1))
            {
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[DE2] Find_xV4 failed at texture names\n");
#endif
                return;
            }
            DataDE2.TexturePathCount = readU32LE(stream);

            if(DataDE2.TexturePathCount)
            {
                DataDE2.Data_TexturePath.clear();
                DataDE2.Data_TexturePath.resize(DataDE2.TexturePathCount);
                for(Ogre::uint32 q=0;q<DataDE2.TexturePathCount;q++)
                {
                    temp = readU32LE(stream);
                    char buf[4096];
                    stream->read(buf, temp);
                    buf[temp] = 0;
                    DataDE2.Data_TexturePath[q] = buf;;
                }
            }
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[DE2] texture names parsed count=%u tell=%u\n",
                static_cast<unsigned int>(DataDE2.TexturePathCount),
                static_cast<unsigned int>(stream->tell()));
#endif

            //terrain names
            if(!Find_xV4(stream,1))
            {
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[DE2] Find_xV4 failed at terrain names\n");
#endif
                return;
            }
            DataDE2.TerranPathCount = readU32LE(stream);
            if(DataDE2.TerranPathCount)
            {
                DataDE2.Data_TerranName.clear();
                DataDE2.Data_TerranName.resize(DataDE2.TerranPathCount);
                for(Ogre::uint32 q=0;q<DataDE2.TerranPathCount;q++)
                {
                    temp = readU32LE(stream);
                    char buf[4096];
                    stream->read(buf, temp);
                    buf[temp] = 0;

                    std::string terrainName(buf);
                    std::string newFileName = terrainName.substr(terrainName.find_last_of("/\\") + 1);
                    std::transform(newFileName.begin(), newFileName.end(), newFileName.begin(), ::tolower);
                    //std::replace(newFileName.begin(), newFileName.end(), ' ', '_');

                    DataDE2.Data_TerranName[q] = newFileName;
                }
            }
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[DE2] terrain names parsed count=%u tell=%u\n",
                static_cast<unsigned int>(DataDE2.TerranPathCount),
                static_cast<unsigned int>(stream->tell()));
#endif

            //LODs
            if(!Find_xV4(stream,1))
            {
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[DE2] Find_xV4 failed at LODs\n");
#endif
                return;
            }
            DataDE2.RealPartsCount = readU32LE(stream);
            DataDE2.indexesForHighestLODS.clear();
            DataDE2.indexesForHighestLODS.resize(DataDE2.RealPartsCount);
            for(Ogre::uint32 q=0;q<DataDE2.RealPartsCount;q++)
            {
                Ogre::uint32 subLODSCount;
                subLODSCount = readU32LE(stream);
                for(Ogre::uint32 w = 0; w < subLODSCount; ++w)
                {
                    Ogre::uint32 partIndex;
                    float something3,something4;
                    Ogre::uint32 lodTriCount;
                    partIndex = readU32LE(stream);
                    something3 = readF32LE(stream);
                    something4 = readF32LE(stream);
                    lodTriCount = readU32LE(stream);

                    if(w == 0)
                    {
                        DataDE2.indexesForHighestLODS[q] = partIndex;
                    }
                }
            }
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[DE2] lods parsed realParts=%u lodIdx=%u tell=%u\n",
                static_cast<unsigned int>(DataDE2.RealPartsCount),
                static_cast<unsigned int>(DataDE2.indexesForHighestLODS.size()),
                static_cast<unsigned int>(stream->tell()));
#endif

            //transformation matrix

            //visibility cells more

            //vis bound box

            //soundregions

        }
    }

    int FindVertex(const std::vector<DE2_Vertex>& Vertexes, const DE2_Vertex& Vertex, size_t From)
    {
        for (size_t q = From; q < Vertexes.size(); q++)
        {
            if(
                Vertexes[q].x == Vertex.x      &&
                Vertexes[q].y == Vertex.y      &&
                Vertexes[q].z == Vertex.z
            ) return q;
        }

    return -1;
    }

    int FindTexCoor(const std::vector<DE2_TextureCoord>& TexCoord, const DE2_TextureCoord& TriCoor, int From)
    {
        for (size_t q = From; q < TexCoord.size(); q++)
        {
                if(
                    TexCoord[q].uv == TriCoor.uv   &&
                    TexCoord[q].uw == TriCoor.uw   &&
                    TexCoord[q].uz == TriCoor.uz
                ) return q;
        }

    return -1;
    }

    void processPart(const DE2_File& DataDE2, size_t PartIndex, MSHData& mshData)
    {

        mshData.clear();

        typedef unsigned char BYTE;
        typedef unsigned short WORD;

        if(PartIndex >= 0 && PartIndex < DataDE2.Parts)
        {
            std::vector<DE2_Vertex> Vertexes;
            std::vector<DE2_TextureCoord> TexC;
            std::vector<DE2_Triangle> TriTex;

            const Ogre::uint32 numTriangles = DataDE2.Data_Parts[PartIndex].Triangles;
#if defined(WII) || defined(__wii__)
            static int sProcessLogCounter = 0;
            if(sProcessLogCounter < 5)
            {
                WiiDebugLog("[DE2] processPart[%u] numTriangles=%u\n",
                    static_cast<unsigned int>(PartIndex),
                    static_cast<unsigned int>(numTriangles));
                sProcessLogCounter++;
            }
#endif
            if(numTriangles == 0)
                return;

            for(size_t q=0;q<numTriangles;q++)
            {
                WORD hz_0 = DataDE2.Data_Parts[PartIndex].Data_Triangles[q].hz0;
                bool isdecal = !(hz_0 & 3);

                Ogre::uint32 v0=DataDE2.Data_Parts[PartIndex].Data_Triangles[q].v0;
                Ogre::uint32 v1=DataDE2.Data_Parts[PartIndex].Data_Triangles[q].v1;
                Ogre::uint32 v2=DataDE2.Data_Parts[PartIndex].Data_Triangles[q].v2;

                Ogre::uint32 t0=DataDE2.Data_Parts[PartIndex].Data_Triangles[q].t0;
                Ogre::uint32 t1=DataDE2.Data_Parts[PartIndex].Data_Triangles[q].t1;
                Ogre::uint32 t2=DataDE2.Data_Parts[PartIndex].Data_Triangles[q].t2;


                int It_0=FindTexCoor(TexC, DataDE2.Data_Texture_Coord[t0],0);
                int It_1=FindTexCoor(TexC, DataDE2.Data_Texture_Coord[t1],0);
                int It_2=FindTexCoor(TexC, DataDE2.Data_Texture_Coord[t2],0);

                if(isdecal)
                {
                    It_0=FindTexCoor(TexC, DataDE2.Data_Texture_Coord_Decal[t0],0);
                    It_1=FindTexCoor(TexC, DataDE2.Data_Texture_Coord_Decal[t1],0);
                    It_2=FindTexCoor(TexC, DataDE2.Data_Texture_Coord_Decal[t2],0);
                }

                int Iv_0=FindVertex(Vertexes, DataDE2.Data_Vertexes[v0],0);
                int Iv_1=FindVertex(Vertexes, DataDE2.Data_Vertexes[v1],0);
                int Iv_2=FindVertex(Vertexes, DataDE2.Data_Vertexes[v2],0);

                if(It_0 == -1)
                {
                    if(isdecal)
                    {
                        TexC.push_back(DataDE2.Data_Texture_Coord_Decal[t0]);
                    }
                    else
                    {
                        TexC.push_back(DataDE2.Data_Texture_Coord[t0]);
                    }

                    It_0 = TexC.size() - 1;
                }

                if(It_1==-1)
                {
                    if(isdecal)
                    {
                        TexC.push_back(DataDE2.Data_Texture_Coord_Decal[t1]);
                    }
                    else
                    {
                        TexC.push_back(DataDE2.Data_Texture_Coord[t1]);
                    }

                    It_1 = TexC.size() - 1;
                }


                if(It_2==-1)
                {
                    if(isdecal)
                    {
                        TexC.push_back(DataDE2.Data_Texture_Coord_Decal[t2]);
                    }
                    else
                    {
                        TexC.push_back(DataDE2.Data_Texture_Coord[t2]);
                    }

                    It_2 = TexC.size() - 1;
                }


                Iv_0=FindVertex(Vertexes, DataDE2.Data_Vertexes[v0],0);
                Iv_1=FindVertex(Vertexes, DataDE2.Data_Vertexes[v1],0);
                Iv_2=FindVertex(Vertexes, DataDE2.Data_Vertexes[v2],0);


                if(Iv_0==-1)
                {
                    Vertexes.push_back(DataDE2.Data_Vertexes[v0]);
                    Iv_0 = Vertexes.size() - 1;
                }

                if(Iv_1==-1)
                {
                    Vertexes.push_back(DataDE2.Data_Vertexes[v1]);
                    Iv_1 = Vertexes.size() - 1;
                }

                if(Iv_2==-1)
                {
                    Vertexes.push_back(DataDE2.Data_Vertexes[v2]);
                    Iv_2 = Vertexes.size() - 1;
                }


                DE2_Triangle TriTemp;
                TriTemp.t0=It_0;
                TriTemp.t1=It_1;
                TriTemp.t2=It_2;
                TriTemp.v0=Iv_0;
                TriTemp.v1=Iv_1;
                TriTemp.v2=Iv_2;
                TriTex.push_back(TriTemp);
            }

            //store data

            mshData.vertCount = Vertexes.size();
            mshData.triCount = numTriangles;
            mshData.texCount = TexC.size();

            if(mshData.vertCount == 0 || mshData.triCount == 0)
            {
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[DE2] processPart[%u] EMPTY: verts=%u tris=%u\n",
                    static_cast<unsigned int>(PartIndex),
                    static_cast<unsigned int>(mshData.vertCount),
                    static_cast<unsigned int>(mshData.triCount));
#endif
                return;
            }

            mshData.preallocateRawData();

            for(size_t q = 0; q < Vertexes.size(); q++)
            {
                mshData.vertexes[q].x = Vertexes[q].x;
                mshData.vertexes[q].y = Vertexes[q].y;
                mshData.vertexes[q].z = -Vertexes[q].z;//original data is left hand
            }

            for(size_t q=0;q<DataDE2.Data_Parts[PartIndex].Triangles;q++)
            {
                mshData.triIndexes[q].a = TriTex[q].v0;
                mshData.triIndexes[q].b = TriTex[q].v1;
                mshData.triIndexes[q].c = TriTex[q].v2;
            }

            for(size_t q = 0; q < TexC.size(); q++)
            {
                mshData.texcoords[q].x = TexC[q].uv;
                mshData.texcoords[q].y = TexC[q].uw;
                mshData.texcoords[q].z = TexC[q].uz;

                mshData.colors[q] = Ogre::ColourValue(TexC[q].r / 255.0f, TexC[q].g / 255.0f, TexC[q].b / 255.0f);
            }

            for(size_t q=0;q<DataDE2.Data_Parts[PartIndex].Triangles;q++)
            {
                mshData.texCoordsIndexes[q].a = TriTex[q].t0;
                mshData.texCoordsIndexes[q].b = TriTex[q].t1;
                mshData.texCoordsIndexes[q].c = TriTex[q].t2;
            }

            //find unique texture names for part
            std::set<std::string> texNames;
            std::set<std::string> texNamesNew;

            typedef std::map<std::string, bool> mBool;
            mBool texNamesToDecals;

            for(size_t q=0;q<DataDE2.Data_Parts[PartIndex].Triangles;q++)
            {

                    std::string textureName = DataDE2.Data_TexturePath[DataDE2.Data_Parts[PartIndex].Data_Triangles[q].hz1];

                    texNames.insert(textureName);


                    std::string newFileName = textureName.substr(textureName.find_last_of("/\\") + 1);
                    newFileName = newFileName.substr(0, newFileName.length() - 3) + "tex";
                    std::transform(newFileName.begin(), newFileName.end(), newFileName.begin(), ::tolower);
                    //std::replace(newFileName.begin(), newFileName.end(), ' ', '_');
                    texNamesNew.insert(newFileName);

                    bool isdecal = !(DataDE2.Data_Parts[PartIndex].Data_Triangles[q].hz0 & 3);
                    mBool::iterator found = texNamesToDecals.find(newFileName);
                    if(found == texNamesToDecals.end())
                    {
                        texNamesToDecals.insert(std::make_pair(newFileName, isdecal));
                    }
                    else if(isdecal && found != texNamesToDecals.end())
                    {
                        (*found).second = isdecal;
                    }
            }

            //textures writing
            mshData.texturesCount = texNamesNew.size();

            assert(mshData.texturesCount != 0);
            mshData.preallocateTextureNames();

            {
                size_t texIndex = 0;
                for(std::set<std::string>::iterator i = texNamesNew.begin(),j = texNamesNew.end();i != j; ++i)
                {
                    mshData.textureNames[texIndex] = (*i);

                    mBool::const_iterator found = texNamesToDecals.find(*i);
                    mshData.isDecalTexture[texIndex++] = (*found).second;
                }
            }

            for(size_t q=0;q<DataDE2.Data_Parts[PartIndex].Triangles;q++)
            {
                int index = 0;
                std::string textureName = DataDE2.Data_TexturePath[DataDE2.Data_Parts[PartIndex].Data_Triangles[q].hz1];
                std::string newFileName = textureName.substr(textureName.find_last_of("/\\") + 1);
                newFileName = newFileName.substr(0, newFileName.length() - 3) + "tex";
                std::transform(newFileName.begin(), newFileName.end(), newFileName.begin(), ::tolower);
                //std::replace(newFileName.begin(), newFileName.end(), ' ', '_');

                std::set<std::string>::iterator i = texNamesNew.find(newFileName);
                index = std::distance(texNamesNew.begin(), i);

                mshData.textureForTriangleIndex[q] = index;
            }

            //fprintf(f,"PartIndex: %d\n",PartIndex);
        }//if(PartIndex >= 0 && PartIndex < DataDE2.Parts)
    }

}//DE2 namespace

bool DE2Loader::load(std::vector<MSHData>& parts, const Ogre::DataStreamPtr& fileToLoad, bool isTerrain)
{
#if defined(WII) || defined(__wii__)
    WiiDebugLog("[DE2] load entry stream=%p readable=%d size=%u\n",
        fileToLoad.get(),
        (fileToLoad.get() && fileToLoad->isReadable()) ? 1 : 0,
        fileToLoad.get() ? static_cast<unsigned int>(fileToLoad->size()) : 0);
#endif
    bool res = false;

    parts.clear();

    if(fileToLoad.get() && fileToLoad->isReadable())
    {
#if defined(WII) || defined(__wii__)
        WiiDebugLog("[DE2] load calling readDE2\n");
#endif
        mDE2.Clear();
        readDE2(fileToLoad, mDE2);

#if defined(WII) || defined(__wii__)
        WiiDebugLog("[DE2] load summary realParts=%u lodIdx=%u parts=%u tex=%u decal=%u\n",
            static_cast<unsigned int>(mDE2.RealPartsCount),
            static_cast<unsigned int>(mDE2.indexesForHighestLODS.size()),
            static_cast<unsigned int>(mDE2.Parts),
            static_cast<unsigned int>(mDE2.TexCoords),
            static_cast<unsigned int>(mDE2.TexCoordsDecal));
#endif

        if(mDE2.RealPartsCount == 0 || mDE2.indexesForHighestLODS.size() < mDE2.RealPartsCount)
        {
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[DE2] VALIDATION FAIL: realParts=%u lodIdxSize=%u check1=%d check2=%d\n",
                static_cast<unsigned int>(mDE2.RealPartsCount),
                static_cast<unsigned int>(mDE2.indexesForHighestLODS.size()),
                mDE2.RealPartsCount == 0 ? 1 : 0,
                static_cast<unsigned int>(mDE2.indexesForHighestLODS.size()) < mDE2.RealPartsCount ? 1 : 0);
#endif
            return false;
        }

        for(size_t q = 0; q < mDE2.RealPartsCount; q++)
        {
            MSHData mshData;
            int partIndex = mDE2.indexesForHighestLODS[q];
#if defined(WII) || defined(__wii__)
            if(q < 5)
            {
                WiiDebugLog("[DE2] part[%u] lodIndex=%d parts=%u\n",
                    static_cast<unsigned int>(q),
                    partIndex,
                    static_cast<unsigned int>(mDE2.Data_Parts.size()));
            }
#endif
            if(partIndex < 0 || static_cast<size_t>(partIndex) >= mDE2.Data_Parts.size())
            {
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[DE2] invalid partIndex=%d at q=%u parts=%u\n",
                    partIndex,
                    static_cast<unsigned int>(q),
                    static_cast<unsigned int>(mDE2.Data_Parts.size()));
#endif
                continue;
            }
            processPart(mDE2, partIndex, mshData);
#if defined(WII) || defined(__wii__)
            if(q < 5)
            {
                WiiDebugLog("[DE2] part[%u] processed verts=%u tris=%u\n",
                    static_cast<unsigned int>(q),
                    static_cast<unsigned int>(mshData.vertCount),
                    static_cast<unsigned int>(mshData.triCount));
            }
#endif
#if defined(WII) || defined(__wii__)
            if(q < 5)
            {
                WiiDebugLog("[DE2] part[%u] processed verts=%u tris=%u\n",
                    static_cast<unsigned int>(q),
                    static_cast<unsigned int>(mshData.vertCount),
                    static_cast<unsigned int>(mshData.triCount));
            }
#endif
#if defined(WII) || defined(__wii__)
            if(q < 5)
            {
                WiiDebugLog("[DE2] part[%u] processed verts=%u tris=%u\n",
                    static_cast<unsigned int>(q),
                    static_cast<unsigned int>(mshData.vertCount),
                    static_cast<unsigned int>(mshData.triCount));
            }
#endif
#if defined(WII) || defined(__wii__)
            if(q < 5)
            {
                WiiDebugLog("[DE2] part[%u] processed verts=%u tris=%u\n",
                    static_cast<unsigned int>(q),
                    static_cast<unsigned int>(mshData.vertCount),
                    static_cast<unsigned int>(mshData.triCount));
            }
#endif
#if defined(WII) || defined(__wii__)
            if(q < 5)
            {
                WiiDebugLog("[DE2] part[%u] processed verts=%u tris=%u\n",
                    static_cast<unsigned int>(q),
                    static_cast<unsigned int>(mshData.vertCount),
                    static_cast<unsigned int>(mshData.triCount));
            }
#endif

            mshData.preallocatePlainData(isTerrain);
            for(size_t qq = 0; qq < mshData.triCount; ++qq)
            {
                Ogre::Vector3 A = mshData.vertexes[mshData.triIndexes[qq].a];
                Ogre::Vector3 B = mshData.vertexes[mshData.triIndexes[qq].b];
                Ogre::Vector3 C = mshData.vertexes[mshData.triIndexes[qq].c];

                Ogre::Vector3 texA = mshData.texcoords[mshData.texCoordsIndexes[qq].a];
                Ogre::Vector3 texB = mshData.texcoords[mshData.texCoordsIndexes[qq].b];
                Ogre::Vector3 texC = mshData.texcoords[mshData.texCoordsIndexes[qq].c];

                Ogre::ColourValue colA = mshData.colors[mshData.texCoordsIndexes[qq].a];
                Ogre::ColourValue colB = mshData.colors[mshData.texCoordsIndexes[qq].b];
                Ogre::ColourValue colC = mshData.colors[mshData.texCoordsIndexes[qq].c];

                mshData.plainVertices[qq * 3 + 0] = A;
                mshData.plainVertices[qq * 3 + 1] = B;
                mshData.plainVertices[qq * 3 + 2] = C;

                if(!isTerrain)
                {
                    Ogre::Vector3 normal = (B - A).crossProduct(C - A);
                    normal.normalise();

                    mshData.plainNormals[qq * 3 + 0] = normal;
                    mshData.plainNormals[qq * 3 + 1] = normal;
                    mshData.plainNormals[qq * 3 + 2] = normal;
                }

                mshData.plainTexCoords[qq * 3 + 0] = texA;
                mshData.plainTexCoords[qq * 3 + 1] = texB;
                mshData.plainTexCoords[qq * 3 + 2] = texC;

                mshData.plainColors[qq * 3 + 0] = colA;
                mshData.plainColors[qq * 3 + 1] = colB;
                mshData.plainColors[qq * 3 + 2] = colC;
            }

            parts.push_back(mshData);
        }
        res = true;
    }

    return res;
}
