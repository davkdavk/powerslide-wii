#include "OgreSceneManager.h"
#include "Ogre.h"
#include "../../orig_src/WiiDebugLog.h"
#include <vector>
#include <algorithm>

#if defined(WII) || defined(__wii__)
#include <ogc/gx.h>
#include <ogc/gu.h>
#include <ogc/cache.h>
#endif

namespace Ogre
{
    std::vector<ManualObjectData> gManualObjects;

    namespace
    {
        std::vector<Entity*> gMeshEntities;
        std::map<Entity*, String> gPendingMeshNames;
    }

    void bindEntityToMeshName(Entity* entity, const String& meshName)
    {
        if (!entity) {
            return;
        }
        MeshPtr mesh = MeshManager::getSingleton().getByName(meshName);
        if (mesh) {
            entity->_setMesh(mesh);
            gPendingMeshNames.erase(entity);
        } else {
            gPendingMeshNames[entity] = meshName;
        }
    }

    void registerMeshEntity(Entity* entity)
    {
        if (!entity) {
            return;
        }
        if (std::find(gMeshEntities.begin(), gMeshEntities.end(), entity) == gMeshEntities.end()) {
            gMeshEntities.push_back(entity);
        }
    }

    void unregisterMeshEntity(Entity* entity)
    {
        if (!entity) {
            return;
        }
        gMeshEntities.erase(std::remove(gMeshEntities.begin(), gMeshEntities.end(), entity), gMeshEntities.end());
        gPendingMeshNames.erase(entity);
    }

    void drawMeshEntities()
    {
#if defined(WII) || defined(__wii__)
        static bool sSanityTriEnabled = false;
        static u32 sSubmittedTerrainIndices = 0;

        Vector3 focusPos = Vector3::ZERO;
        Vector3 firstVertexFocus = Vector3::ZERO;
        bool haveFirstVertexFocus = false;
        bool haveFocus = false;
        for (std::map<Entity*, String>::iterator it = gPendingMeshNames.begin(); it != gPendingMeshNames.end(); )
        {
            Entity* entity = it->first;
            if (!entity) {
                it = gPendingMeshNames.erase(it);
                continue;
            }
            MeshPtr mesh = MeshManager::getSingleton().getByName(it->second);
            if (mesh) {
                entity->_setMesh(mesh);
                it = gPendingMeshNames.erase(it);
            } else {
                ++it;
            }
        }

        bool firstSubmeshProbed = false;
        for (size_t entIdx = 0; entIdx < gMeshEntities.size(); ++entIdx)
        {
            Entity* entity = gMeshEntities[entIdx];
            if (!entity || !entity->isVisible()) {
                continue;
            }
            std::shared_ptr<Mesh> mesh = entity->getMesh();
            if (!mesh || !mesh->sharedVertexData || mesh->sharedVertexData->vertexCount < 3) {
                continue;
            }
            SceneNode* node = entity->getParentSceneNode();
            if (node) {
                focusPos = node->_getDerivedPosition();
                haveFocus = true;
                break;
            }
        }

        Mtx view;
        Mtx modelView;
        guVector camera = {focusPos.x, focusPos.y + 900.0f, focusPos.z + 900.0f};
        guVector up = {0.0f, 1.0f, 0.0f};
        guVector look = {focusPos.x, focusPos.y, focusPos.z};
        guLookAt(view, &camera, &up, &look);

        guMtxIdentity(modelView);
        guMtxTransApply(modelView, modelView, 0.0f, 0.0f, -50.0f);
        guMtxConcat(view, modelView, modelView);
        GX_LoadPosMtxImm(modelView, GX_PNMTX0);
        GX_SetCurrentMtx(GX_PNMTX0);

        if (sSanityTriEnabled)
        {
            static short sanityPos[9] ATTRIBUTE_ALIGN(32) = {
                -400, 20, 0,
                 400, 20, 0,
                   0, 20, -400
            };
            static unsigned char sanityCol[12] ATTRIBUTE_ALIGN(32) = {
                255, 32, 32, 255,
                32, 255, 32, 255,
                32, 32, 255, 255
            };

            GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
            DCFlushRange(sanityPos, sizeof(sanityPos));
            DCFlushRange(sanityCol, sizeof(sanityCol));
            GX_SetArray(GX_VA_POS, sanityPos, 3 * sizeof(short));
            GX_SetArray(GX_VA_CLR0, sanityCol, 4 * sizeof(unsigned char));
            GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
            GX_Position1x16(0); GX_Color1x16(0);
            GX_Position1x16(1); GX_Color1x16(1);
            GX_Position1x16(2); GX_Color1x16(2);
            GX_End();
            GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        }

        static u32 sLogFrameCounter = 0;
        static int sPrevFocusX = 0x7FFFFFFF;
        static int sPrevFocusY = 0x7FFFFFFF;
        static int sPrevFocusZ = 0x7FFFFFFF;
        static u32 sPrevEntities = 0xFFFFFFFFu;
        static u32 sPrevDrawn = 0xFFFFFFFFu;
        static u32 sPrevSkipped = 0xFFFFFFFFu;
        static u32 sPrevMissingMesh = 0xFFFFFFFFu;
        static u32 sPrevMissingVd = 0xFFFFFFFFu;
        static u32 sPrevMissingPos = 0xFFFFFFFFu;
        static u32 sPrevMissingVb = 0xFFFFFFFFu;

        u32 drawnSubmeshes = 0;
        u32 drawnSequentialMeshes = 0;
        u32 skippedSubmeshes = 0;
        u32 missingMesh = 0;
        u32 missingVd = 0;
        u32 missingPos = 0;
        u32 missingVb = 0;

        static const size_t kMaxBatch = 60000;
        static short gxPositions[kMaxBatch * 3] ATTRIBUTE_ALIGN(32);
        static unsigned char gxColors[kMaxBatch * 4] ATTRIBUTE_ALIGN(32);

        for (size_t entIdx = 0; entIdx < gMeshEntities.size(); ++entIdx)
        {
            Entity* entity = gMeshEntities[entIdx];
            if (!entity || !entity->isVisible()) {
                continue;
            }

            Vector3 worldPos = Vector3::ZERO;
            SceneNode* node = entity->getParentSceneNode();
            if (node) {
                worldPos = node->_getDerivedPosition();
            }

            std::shared_ptr<Mesh> mesh = entity->getMesh();
            if (!mesh) {
                ++missingMesh;
                continue;
            }

            VertexData* sharedVd = mesh->sharedVertexData;
            if (!sharedVd || !sharedVd->vertexDeclaration || !sharedVd->vertexBufferBinding) {
                ++missingVd;
                continue;
            }

            const VertexElement* posElem = sharedVd->vertexDeclaration->findElementBySemantic(VES_POSITION);
            if (!posElem) {
                ++missingPos;
                continue;
            }

            HardwareVertexBufferSharedPtr vb = sharedVd->vertexBufferBinding->getBuffer(posElem->getSource());
            if (!vb) {
                ++missingVb;
                continue;
            }

            const void* vbPtr = vb->getPointer();
            if (!firstSubmeshProbed)
            {
                firstSubmeshProbed = true;
                if (!vbPtr)
                {
                    WiiDebugLog("WiiGX: [ERROR] Mesh %s has NULL Vertex Buffer\n", entity->getName().c_str());
                    SYS_Report("WiiGX: [ERROR] Mesh %s has NULL Vertex Buffer\n", entity->getName().c_str());
                }
                else
                {
                    const unsigned char* vertexBase0 = static_cast<const unsigned char*>(vbPtr);
                    const float* pos0 = 0;
                    posElem->baseVertexPointerToElement(vertexBase0, &pos0);
                    if (pos0)
                    {
                        firstVertexFocus = Vector3(pos0[0] + worldPos.x, pos0[1] + worldPos.y, pos0[2] + worldPos.z);
                        haveFirstVertexFocus = true;
                        WiiDebugLog("WiiGX: [DATA] Vertex0: %f, %f, %f\n", pos0[0], pos0[1], pos0[2]);
                        SYS_Report("WiiGX: [DATA] Vertex0: %f, %f, %f\n", pos0[0], pos0[1], pos0[2]);
                    }
                    else
                    {
                        WiiDebugLog("WiiGX: [ERROR] Mesh %s has NULL Vertex Pointer\n", entity->getName().c_str());
                        SYS_Report("WiiGX: [ERROR] Mesh %s has NULL Vertex Pointer\n", entity->getName().c_str());
                    }
                }
            }

            const void* vbRaw = vb->lock(HardwareBuffer::HBL_READ_ONLY);
            if (!vbRaw) {
                vb->unlock();
                continue;
            }

            const size_t vtxCount = sharedVd->vertexCount;

            bool drewSequential = false;
            if (vtxCount >= 3)
            {
                for (size_t batchStart = 0; batchStart < vtxCount; batchStart += kMaxBatch)
                {
                    size_t batchCount = vtxCount - batchStart;
                    if (batchCount > kMaxBatch) {
                        batchCount = kMaxBatch;
                    }
                    batchCount = (batchCount / 3) * 3;
                    if (batchCount < 3) {
                        continue;
                    }

                    for (size_t i = 0; i < batchCount; ++i)
                    {
                        const size_t idx = batchStart + i;
                        const unsigned char* vertexBase = static_cast<const unsigned char*>(vbRaw) + idx * vb->getVertexSize();
                        const float* pos = 0;
                        posElem->baseVertexPointerToElement(vertexBase, &pos);

                        const float px = (pos ? pos[0] : 0.0f) + worldPos.x - focusPos.x;
                        const float py = (pos ? pos[1] : 0.0f) + worldPos.y - focusPos.y;
                        const float pz = (pos ? pos[2] : 0.0f) + worldPos.z - focusPos.z;

                        gxPositions[i * 3 + 0] = static_cast<short>(px);
                        gxPositions[i * 3 + 1] = static_cast<short>(py);
                        gxPositions[i * 3 + 2] = static_cast<short>(pz);

                        gxColors[i * 4 + 0] = 196;
                        gxColors[i * 4 + 1] = 180;
                        gxColors[i * 4 + 2] = 96;
                        gxColors[i * 4 + 3] = 255;
                    }

                    DCFlushRange(gxPositions, batchCount * 3 * sizeof(short));
                    DCFlushRange(gxColors, batchCount * 4 * sizeof(unsigned char));
                    GX_SetArray(GX_VA_POS, gxPositions, 3 * sizeof(short));
                    GX_SetArray(GX_VA_CLR0, gxColors, 4 * sizeof(unsigned char));
                    GX_SetNumTevStages(1);
                    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
                    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
                    GX_SetNumChans(1);

                    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, batchCount);
                    for (size_t i = 0; i < batchCount; ++i)
                    {
                        GX_Position1x16(static_cast<u16>(i));
                        GX_Color1x16(static_cast<u16>(i));
                    }
                    GX_End();
                    sSubmittedTerrainIndices += static_cast<u32>(batchCount);
                }

                drewSequential = true;
                ++drawnSequentialMeshes;
            }

            if (drewSequential)
            {
                vb->unlock();
                continue;
            }

            const unsigned short subCount = mesh->getNumSubMeshes();
            for (unsigned short si = 0; si < subCount; ++si)
            {
                SubMesh* sub = mesh->getSubMesh(si);
                if (!sub || !sub->indexData || !sub->indexData->indexBuffer) {
                    continue;
                }

                const size_t indexCount = sub->indexData->indexCount;
                if (indexCount < 3) {
                    ++skippedSubmeshes;
                    continue;
                }

                const void* ibRaw = sub->indexData->indexBuffer->lock(HardwareBuffer::HBL_READ_ONLY);
                if (!ibRaw) {
                    continue;
                }

                const bool idx32 = (sub->indexData->indexBuffer->getType() == HardwareIndexBuffer::IT_32BIT);
                const size_t base = sub->indexData->indexStart;
                const size_t vtxCountIdx = sharedVd->vertexCount;

                for (size_t batchStart = 0; batchStart < indexCount; batchStart += kMaxBatch)
                {
                    size_t batchCount = indexCount - batchStart;
                    if (batchCount > kMaxBatch) {
                        batchCount = kMaxBatch;
                    }

                    for (size_t i = 0; i < batchCount; ++i)
                    {
                        const size_t srcI = base + batchStart + i;
                        size_t idx = 0;
                        if (idx32) {
                            const uint32* idxData = static_cast<const uint32*>(ibRaw);
                            idx = static_cast<size_t>(idxData[srcI]);
                        } else {
                            const uint16* idxData = static_cast<const uint16*>(ibRaw);
                            idx = static_cast<size_t>(idxData[srcI]);
                        }

                        if (idx >= vtxCountIdx) {
                            idx = 0;
                        }

                        const unsigned char* vertexBase = static_cast<const unsigned char*>(vbRaw) + idx * vb->getVertexSize();
                        const float* pos = 0;
                        posElem->baseVertexPointerToElement(vertexBase, &pos);

                        const float px = (pos ? pos[0] : 0.0f) + worldPos.x - focusPos.x;
                        const float py = (pos ? pos[1] : 0.0f) + worldPos.y - focusPos.y;
                        const float pz = (pos ? pos[2] : 0.0f) + worldPos.z - focusPos.z;

                        gxPositions[i * 3 + 0] = static_cast<short>(px);
                        gxPositions[i * 3 + 1] = static_cast<short>(py);
                        gxPositions[i * 3 + 2] = static_cast<short>(pz);

                        gxColors[i * 4 + 0] = 176;
                        gxColors[i * 4 + 1] = 196;
                        gxColors[i * 4 + 2] = 168;
                        gxColors[i * 4 + 3] = 255;
                    }

                    DCFlushRange(gxPositions, batchCount * 3 * sizeof(short));
                    DCFlushRange(gxColors, batchCount * 4 * sizeof(unsigned char));
                    GX_SetArray(GX_VA_POS, gxPositions, 3 * sizeof(short));
                    GX_SetArray(GX_VA_CLR0, gxColors, 4 * sizeof(unsigned char));
                    GX_SetNumTevStages(1);
                    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
                    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
                    GX_SetNumChans(1);

                    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, batchCount);
                    for (size_t i = 0; i < batchCount; ++i)
                    {
                        GX_Position1x16(static_cast<u16>(i));
                        GX_Color1x16(static_cast<u16>(i));
                    }
                    GX_End();
                    sSubmittedTerrainIndices += static_cast<u32>(batchCount);
                }

                sub->indexData->indexBuffer->unlock();
                ++drawnSubmeshes;
            }

            vb->unlock();
        }

        ++sLogFrameCounter;
        const bool changed =
            (sPrevEntities != static_cast<u32>(gMeshEntities.size())) ||
            (sPrevDrawn != drawnSubmeshes) ||
            (sPrevSkipped != skippedSubmeshes) ||
            (sPrevMissingMesh != missingMesh) ||
            (sPrevMissingVd != missingVd) ||
            (sPrevMissingPos != missingPos) ||
            (sPrevMissingVb != missingVb);
        const bool periodic = (sLogFrameCounter % 120u) == 0u;
        if (changed || periodic)
        {
            WiiDebugLog("[GXMESH] f=%u entities=%u drawn=%u skipped=%u noMesh=%u noVD=%u noPOS=%u noVB=%u\n",
                static_cast<unsigned int>(sLogFrameCounter),
                static_cast<unsigned int>(gMeshEntities.size()),
                static_cast<unsigned int>(drawnSubmeshes),
                static_cast<unsigned int>(skippedSubmeshes),
                static_cast<unsigned int>(missingMesh),
                static_cast<unsigned int>(missingVd),
                static_cast<unsigned int>(missingPos),
                static_cast<unsigned int>(missingVb));
            SYS_Report("WiiGX: [GXMESH] f=%u entities=%u drawn=%u skipped=%u noMesh=%u noVD=%u noPOS=%u noVB=%u\n",
                static_cast<unsigned int>(sLogFrameCounter),
                static_cast<unsigned int>(gMeshEntities.size()),
                static_cast<unsigned int>(drawnSubmeshes),
                static_cast<unsigned int>(skippedSubmeshes),
                static_cast<unsigned int>(missingMesh),
                static_cast<unsigned int>(missingVd),
                static_cast<unsigned int>(missingPos),
                static_cast<unsigned int>(missingVb));
            WiiDebugLog("[GXMESH] seqMeshes=%u\n", static_cast<unsigned int>(drawnSequentialMeshes));
            SYS_Report("WiiGX: [GXMESH] seqMeshes=%u\n", static_cast<unsigned int>(drawnSequentialMeshes));
            WiiDebugLog("WiiGX: Submitted %u indices for Terrain\n", static_cast<unsigned int>(sSubmittedTerrainIndices));
            SYS_Report("WiiGX: Submitted %u indices for Terrain\n", static_cast<unsigned int>(sSubmittedTerrainIndices));
            const int fx = static_cast<int>(focusPos.x);
            const int fy = static_cast<int>(focusPos.y);
            const int fz = static_cast<int>(focusPos.z);
            if (!haveFocus) {
                WiiDebugLog("[GXMESH] focus=none\n");
            } else if (fx != sPrevFocusX || fy != sPrevFocusY || fz != sPrevFocusZ) {
                WiiDebugLog("[GXMESH] focus=(%d,%d,%d)\n", fx, fy, fz);
                sPrevFocusX = fx;
                sPrevFocusY = fy;
                sPrevFocusZ = fz;
            }
            sPrevEntities = static_cast<u32>(gMeshEntities.size());
            sPrevDrawn = drawnSubmeshes;
            sPrevSkipped = skippedSubmeshes;
            sPrevMissingMesh = missingMesh;
            sPrevMissingVd = missingVd;
            sPrevMissingPos = missingPos;
            sPrevMissingVb = missingVb;
            sSubmittedTerrainIndices = 0;
        }
        if (!firstSubmeshProbed)
        {
            WiiDebugLog("WiiGX: [ERROR] No mesh submesh available to probe\n");
            SYS_Report("WiiGX: [ERROR] No mesh submesh available to probe\n");
        }

        if (haveFirstVertexFocus)
        {
            focusPos = firstVertexFocus;
            Mtx oneFrameView;
            guVector oneFrameCamera = {focusPos.x, focusPos.y + 120.0f, focusPos.z + 120.0f};
            guVector oneFrameLook = {focusPos.x, focusPos.y, focusPos.z};
            guLookAt(oneFrameView, &oneFrameCamera, &up, &oneFrameLook);
            GX_LoadPosMtxImm(oneFrameView, GX_PNMTX0);
        }
#endif
    }
}
