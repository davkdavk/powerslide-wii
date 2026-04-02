/*
 * Wii GX Renderer - Based on devkitPro triangle example
 */

#include "WiiGXRenderer.h"
#include "Ogre.h"
#include "../../orig_src/WiiDebugLog.h"
#include <gccore.h>
#include <ogc/gx.h>
#include <ogc/gu.h>
#include <ogc/system.h>
#include <ogc/console.h>
#include <ogc/pad.h>
#include <wiiuse/wpad.h>
#include <malloc.h>
#include <string.h>
#include <math.h>

#if defined(WII) || defined(__wii__)
#define OSReport SYS_Report
#ifndef WII_TERRAIN_DIAG_STAGE
#define WII_TERRAIN_DIAG_STAGE 4
#endif
static volatile int gWiiTerrainDiagStage = WII_TERRAIN_DIAG_STAGE;
static const bool kForceTransformIsolation = true;
static volatile bool gUseGameplayCameraMode = false;
static volatile float gCameraYawDeg = 0.0f;
static volatile bool gEnableRealTerrainTexture = false;
static volatile bool gTerrainBatchTextureDebug = false;
#endif

namespace Ogre
{
    extern bool gAssetsLoaded;
}

namespace WiiGX
{
    static GXRModeObj* sScreenMode = NULL;
    static void* sFrameBuffer = NULL;
    static vu8 sReadyForCopy = 0;
    static void copyBuffers(u32 count __attribute__ ((unused)))
    {
        if (sReadyForCopy == GX_TRUE && sFrameBuffer)
        {
            GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
            GX_SetColorUpdate(GX_TRUE);
            GX_CopyDisp(sFrameBuffer, GX_TRUE);
            GX_Flush();
            sReadyForCopy = GX_FALSE;
        }
    }
    Renderer& Renderer::getInstance()
    {
        static Renderer instance;
        return instance;
    }
    
    Renderer::Renderer()
        : mClearColor{0,0,0,0},
          mXfb(NULL),
          mProjection(), mViewMatrix(), mWorldMatrix(),
          mCurrentMaterial(),
          mVertexBuffer{},
          mVertexCount(0),
          mFPS(60.0f), mDeltaTime(1.0f/60.0f), mFrameCount(0), mLastFrameTime(0),
          mRotation(0.0f),
          mDepthTestEnabled(true), mDepthWriteEnabled(true), mBlendEnabled(false),
          mViewport{0,0,0,0},
          mTrackVertices(NULL),
          mTrackVertexCount(0),
          mTerrainVertices(NULL),
          mTerrainVertexCount(0),
          mTerrainProbeCreateMeshSeen(false),
          mTerrainProbeTriCount(0),
          mTerrainProbeUploadOk(false),
          mTerrainProbeUploadVerts(0),
          mTerrainProbeRenderCalled(false),
          mTerrainProbeDrawCount(0),
           mTerrainFocusX(0.0f),
           mTerrainFocusY(0.0f),
           mTerrainFocusZ(0.0f),
           mCarStartX(0.0f),
           mCarStartY(0.0f),
           mCarStartZ(0.0f),
           mCarForwardX(0.0f),
           mCarForwardY(0.0f),
           mCarForwardZ(-1.0f),
           mCameraOffsetX(0.0f),
           mCameraOffsetY(0.0f),
           mCameraOffsetZ(0.0f),
           mChunkR(1.0f),
           mChunkG(0.2f),
           mChunkB(0.2f),
           mTerrainTextureName(),
           mFramePresentEnabled(true)
    {
    }

    void Renderer::clearTerrainBuffer()
    {
        mTerrainVertexCount = 0;
        mTerrainProbeUploadOk = false;
        mTerrainProbeUploadVerts = 0;
        mTerrainFocusX = 0.0f;
        mTerrainFocusY = 0.0f;
        mTerrainFocusZ = 0.0f;
        mTerrainDirectVertices.clear();
        mTerrainDirectIndices.clear();
        mTerrainDirectUVByIndex.clear();
        mTerrainChunkBatches.clear();
        mTerrainTextureName.clear();
        mTerrainDirectDataValid = false;
    }
    
    Renderer::~Renderer()
    {
        shutdown();
    }
    
    void Renderer::init()
    {
        VIDEO_Init();
        WPAD_Init();

        sScreenMode = VIDEO_GetPreferredMode(NULL);
        GXRModeObj* rmode = sScreenMode;

        sFrameBuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
        mXfb = sFrameBuffer;

        VIDEO_Configure(rmode);
        VIDEO_SetNextFramebuffer(sFrameBuffer);
        VIDEO_SetPostRetraceCallback(copyBuffers);
        VIDEO_SetBlack(false);
        VIDEO_Flush();
        VIDEO_WaitVSync();
        
        void* gxfifo = MEM_K0_TO_K1(memalign(32, 1024*256));
        memset(gxfifo, 0, 1024*256);
        GX_Init(gxfifo, 1024*256);

        GXColor startupProbeRed = {255, 0, 0, 255};
        GX_SetCopyClear(startupProbeRed, 0x00FFFFFF);
        GX_CopyDisp(sFrameBuffer, GX_TRUE);
        GX_Flush();
        VIDEO_WaitVSync();
        VIDEO_WaitVSync();
         
        mViewport[0] = 0;
        mViewport[1] = 0;
        mViewport[2] = rmode->fbWidth;
        mViewport[3] = rmode->efbHeight;
        
        setupGX(rmode);
        
        mClearColor[0] = 32;
        mClearColor[1] = 32;
        mClearColor[2] = 64;
        
        mRotation = 0.0f;
        
        {
            u64 ticks;
            __asm__ volatile("mftbl %0" : "=r"(ticks));
            mLastFrameTime = ticks;
        }
        mFPS = 60.0f;
    }
    
    void Renderer::shutdown()
    {
        if(mTrackVertices)
        {
            free(mTrackVertices);
            mTrackVertices = NULL;
            mTrackVertexCount = 0;
        }
        if(mTerrainVertices)
        {
            free(mTerrainVertices);
            mTerrainVertices = NULL;
            mTerrainVertexCount = 0;
        }
        printf("WiiGX: Shutting down...\n");
    }
    
    void Renderer::setupGX(GXRModeObj* rmode)
    {
        GXColor bg = {0, 0, 0, 255};
        GX_SetCopyClear(bg, 0x00FFFFFF);

        GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1.0f);
        GX_SetDispCopyYScale((f32)rmode->xfbHeight / (f32)rmode->efbHeight);
        GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
        GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
        GX_SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);
        
        GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
        GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
        
        GX_SetCullMode(GX_CULL_NONE);
        GX_CopyDisp(sFrameBuffer, GX_TRUE);
        GX_SetDispCopyGamma(GX_GM_1_0);

        // Vertex format - match devkitPro triangle sample
        GX_ClearVtxDesc();
        GX_SetVtxDesc(GX_VA_POS, GX_INDEX16);
        GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX16);

        GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
        GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

        GX_SetNumChans(1);
        GX_SetNumTexGens(0);
        GX_SetNumTevStages(1);
        
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
        
        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        
        // Projection
        Mtx44 projection;
        const float aspect = (CONF_GetAspectRatio() == CONF_ASPECT_16_9) ? (16.0f / 9.0f) : (4.0f / 3.0f);
        guPerspective(projection, 60, aspect, 1.0f, 20000.0f);
        GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);
    }
    
    void Renderer::beginFrame()
    {
        if (!sScreenMode) {
            return;
        }
        GX_SetDrawDone();
        GXColor bg = {mClearColor[0], mClearColor[1], mClearColor[2], 255};
        GX_SetCopyClear(bg, 0x00FFFFFF);
        GX_SetViewport(0, 0, sScreenMode->fbWidth, sScreenMode->efbHeight, 0, 1.0f);
        GX_InvVtxCache();
        GX_InvalidateTexAll();
    }
    
    void Renderer::endFrame()
    {
        if (!sScreenMode) {
            return;
        }
        if(!mFramePresentEnabled)
        {
            return;
        }
        GX_DrawDone();
        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GX_SetColorUpdate(GX_TRUE);
        GX_CopyDisp(sFrameBuffer, GX_TRUE);
        GX_Flush();
        sReadyForCopy = GX_FALSE;
        VIDEO_SetNextFramebuffer(sFrameBuffer);
        VIDEO_Flush();
        VIDEO_WaitVSync();
    }

    void Renderer::setFramePresentEnabled(bool enable)
    {
        mFramePresentEnabled = enable;
    }
    
    void Renderer::setClearColor(u8 r, u8 g, u8 b, u8 a)
    {
        mClearColor[0] = r;
        mClearColor[1] = g;
        mClearColor[2] = b;
        mClearColor[3] = a;
    }

    void Renderer::presentProbeColor(u8 r, u8 g, u8 b, u8 a)
    {
#if defined(WII_NATIVE_ASSET_PIPELINE)
        (void)r;
        (void)g;
        (void)b;
        (void)a;
        return;
#endif

        if (!sScreenMode)
        {
            return;
        }

        setClearColor(r, g, b, a);
        beginFrame();
        endFrame();
    }
    
    void Renderer::clearBuffers()
    {
    }
    
    void Renderer::setProjection(const Matrix4x4& proj)
    {
    }
    
    void Renderer::setView(const Matrix4x4& view)
    {
        mViewMatrix = view;
    }
    
    void Renderer::setWorld(const Matrix4x4& world)
    {
        mWorldMatrix = world;
    }
    
    void Renderer::setMaterial(const Material& mat)
    {
    }
    
    void Renderer::drawPrimitives(int type, const Vertex* vertices, u32 count)
    {
        if (!vertices || count == 0) return;

        if (count > WIIGX_MAX_VERTICES) count = WIIGX_MAX_VERTICES;

        static s16 gxVertices[WIIGX_MAX_VERTICES * 3] ATTRIBUTE_ALIGN(32);
        static u8 gxColors[WIIGX_MAX_VERTICES * 4] ATTRIBUTE_ALIGN(32);

        for (u32 i = 0; i < count; ++i)
        {
            gxVertices[i * 3 + 0] = static_cast<s16>(vertices[i].position.x);
            gxVertices[i * 3 + 1] = static_cast<s16>(vertices[i].position.y);
            gxVertices[i * 3 + 2] = static_cast<s16>(vertices[i].position.z);

            gxColors[i * 4 + 0] = vertices[i].color.r;
            gxColors[i * 4 + 1] = vertices[i].color.g;
            gxColors[i * 4 + 2] = vertices[i].color.b;
            gxColors[i * 4 + 3] = vertices[i].color.a;
        }

        GX_SetArray(GX_VA_POS, gxVertices, 3 * sizeof(s16));
        GX_SetArray(GX_VA_CLR0, gxColors, 4 * sizeof(u8));

        Mtx view, modelView;
        guVector camera = {0.0f, 0.0f, 0.0f};
        guVector up = {0.0f, 1.0f, 0.0f};
        guVector look = {0.0f, 0.0f, -1.0f};
        guLookAt(view, &camera, &up, &look);

        guMtxIdentity(modelView);
        guMtxTransApply(modelView, modelView, 0.0f, 0.0f, -50.0f);
        guMtxConcat(view, modelView, modelView);
        GX_LoadPosMtxImm(modelView, GX_PNMTX0);

        GX_Begin(type, GX_VTXFMT0, count);
        for (u32 i = 0; i < count; ++i) {
            GX_Position1x16(static_cast<u16>(i));
            GX_Color1x16(static_cast<u16>(i));
        }
        GX_End();
    }

    void Renderer::drawIndexedPrimitives(int type, const Vertex* vertices, u32 vcount, const u16* indices, u32 icount)
    {
        if (!vertices || !indices || vcount == 0 || icount == 0) return;

        if (vcount > WIIGX_MAX_VERTICES) vcount = WIIGX_MAX_VERTICES;
        if (icount > WIIGX_MAX_VERTICES * 3) icount = WIIGX_MAX_VERTICES * 3;
        
        static s16 gxVertices[WIIGX_MAX_VERTICES * 3] ATTRIBUTE_ALIGN(32);
        static u8 gxColors[WIIGX_MAX_VERTICES * 4] ATTRIBUTE_ALIGN(32);

        for (u32 i = 0; i < vcount; ++i)
        {
            gxVertices[i * 3 + 0] = static_cast<s16>(vertices[i].position.x);
            gxVertices[i * 3 + 1] = static_cast<s16>(vertices[i].position.y);
            gxVertices[i * 3 + 2] = static_cast<s16>(vertices[i].position.z);

            gxColors[i * 4 + 0] = vertices[i].color.r;
            gxColors[i * 4 + 1] = vertices[i].color.g;
            gxColors[i * 4 + 2] = vertices[i].color.b;
            gxColors[i * 4 + 3] = vertices[i].color.a;
        }

        GX_SetArray(GX_VA_POS, gxVertices, 3 * sizeof(s16));
        GX_SetArray(GX_VA_CLR0, gxColors, 4 * sizeof(u8));

        Mtx view, modelView;

        guVector camera = {0.0f, 0.0f, 0.0f};
        guVector up = {0.0f, 1.0f, 0.0f};
        guVector look = {0.0f, 0.0f, -1.0f};
        guLookAt(view, &camera, &up, &look);

        guMtxIdentity(modelView);
        guMtxTransApply(modelView, modelView, 0.0f, 0.0f, -50.0f);
        guMtxConcat(view, modelView, modelView);
        GX_LoadPosMtxImm(modelView, GX_PNMTX0);
        
        GX_Begin(type, GX_VTXFMT0, icount);
        
        for (u32 i = 0; i < icount; i++) {
            u16 idx = indices[i];
            if (idx >= vcount) {
                idx = 0;
            }
            GX_Position1x16(static_cast<u16>(idx));
            GX_Color1x16(static_cast<u16>(idx));
        }
        
        GX_End();
    }
    
    void Renderer::uploadVertices(const Vertex* vertices, u32 count)
    {
    }
    
    void Renderer::enableDepthTest(bool enable)
    {
        mDepthTestEnabled = enable;
    }
    
    void Renderer::enableDepthWrite(bool enable)
    {
        mDepthWriteEnabled = enable;
    }
    
    void Renderer::enableBlend(bool enable)
    {
        mBlendEnabled = enable;
    }

    bool Renderer::uploadTrackBufferBE(const void* beXYZS16, u32 vertexCount)
    {
        if(!beXYZS16 || vertexCount == 0)
            return false;

        if(vertexCount > WIIGX_MAX_VERTICES)
            vertexCount = WIIGX_MAX_VERTICES;

        if(!mTrackVertices)
        {
            mTrackVertices = static_cast<s16*>(memalign(32, WIIGX_MAX_VERTICES * 3 * sizeof(s16)));
            if(!mTrackVertices)
                return false;
        }

        const unsigned char* src = static_cast<const unsigned char*>(beXYZS16);
        for(u32 i = 0; i < vertexCount; ++i)
        {
            const u32 o = i * 6;
            mTrackVertices[i * 3 + 0] = static_cast<s16>((src[o + 0] << 8) | src[o + 1]);
            mTrackVertices[i * 3 + 1] = static_cast<s16>((src[o + 2] << 8) | src[o + 3]);
            mTrackVertices[i * 3 + 2] = static_cast<s16>((src[o + 4] << 8) | src[o + 5]);
        }

        mTrackVertexCount = vertexCount;
        DCFlushRange(mTrackVertices, mTrackVertexCount * 3 * sizeof(s16));
        return true;
    }

    void Renderer::renderTrackBuffer()
    {
        if(!mTrackVertices || mTrackVertexCount == 0)
            return;

        static u8 cyan[4] ATTRIBUTE_ALIGN(32) = {0, 255, 255, 255};
        static u8 roadColors[WIIGX_MAX_VERTICES * 2 * 4] ATTRIBUTE_ALIGN(32);
        static s16 roadVerts[WIIGX_MAX_VERTICES * 2 * 3] ATTRIBUTE_ALIGN(32);

        s16 minX = mTrackVertices[0];
        s16 maxX = mTrackVertices[0];
        s16 minY = mTrackVertices[1];
        s16 maxY = mTrackVertices[1];
        s16 minZ = mTrackVertices[2];
        s16 maxZ = mTrackVertices[2];
        for(u32 i = 1; i < mTrackVertexCount; ++i)
        {
            const s16 x = mTrackVertices[i * 3 + 0];
            const s16 y = mTrackVertices[i * 3 + 1];
            const s16 z = mTrackVertices[i * 3 + 2];
            if(x < minX) minX = x;
            if(x > maxX) maxX = x;
            if(y < minY) minY = y;
            if(y > maxY) maxY = y;
            if(z < minZ) minZ = z;
            if(z > maxZ) maxZ = z;
        }

        const f32 cx = 0.5f * (static_cast<f32>(minX) + static_cast<f32>(maxX));
        const f32 cy = 0.5f * (static_cast<f32>(minY) + static_cast<f32>(maxY));
        const f32 cz = 0.5f * (static_cast<f32>(minZ) + static_cast<f32>(maxZ));
        const f32 dx = static_cast<f32>(maxX - minX);
        const f32 dz = static_cast<f32>(maxZ - minZ);
        f32 span = (dx > dz) ? dx : dz;
        if(span < 200.0f)
            span = 200.0f;

        f32 roadHalfWidth = span * 0.03f;
        if(roadHalfWidth < 18.0f)
            roadHalfWidth = 18.0f;
        if(roadHalfWidth > 80.0f)
            roadHalfWidth = 80.0f;

        Mtx view;
        guVector camera = {cx, cy + span * 1.5f, cz + span * 1.2f};
        guVector up = {0.0f, 1.0f, 0.0f};
        guVector look = {cx, cy, cz};
        guLookAt(view, &camera, &up, &look);
        GX_LoadPosMtxImm(view, GX_PNMTX0);

        const u32 n = mTrackVertexCount;
        const u32 roadCount = (n * 2u <= WIIGX_MAX_VERTICES * 2u) ? (n * 2u) : (WIIGX_MAX_VERTICES * 2u);
        const u32 usableN = roadCount / 2u;

        for(u32 i = 0; i < usableN; ++i)
        {
            const u32 ip = (i == 0) ? (usableN - 1) : (i - 1);
            const u32 in = (i + 1u) % usableN;

            const f32 px = static_cast<f32>(mTrackVertices[ip * 3 + 0]);
            const f32 pz = static_cast<f32>(mTrackVertices[ip * 3 + 2]);
            const f32 cx0 = static_cast<f32>(mTrackVertices[i * 3 + 0]);
            const f32 cy0 = static_cast<f32>(mTrackVertices[i * 3 + 1]);
            const f32 cz0 = static_cast<f32>(mTrackVertices[i * 3 + 2]);
            const f32 nx = static_cast<f32>(mTrackVertices[in * 3 + 0]);
            const f32 nz = static_cast<f32>(mTrackVertices[in * 3 + 2]);

            f32 tx = nx - px;
            f32 tz = nz - pz;
            const f32 tlen = sqrtf(tx * tx + tz * tz);
            if(tlen > 0.001f)
            {
                tx /= tlen;
                tz /= tlen;
            }
            else
            {
                tx = 1.0f;
                tz = 0.0f;
            }

            const f32 rx = -tz;
            const f32 rz = tx;

            const f32 lx = cx0 + rx * roadHalfWidth;
            const f32 lz = cz0 + rz * roadHalfWidth;
            const f32 rxp = cx0 - rx * roadHalfWidth;
            const f32 rzp = cz0 - rz * roadHalfWidth;
            const s16 y = static_cast<s16>(cy0 + 2.0f);

            const u32 li = i * 2u;
            const u32 ri = li + 1u;

            roadVerts[li * 3 + 0] = static_cast<s16>(lx);
            roadVerts[li * 3 + 1] = y;
            roadVerts[li * 3 + 2] = static_cast<s16>(lz);

            roadVerts[ri * 3 + 0] = static_cast<s16>(rxp);
            roadVerts[ri * 3 + 1] = y;
            roadVerts[ri * 3 + 2] = static_cast<s16>(rzp);

            roadColors[li * 4 + 0] = 180;
            roadColors[li * 4 + 1] = 230;
            roadColors[li * 4 + 2] = 255;
            roadColors[li * 4 + 3] = 255;

            roadColors[ri * 4 + 0] = 180;
            roadColors[ri * 4 + 1] = 230;
            roadColors[ri * 4 + 2] = 255;
            roadColors[ri * 4 + 3] = 255;
        }

        DCFlushRange(roadVerts, roadCount * 3u * sizeof(s16));
        DCFlushRange(roadColors, roadCount * 4u * sizeof(u8));

        GX_SetArray(GX_VA_POS, roadVerts, 3 * sizeof(s16));
        GX_SetArray(GX_VA_CLR0, roadColors, 4 * sizeof(u8));
        GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, roadCount);
        for(u32 i = 0; i < roadCount; ++i)
        {
            GX_Position1x16(static_cast<u16>(i));
            GX_Color1x16(static_cast<u16>(i));
        }
        GX_End();

        GX_SetArray(GX_VA_POS, mTrackVertices, 3 * sizeof(s16));
        GX_SetArray(GX_VA_CLR0, cyan, 0);
        const u32 drawCount = (usableN > 1) ? (usableN + 1u) : usableN;
        GX_Begin(GX_LINESTRIP, GX_VTXFMT0, drawCount);
        for(u32 i = 0; i < usableN; ++i)
        {
            GX_Position1x16(static_cast<u16>(i));
            GX_Color1x16(0);
        }
        if(usableN > 1)
        {
            GX_Position1x16(0);
            GX_Color1x16(0);
        }
        GX_End();
    }

    bool Renderer::uploadTerrainBufferF32(const float* xyzF32, u32 vertexCount)
    {
        OSReport("[TERRAIN] upload vertCount=%d\n", static_cast<int>(vertexCount));
        mTerrainProbeUploadOk = false;
        mTerrainProbeUploadVerts = vertexCount;
        if(!xyzF32 || vertexCount < 3)
            return false;

        if(mTerrainVertexCount >= WIIGX_MAX_TERRAIN_VERTICES)
            return false;

        const u32 remaining = WIIGX_MAX_TERRAIN_VERTICES - mTerrainVertexCount;
        if(vertexCount > remaining)
            vertexCount = remaining;

        if(!mTerrainVertices)
        {
            mTerrainVertices = static_cast<s16*>(memalign(32, WIIGX_MAX_TERRAIN_VERTICES * 3 * sizeof(s16)));
            if(!mTerrainVertices)
                return false;
        }

        const u32 baseVertex = mTerrainVertexCount;
        for(u32 i = 0; i < vertexCount; ++i)
        {
            const u32 dst = (baseVertex + i) * 3;
            mTerrainVertices[dst + 0] = static_cast<s16>(xyzF32[i * 3 + 0]);
            mTerrainVertices[dst + 1] = static_cast<s16>(xyzF32[i * 3 + 1]);
            mTerrainVertices[dst + 2] = static_cast<s16>(xyzF32[i * 3 + 2]);
        }

        mTerrainVertexCount += vertexCount;
        DCFlushRange(mTerrainVertices, mTerrainVertexCount * 3 * sizeof(s16));
        mTerrainProbeUploadOk = true;
        mTerrainProbeUploadVerts = mTerrainVertexCount;
        return true;
    }

    void Renderer::setTerrainBuildProbe(u32 triCount)
    {
        mTerrainProbeCreateMeshSeen = true;
        mTerrainProbeTriCount = triCount;
    }

    void Renderer::setTerrainFocusF32(float x, float y, float z)
    {
        if(mTerrainVertexCount == 0)
        {
            mTerrainFocusX = x;
            mTerrainFocusY = y;
            mTerrainFocusZ = z;
        }
    }

    void Renderer::setCarStartPositionF32(float x, float y, float z)
    {
        mCarStartX = x;
        mCarStartY = y;
        mCarStartZ = z;
    }

    void Renderer::setCarForwardDirF32(float x, float y, float z)
    {
        mCarForwardX = x;
        mCarForwardY = y;
        mCarForwardZ = z;
    }

    void Renderer::setTerrainTextureName(const std::string& textureName)
    {
        if(textureName.empty())
            return;

        mTerrainTextureName = textureName;
    }

    void Renderer::setTerrainChunkColorF32(float r, float g, float b)
    {
        mChunkR = r;
        mChunkG = g;
        mChunkB = b;
    }

    bool Renderer::uploadTerrainIndexedData(const float* vertexPositions, u32 vertexCount, const u32* indices, u32 indexCount)
    {
        if(!vertexPositions || !indices || vertexCount < 3 || indexCount < 3)
            return false;

        const u32 baseVert = static_cast<u32>(mTerrainDirectVertices.size()) / 3;
        const u32 baseIdx = static_cast<u32>(mTerrainDirectIndices.size());
        const u32 uvStart = static_cast<u32>(mTerrainDirectUVByIndex.size());
        const u32 floatCount = vertexCount * 3;
        mTerrainDirectVertices.insert(mTerrainDirectVertices.end(), vertexPositions, vertexPositions + floatCount);

        for(u32 i = 0; i < indexCount; ++i)
            mTerrainDirectIndices.push_back(indices[i] + baseVert);

        TerrainChunkBatch batch;
        batch.indexStart = baseIdx;
        batch.indexCount = indexCount;
        batch.uvStart = uvStart;
        batch.textureName = mTerrainTextureName;
        mTerrainChunkBatches.push_back(batch);

        mTerrainDirectDataValid = true;

        float minX = 1e30f, minY = 1e30f, minZ = 1e30f;
        float maxX = -1e30f, maxY = -1e30f, maxZ = -1e30f;
        for(u32 i = 0; i < vertexCount; ++i)
        {
            const float px = vertexPositions[i * 3 + 0];
            const float py = vertexPositions[i * 3 + 1];
            const float pz = vertexPositions[i * 3 + 2];
            if(px < minX) minX = px; if(px > maxX) maxX = px;
            if(py < minY) minY = py; if(py > maxY) maxY = py;
            if(pz < minZ) minZ = pz; if(pz > maxZ) maxZ = pz;
        }
        WiiDebugLog("[GXDIR] indexed terrain chunk bbox (%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f) verts=%u idx=%u total=%u\n",
            minX, minY, minZ, maxX, maxY, maxZ, static_cast<unsigned int>(vertexCount), static_cast<unsigned int>(indexCount),
            static_cast<unsigned int>(mTerrainDirectVertices.size() / 3));

        const float cx = (minX + maxX) * 0.5f;
        const float cy = (minY + maxY) * 0.5f;
        const float cz = (minZ + maxZ) * 0.5f;
        if(mTerrainDirectVertices.size() / 3 == vertexCount)
            setTerrainFocusF32(cx, cy, cz);
        return true;
    }

    bool Renderer::uploadTerrainIndexedDataWithUV(const float* vertexPositions, u32 vertexCount, const u32* indices, u32 indexCount, const float* uvByIndexF32)
    {
        if(!uploadTerrainIndexedData(vertexPositions, vertexCount, indices, indexCount))
            return false;

        if(!uvByIndexF32)
            return false;

        const u32 floatCount = indexCount * 2u;
        const u32 uvStart = static_cast<u32>(mTerrainDirectUVByIndex.size());
        mTerrainDirectUVByIndex.insert(mTerrainDirectUVByIndex.end(), uvByIndexF32, uvByIndexF32 + floatCount);
        if(!mTerrainChunkBatches.empty())
            mTerrainChunkBatches.back().uvStart = uvStart;
        return true;
    }

    void Renderer::updateCameraFromInput()
    {
        static u32 sPrevHeld = 0;
        static u32 sPrevGCHeld = 0;

        WPAD_ScanPads();
        PAD_ScanPads();
        const u32 held = WPAD_ButtonsHeld(0);
        const u32 down = WPAD_ButtonsDown(0);
        const u32 gcHeld = PAD_ButtonsHeld(0);
        const u32 gcDown = PAD_ButtonsDown(0);
        const u32 rising = held & ~sPrevHeld;
        const u32 gcRising = gcHeld & ~sPrevGCHeld;

        const float speed = 100.0f;
        const float yawStep = 1.5f;

        if(held & WPAD_BUTTON_UP)    mCameraOffsetZ -= speed;
        if(held & WPAD_BUTTON_DOWN)  mCameraOffsetZ += speed;
        if(gcHeld & PAD_BUTTON_UP)   mCameraOffsetZ -= speed;
        if(gcHeld & PAD_BUTTON_DOWN) mCameraOffsetZ += speed;

        if(held & WPAD_BUTTON_A)     mCameraOffsetY += speed;
        if(held & WPAD_BUTTON_B)     mCameraOffsetY -= speed;
        if(gcHeld & PAD_BUTTON_A)    mCameraOffsetY += speed;
        if(gcHeld & PAD_BUTTON_B)    mCameraOffsetY -= speed;

        if(gUseGameplayCameraMode)
        {
            if(held & WPAD_BUTTON_LEFT)   gCameraYawDeg -= yawStep;
            if(held & WPAD_BUTTON_RIGHT)  gCameraYawDeg += yawStep;
            if(gcHeld & PAD_BUTTON_LEFT)  gCameraYawDeg -= yawStep;
            if(gcHeld & PAD_BUTTON_RIGHT) gCameraYawDeg += yawStep;
        }
        else
        {
            if(held & WPAD_BUTTON_LEFT)   mCameraOffsetX -= speed;
            if(held & WPAD_BUTTON_RIGHT)  mCameraOffsetX += speed;
            if(gcHeld & PAD_BUTTON_LEFT)  mCameraOffsetX -= speed;
            if(gcHeld & PAD_BUTTON_RIGHT) mCameraOffsetX += speed;
        }

        if((down & WPAD_BUTTON_PLUS) || (gcDown & PAD_TRIGGER_L))
        {
            gUseGameplayCameraMode = true;
            WiiDebugLog("[CAM_MODE] gameplay\n");
        }
        if((down & WPAD_BUTTON_MINUS) || (gcDown & PAD_TRIGGER_R))
        {
            gUseGameplayCameraMode = false;
            WiiDebugLog("[CAM_MODE] offset\n");
        }

        if((down & WPAD_BUTTON_1) || (down & WPAD_BUTTON_2) || (rising & WPAD_BUTTON_1) || (rising & WPAD_BUTTON_2) ||
           (gcDown & PAD_BUTTON_X) || (gcDown & PAD_BUTTON_Y) || (gcRising & PAD_BUTTON_X) || (gcRising & PAD_BUTTON_Y))
        {
            gEnableRealTerrainTexture = !gEnableRealTerrainTexture;
            WiiDebugLog("[TEX_MODE] real terrain texture %s\n", gEnableRealTerrainTexture ? "on" : "off");
        }

        sPrevHeld = held;
        sPrevGCHeld = gcHeld;
    }

    void Renderer::renderTerrainBuffer()
    {
        OSReport("[TERRAIN] render vertCount=%d\n", static_cast<int>(mTerrainVertexCount));
        if(gWiiTerrainDiagStage >= 4)
            updateCameraFromInput();

        if(gWiiTerrainDiagStage == 1)
        {
            Mtx ident;
            Mtx44 ortho;
            guMtxIdentity(ident);
            guOrtho(ortho, 0.0f, 480.0f, 0.0f, 640.0f, 0.0f, 1.0f);
            GX_LoadProjectionMtx(ortho, GX_ORTHOGRAPHIC);
            GX_LoadPosMtxImm(ident, GX_PNMTX0);
            GX_SetCurrentMtx(GX_PNMTX0);

            GX_ClearVtxDesc();
            GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
            GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
            GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
            GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
            GX_SetNumChans(1);
            GX_SetNumTexGens(0);
            GX_SetNumTevStages(1);
            GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
            GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
            GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
            GX_SetCullMode(GX_CULL_NONE);

            GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
            GX_Position3s16(0, 0, 0);     GX_Color4u8(255, 255, 0, 255);
            GX_Position3s16(640, 0, 0);   GX_Color4u8(255, 255, 0, 255);
            GX_Position3s16(640, 480, 0); GX_Color4u8(255, 255, 0, 255);
            GX_Position3s16(0, 480, 0);   GX_Color4u8(255, 255, 0, 255);
            GX_End();

            static int sStage1Log = 0;
            if(sStage1Log < 3)
            {
                WiiDebugLog("[VISDBG] stage1 fullscreen quad drawn\n");
                sStage1Log++;
            }
            return;
        }

        mTerrainProbeRenderCalled = true;

        static int sRenderLogCounter = 0;
        if(sRenderLogCounter < 3)
        {
            WiiDebugLog("[TERRAIN_RENDER] entry dataValid=%d verts=%u indices=%u\n",
                mTerrainDirectDataValid ? 1 : 0,
                static_cast<unsigned int>(mTerrainDirectVertices.size() / 3),
                static_cast<unsigned int>(mTerrainDirectIndices.size()));
            sRenderLogCounter++;
        }

        if(!mTerrainDirectDataValid || mTerrainDirectVertices.empty() || mTerrainDirectIndices.empty())
        {
            static int sEarlyLog = 0;
            if(sEarlyLog < 3)
            {
                WiiDebugLog("[TERRAIN_RENDER] EARLY RETURN: dataValid=%d verts=%u indices=%u\n",
                    mTerrainDirectDataValid ? 1 : 0,
                    static_cast<unsigned int>(mTerrainDirectVertices.size()),
                    static_cast<unsigned int>(mTerrainDirectIndices.size()));
                sEarlyLog++;
            }
            return;
        }

        const u32 indexCount = static_cast<u32>(mTerrainDirectIndices.size());
        const u32 vertexCount = static_cast<u32>(mTerrainDirectVertices.size() / 3);
        mTerrainProbeDrawCount = (indexCount / 3) * 3;

        u32 maxIndex = 0;
        for(u32 i = 0; i < indexCount; ++i)
        {
            if(mTerrainDirectIndices[i] > maxIndex)
                maxIndex = mTerrainDirectIndices[i];
        }
        u32 nonFiniteVerts = 0;
        for(u32 v = 0; v < vertexCount; ++v)
        {
            const float x = mTerrainDirectVertices[v * 3 + 0];
            const float y = mTerrainDirectVertices[v * 3 + 1];
            const float z = mTerrainDirectVertices[v * 3 + 2];
            if(!((x == x) && (y == y) && (z == z)))
                ++nonFiniteVerts;
        }
        static int sContractRenderLogs = 0;
        if(sContractRenderLogs < 16)
        {
            WiiDebugLog("[CONTRACT][RENDER] stage=%d verts=%u idx=%u maxIdx=%u nonFinite=%u\n",
                gWiiTerrainDiagStage,
                static_cast<unsigned int>(vertexCount),
                static_cast<unsigned int>(indexCount),
                static_cast<unsigned int>(maxIndex),
                static_cast<unsigned int>(nonFiniteVerts));
            sContractRenderLogs++;
        }
        if(maxIndex >= vertexCount)
        {
            WiiDebugLog("[CONTRACT][RENDER][FAIL] maxIdx=%u verts=%u\n",
                static_cast<unsigned int>(maxIndex),
                static_cast<unsigned int>(vertexCount));
        }

        const f32 useX = mCarStartX != 0.0f ? mCarStartX : mTerrainFocusX;
        const f32 useY = mCarStartY != 0.0f ? mCarStartY : mTerrainFocusY;
        const f32 useZ = mCarStartZ != 0.0f ? mCarStartZ : mTerrainFocusZ;

        f32 fwdX = mCarForwardX;
        f32 fwdY = mCarForwardY;
        f32 fwdZ = mCarForwardZ;
        const f32 fwdLenSq = fwdX * fwdX + fwdY * fwdY + fwdZ * fwdZ;
        if(fwdLenSq > 0.0001f)
        {
            const f32 invLen = 1.0f / sqrtf(fwdLenSq);
            fwdX *= invLen;
            fwdY *= invLen;
            fwdZ *= invLen;
        }
        else
        {
            fwdX = 0.0f;
            fwdY = 0.0f;
            fwdZ = -1.0f;
        }

        const f32 cameraBackDist = 160.0f;
        const f32 cameraHeight = 55.0f;
        const f32 lookAheadDist = 240.0f;

        static int sCamLogCounter = 0;
        if(sCamLogCounter < 3)
        {
            WiiDebugLog("[TERRAIN_RENDER] carStart=(%.1f,%.1f,%.1f) terrainFocus=(%.1f,%.1f,%.1f)\n",
                mCarStartX, mCarStartY, mCarStartZ,
                mTerrainFocusX, mTerrainFocusY, mTerrainFocusZ);
            sCamLogCounter++;
        }

        Mtx view;
        Mtx modelView;
        float minX = 1.0e30f, minY = 1.0e30f, minZ = 1.0e30f;
        float maxX = -1.0e30f, maxY = -1.0e30f, maxZ = -1.0e30f;
        for(u32 v = 0; v < vertexCount; ++v)
        {
            const float x = mTerrainDirectVertices[v * 3 + 0];
            const float y = mTerrainDirectVertices[v * 3 + 1];
            const float z = mTerrainDirectVertices[v * 3 + 2];
            if(x < minX) minX = x;
            if(x > maxX) maxX = x;
            if(y < minY) minY = y;
            if(y > maxY) maxY = y;
            if(z < minZ) minZ = z;
            if(z > maxZ) maxZ = z;
        }

        const f32 centerX = (minX + maxX) * 0.5f;
        const f32 centerY = (minY + maxY) * 0.5f;
        const f32 centerZ = (minZ + maxZ) * 0.5f;

        const bool fixedCam = kForceTransformIsolation ? true : (gWiiTerrainDiagStage <= 3);
        const f32 focusX = fixedCam ? centerX : (useX + fwdX * lookAheadDist + mCameraOffsetX);
        const f32 focusY = fixedCam ? centerY : (useY + 15.0f + mCameraOffsetY);
        const f32 focusZ = fixedCam ? centerZ : (useZ + fwdZ * lookAheadDist + mCameraOffsetZ);
        guVector camera = {
            fixedCam ? centerX : (useX - fwdX * cameraBackDist + mCameraOffsetX),
            fixedCam ? (maxY + 900.0f) : (useY + cameraHeight + mCameraOffsetY),
            fixedCam ? centerZ : (useZ - fwdZ * cameraBackDist + mCameraOffsetZ)
        };
        guVector up = {0.0f, 1.0f, 0.0f};
        guVector look = {focusX, focusY, focusZ};

        static int sCamActualLogCounter = 0;
        if(sCamActualLogCounter < 3)
        {
            WiiDebugLog("[TERRAIN_RENDER] camera at (%.1f,%.1f,%.1f) looking at (%.1f,%.1f,%.1f) fwd=(%.2f,%.2f,%.2f)\n",
                camera.x, camera.y, camera.z,
                look.x, look.y, look.z,
                fwdX, fwdY, fwdZ);
            WiiDebugLog("[TERRAIN_XFORM] fixedCam=%d forceIsolation=%d\n", fixedCam ? 1 : 0, kForceTransformIsolation ? 1 : 0);
            WiiDebugLog("[TERRAIN_XFORM] bbox=(%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f) center=(%.1f,%.1f,%.1f)\n",
                minX, minY, minZ, maxX, maxY, maxZ, centerX, centerY, centerZ);
            sCamActualLogCounter++;
        }

        Mtx44 projection;
        const float aspect = (CONF_GetAspectRatio() == CONF_ASPECT_16_9) ? (16.0f / 9.0f) : (4.0f / 3.0f);
        guPerspective(projection, 60.0f, aspect, 1.0f, 20000.0f);
        GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);

        guLookAt(view, &camera, &up, &look);

        guMtxIdentity(modelView);
        guMtxConcat(view, modelView, modelView);
        GX_LoadPosMtxImm(modelView, GX_PNMTX0);
        GX_SetCurrentMtx(GX_PNMTX0);

        GX_SetCullMode(GX_CULL_NONE);
        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GX_ClearVtxDesc();
        GX_SetVtxDesc(GX_VA_POS, GX_INDEX16);
        GX_SetVtxDesc(GX_VA_CLR0, GX_INDEX16);
        GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
        GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
        GX_SetNumChans(1);
        GX_SetNumTexGens(0);
        GX_SetNumTevStages(1);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

        if(gWiiTerrainDiagStage == 2)
        {
            Mtx ident;
            guMtxIdentity(ident);
            GX_LoadPosMtxImm(ident, GX_PNMTX0);
            GX_SetCurrentMtx(GX_PNMTX0);

            Mtx44 persp;
            const float aspect2 = (CONF_GetAspectRatio() == CONF_ASPECT_16_9) ? (16.0f / 9.0f) : (4.0f / 3.0f);
            guPerspective(persp, 60.0f, aspect2, 1.0f, 5000.0f);
            GX_LoadProjectionMtx(persp, GX_PERSPECTIVE);

            GX_ClearVtxDesc();
            GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
            GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
            GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
            GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
            GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
            GX_SetCullMode(GX_CULL_NONE);
            GX_SetLineWidth(48, GX_TO_ONE);

            // Big camera-space triangle: should always be visible if 3D transform path works.
            GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
            GX_Position3s16(-300, -180, -700); GX_Color4u8(255, 0, 255, 255);
            GX_Position3s16( 300, -180, -700); GX_Color4u8(255, 0, 255, 255);
            GX_Position3s16(   0,  240, -700); GX_Color4u8(255, 0, 255, 255);
            GX_End();

            // Big camera-space axis lines.
            GX_Begin(GX_LINES, GX_VTXFMT0, 6);
            GX_Position3s16(-500, 0, -700); GX_Color4u8(255, 0, 0, 255);
            GX_Position3s16( 500, 0, -700); GX_Color4u8(255, 0, 0, 255);

            GX_Position3s16(0, -500, -700); GX_Color4u8(0, 255, 0, 255);
            GX_Position3s16(0,  500, -700); GX_Color4u8(0, 255, 0, 255);

            GX_Position3s16(0, 0, -1200); GX_Color4u8(0, 128, 255, 255);
            GX_Position3s16(0, 0,  -200); GX_Color4u8(0, 128, 255, 255);
            GX_End();

            // Draw an indexed terrain subset transformed into camera space.
            if(mTerrainDirectDataValid && vertexCount > 0 && !mTerrainDirectIndices.empty())
            {
                const u32 triVerts = (mTerrainProbeDrawCount > 300u) ? 300u : mTerrainProbeDrawCount;
                u16 lastValid = 0;
                u32 invalid = 0;
                GX_Begin(GX_TRIANGLES, GX_VTXFMT0, triVerts);
                for(u32 i = 0; i < triVerts; ++i)
                {
                    const u32 idxRaw = mTerrainDirectIndices[i];
                    const u32 vi = (idxRaw < vertexCount) ? idxRaw : static_cast<u32>(lastValid);
                    if(idxRaw >= vertexCount)
                        ++invalid;
                    lastValid = static_cast<u16>(vi);

                    const float fx = mTerrainDirectVertices[vi * 3 + 0] - centerX;
                    const float fy = mTerrainDirectVertices[vi * 3 + 1] - centerY;
                    const float fz = mTerrainDirectVertices[vi * 3 + 2] - centerZ - 1200.0f;

                    const s16 sx = (fx < -32768.0f) ? static_cast<s16>(-32768) : ((fx > 32767.0f) ? static_cast<s16>(32767) : static_cast<s16>(fx));
                    const s16 sy = (fy < -32768.0f) ? static_cast<s16>(-32768) : ((fy > 32767.0f) ? static_cast<s16>(32767) : static_cast<s16>(fy));
                    const s16 sz = (fz < -32768.0f) ? static_cast<s16>(-32768) : ((fz > 32767.0f) ? static_cast<s16>(32767) : static_cast<s16>(fz));

                    GX_Position3s16(sx, sy, sz);
                    GX_Color4u8(0, 255, 255, 255);
                }
                GX_End();

                static int sStage2SubsetLog = 0;
                if(sStage2SubsetLog < 3)
                {
                    WiiDebugLog("[VISDBG] stage2 indexed terrain subset drawn verts=%u invalid=%u\n",
                        static_cast<unsigned int>(triVerts),
                        static_cast<unsigned int>(invalid));
                    sStage2SubsetLog++;
                }
            }

            static int sStage2Log = 0;
            if(sStage2Log < 3)
            {
                WiiDebugLog("[VISDBG] stage2 camera-space 3D primitives + terrain subset drawn\n");
                sStage2Log++;
            }
            return;
        }

        // Stable baseline path: draw terrain in centered camera-space.
        if(gWiiTerrainDiagStage >= 4)
        {
            static u16 sTerrainTex565[128 * 128] ATTRIBUTE_ALIGN(32);
            static GXTexObj sTerrainTexObj;
            static bool sTerrainTexReady = false;
            static std::string sLoadedTerrainTextureName;
            static std::string sLoadedTerrainTextureSource;
            static int sTexNotReadyLogs = 0;
            static int sTexDiagLogs = 0;
            static int sTexBindLogs = 0;
            static int sUVRangeLogs = 0;
            static int sUVSummaryLogs = 0;
            static int sBatchTexLogs = 0;
            static int sTexMissingLogs = 0;
            const char* kPinnedTerrainTexture = "roadsection2_m_1.tex";
            const char* kPinnedTerrainTextureAlt = "roadsection2.tga";
            const float kTerrainPlanarTextureScale = 10.0f;
            static const char* kDebugBatchTextures[] = {
                "roadsection2_m_1.tex",
                "startline_right_m_1.tex",
                "sandt3dcopy_m_1.tex"
            };

            const u32 drawCountStable = (mTerrainProbeDrawCount < indexCount) ? mTerrainProbeDrawCount : indexCount;
            const bool hasInlineUV = mTerrainDirectUVByIndex.size() >= (static_cast<size_t>(drawCountStable) * 2u);

            Mtx ident;
            guMtxIdentity(ident);
            GX_LoadPosMtxImm(ident, GX_PNMTX0);
            GX_SetCurrentMtx(GX_PNMTX0);

            Mtx44 persp;
            const float aspect2 = (CONF_GetAspectRatio() == CONF_ASPECT_16_9) ? (16.0f / 9.0f) : (4.0f / 3.0f);
            guPerspective(persp, 60.0f, aspect2, 1.0f, 20000.0f);
            GX_LoadProjectionMtx(persp, GX_PERSPECTIVE);

            const bool textureModeRequested = gEnableRealTerrainTexture;
            GX_ClearVtxDesc();
            GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
            GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
            if(textureModeRequested)
                GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
            GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
            GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
            if(textureModeRequested)
                GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
            GX_SetNumChans(1);
            GX_SetNumTexGens(textureModeRequested ? 1 : 0);
            GX_SetNumTevStages(1);
            if(textureModeRequested)
                GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
            GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
            GX_SetCullMode(GX_CULL_NONE);

            const float yawRad = gCameraYawDeg * 0.01745329251f;
            const float cYaw = cosf(yawRad);
            const float sYaw = sinf(yawRad);

            std::vector<TerrainChunkBatch> drawBatches;
            if(!mTerrainChunkBatches.empty())
                drawBatches = mTerrainChunkBatches;
            else
            {
                TerrainChunkBatch single;
                single.indexStart = 0;
                single.indexCount = drawCountStable;
                single.uvStart = 0;
                single.textureName = mTerrainTextureName;
                drawBatches.push_back(single);
            }

            unsigned int frameBatchCount = 0;
            unsigned int frameNoUVBatchCount = 0;

            for(size_t bi = 0; bi < drawBatches.size(); ++bi)
            {
                TerrainChunkBatch batch = drawBatches[bi];
                if(batch.indexStart >= drawCountStable)
                    continue;
                if(batch.indexStart + batch.indexCount > drawCountStable)
                    batch.indexCount = drawCountStable - batch.indexStart;
                if(batch.indexCount == 0)
                    continue;

                bool useTerrainTexture = false;
                bool batchHasUV = false;
                u32 uvStart = batch.uvStart;

                if(textureModeRequested)
                {
                    std::string requestedTexture = batch.textureName;
                    if(requestedTexture.empty())
                    {
                        if(sTexMissingLogs < 128)
                        {
                            WiiDebugLog("[TEX_MISS] bi=%u chunk='<empty>' requested='<empty>' (no global fallback)\n",
                                static_cast<unsigned int>(bi));
                            sTexMissingLogs++;
                        }
                        continue;
                    }
                    if(gTerrainBatchTextureDebug)
                    {
                        const size_t debugCount = sizeof(kDebugBatchTextures) / sizeof(kDebugBatchTextures[0]);
                        requestedTexture = kDebugBatchTextures[bi % debugCount];
                    }
                    if(sLoadedTerrainTextureName != requestedTexture)
                        sTerrainTexReady = false;

                    Ogre::TexturePtr resolvedTex;
                    std::string resolvedName;
                    {
                        std::vector<std::string> candidates;
                        candidates.push_back(requestedTexture);

                        const size_t dot = requestedTexture.find_last_of('.');
                        const std::string base = (dot == std::string::npos) ? requestedTexture : requestedTexture.substr(0, dot);
                        if(!base.empty())
                        {
                            candidates.push_back(base + "_m_1.tex");
                            candidates.push_back(base + "_m_2.tex");
                            candidates.push_back(base + "_m_3.tex");
                            candidates.push_back(base + ".tga");
                            candidates.push_back(base + ".png");
                        }

                        for(size_t ci = 0; ci < candidates.size(); ++ci)
                        {
                            Ogre::TexturePtr probe = Ogre::TextureManager::getSingleton().getByName(candidates[ci], "tempRes");
                            const Ogre::Texture* p = probe.get();
                            if(p && p->getRawData() && p->getWidth() > 0 && p->getHeight() > 0)
                            {
                                resolvedTex = probe;
                                resolvedName = candidates[ci];
                                break;
                            }
                        }
                    }

                    if(sBatchTexLogs < 96)
                    {
                        WiiDebugLog("[TEX_BATCH] bi=%u start=%u count=%u raw='%s' requested='%s' debug=%d\n",
                            static_cast<unsigned int>(bi),
                            static_cast<unsigned int>(batch.indexStart),
                            static_cast<unsigned int>(batch.indexCount),
                            batch.textureName.empty() ? "<default>" : batch.textureName.c_str(),
                            requestedTexture.c_str(),
                            gTerrainBatchTextureDebug ? 1 : 0);
                        sBatchTexLogs++;
                    }

                    bool textureResolvedForBatch = false;

                    if(!sTerrainTexReady)
                    {
                        const char* requestedName = requestedTexture.c_str();
                        Ogre::TexturePtr tex = resolvedTex;
                        if(!tex.get())
                            tex = Ogre::TextureManager::getSingleton().getByName(requestedName, "tempRes");
                        const Ogre::Texture* texPtr = tex.get();
                        const unsigned char* src = texPtr ? texPtr->getRawData() : 0;
                        unsigned int srcW = texPtr ? texPtr->getWidth() : 0;
                        unsigned int srcH = texPtr ? texPtr->getHeight() : 0;
                        Ogre::PixelFormat srcFmt = texPtr ? texPtr->getFormat() : Ogre::PF_UNKNOWN;
                        std::string srcName = resolvedName.empty() ? std::string(requestedName) : resolvedName;

                        if(!(src && srcW > 0 && srcH > 0) && requestedTexture.empty())
                        {
                            Ogre::TexturePtr texAlt = Ogre::TextureManager::getSingleton().getByName(kPinnedTerrainTextureAlt, "tempRes");
                            const Ogre::Texture* texAltPtr = texAlt.get();
                            if(texAltPtr && texAltPtr->getRawData() && texAltPtr->getWidth() > 0 && texAltPtr->getHeight() > 0)
                            {
                                src = texAltPtr->getRawData();
                                srcW = texAltPtr->getWidth();
                                srcH = texAltPtr->getHeight();
                                srcFmt = texAltPtr->getFormat();
                                srcName = kPinnedTerrainTextureAlt;
                            }
                        }

                        if(!(src && srcW > 0 && srcH > 0) && requestedTexture.empty())
                        {
                            Ogre::String fallbackName;
                            Ogre::TexturePtr fallback = Ogre::TextureManager::getSingleton().getFirstPopulatedTexture(&fallbackName);
                            const Ogre::Texture* fallbackPtr = fallback.get();
                            if(fallbackPtr && fallbackPtr->getRawData() && fallbackPtr->getWidth() > 0 && fallbackPtr->getHeight() > 0)
                            {
                                src = fallbackPtr->getRawData();
                                srcW = fallbackPtr->getWidth();
                                srcH = fallbackPtr->getHeight();
                                srcFmt = fallbackPtr->getFormat();
                                srcName = fallbackName;
                            }
                        }

                        if(src && srcW > 0 && srcH > 0)
                        {
                            const unsigned int copyW = (srcW > 128u) ? 128u : srcW;
                            const unsigned int copyH = (srcH > 128u) ? 128u : srcH;
                            for(unsigned int y = 0; y < 128u; ++y)
                            {
                                const unsigned int sy = y % copyH;
                                for(unsigned int x = 0; x < 128u; ++x)
                                {
                                    const unsigned int sx = x % copyW;
                                    unsigned int r = 255;
                                    unsigned int g = 255;
                                    unsigned int b = 255;

                                    if(srcFmt == Ogre::PF_R5G6B5)
                                    {
                                        const size_t idx = (static_cast<size_t>(sy) * srcW + sx) * 2u;
                                        const u16 packed = static_cast<u16>(src[idx + 0] | (static_cast<unsigned int>(src[idx + 1]) << 8));
                                        r = static_cast<unsigned int>((packed >> 11) & 0x1Fu) * 255u / 31u;
                                        g = static_cast<unsigned int>((packed >> 5) & 0x3Fu) * 255u / 63u;
                                        b = static_cast<unsigned int>(packed & 0x1Fu) * 255u / 31u;
                                    }
                                    else if(srcFmt == Ogre::PF_BYTE_BGRA || srcFmt == Ogre::PF_B8G8R8A8)
                                    {
                                        const size_t idx = (static_cast<size_t>(sy) * srcW + sx) * 4u;
                                        b = src[idx + 0];
                                        g = src[idx + 1];
                                        r = src[idx + 2];
                                    }
                                    else if(srcFmt == Ogre::PF_A8R8G8B8)
                                    {
                                        const size_t idx = (static_cast<size_t>(sy) * srcW + sx) * 4u;
                                        r = src[idx + 1];
                                        g = src[idx + 2];
                                        b = src[idx + 3];
                                    }
                                    else if(srcFmt == Ogre::PF_R8G8B8)
                                    {
                                        const size_t idx = (static_cast<size_t>(sy) * srcW + sx) * 3u;
                                        r = src[idx + 0];
                                        g = src[idx + 1];
                                        b = src[idx + 2];
                                    }
                                    else if(srcFmt == Ogre::PF_B8G8R8)
                                    {
                                        const size_t idx = (static_cast<size_t>(sy) * srcW + sx) * 3u;
                                        b = src[idx + 0];
                                        g = src[idx + 1];
                                        r = src[idx + 2];
                                    }
                                    else if(srcFmt == Ogre::PF_BYTE_RGB)
                                    {
                                        const size_t idx = (static_cast<size_t>(sy) * srcW + sx) * 3u;
                                        r = src[idx + 0];
                                        g = src[idx + 1];
                                        b = src[idx + 2];
                                    }

                                    const u16 r5 = static_cast<u16>((r >> 3) & 0x1F);
                                    const u16 g6 = static_cast<u16>((g >> 2) & 0x3F);
                                    const u16 b5 = static_cast<u16>((b >> 3) & 0x1F);
                                    sTerrainTex565[y * 128 + x] = static_cast<u16>((r5 << 11) | (g6 << 5) | b5);
                                }
                            }
                            DCFlushRange(sTerrainTex565, sizeof(sTerrainTex565));
                            GX_InitTexObj(&sTerrainTexObj, sTerrainTex565, 128, 128, GX_TF_RGB565, GX_REPEAT, GX_REPEAT, GX_FALSE);
                            GX_InitTexObjLOD(&sTerrainTexObj, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
                            sTerrainTexReady = true;
                            sLoadedTerrainTextureName = requestedTexture;
                            sLoadedTerrainTextureSource = srcName;
                            textureResolvedForBatch = true;
                            if(sTexBindLogs < 64)
                            {
                                WiiDebugLog("[TEX_BIND] requested='%s' source='%s' chunk='%s' fmt=%d size=%ux%u\n",
                                    requestedName,
                                    srcName.c_str(),
                                    batch.textureName.empty() ? "<default>" : batch.textureName.c_str(),
                                    static_cast<int>(srcFmt),
                                    srcW,
                                    srcH);
                                sTexBindLogs++;
                            }
                        }
                        else if(sTexNotReadyLogs < 32)
                        {
                            WiiDebugLog("[TEX_BIND] texture not ready requested='%s'\n", requestedName);
                            sTexNotReadyLogs++;
                        }
                    }
                    else
                    {
                        textureResolvedForBatch = true;
                    }

                    if(!textureResolvedForBatch)
                    {
                        if(sTexMissingLogs < 128)
                        {
                            WiiDebugLog("[TEX_MISS] bi=%u chunk='%s' requested='%s' (batch skipped)\n",
                                static_cast<unsigned int>(bi),
                                batch.textureName.empty() ? "<default>" : batch.textureName.c_str(),
                                requestedTexture.c_str());
                            sTexMissingLogs++;
                        }
                        continue;
                    }

                    useTerrainTexture = sTerrainTexReady;
                    batchHasUV = hasInlineUV && (mTerrainDirectUVByIndex.size() >= (static_cast<size_t>(uvStart + batch.indexCount) * 2u));
                    frameBatchCount++;
                    if(!batchHasUV)
                        frameNoUVBatchCount++;
                }

                if(useTerrainTexture)
                {
                    GX_LoadTexObj(&sTerrainTexObj, GX_TEXMAP0);
                    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
                    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
                }
                else
                {
                    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
                    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
                }

                u16 lastValid = 0;
                u32 invalid = 0;
                u32 invalidUV = 0;
                float minU = 1.0e30f;
                float minV = 1.0e30f;
                float maxU = -1.0e30f;
                float maxV = -1.0e30f;

                GX_Begin(GX_TRIANGLES, GX_VTXFMT0, batch.indexCount);
                for(u32 localI = 0; localI < batch.indexCount; ++localI)
                {
                    const u32 globalI = batch.indexStart + localI;
                    const u32 idxRaw = mTerrainDirectIndices[globalI];
                    const u32 vi = (idxRaw < vertexCount) ? idxRaw : static_cast<u32>(lastValid);
                    if(idxRaw >= vertexCount)
                        ++invalid;
                    lastValid = static_cast<u16>(vi);

                    const float rx = (mTerrainDirectVertices[vi * 3 + 0] - centerX) - mCameraOffsetX;
                    const float fy = (mTerrainDirectVertices[vi * 3 + 1] - centerY) - mCameraOffsetY;
                    const float rz = (mTerrainDirectVertices[vi * 3 + 2] - centerZ - 1200.0f) - mCameraOffsetZ;
                    const float fx = rx * cYaw - rz * sYaw;
                    const float fz = rx * sYaw + rz * cYaw;
                    const float wy = mTerrainDirectVertices[vi * 3 + 1];

                    const s16 sx = (fx < -32768.0f) ? static_cast<s16>(-32768) : ((fx > 32767.0f) ? static_cast<s16>(32767) : static_cast<s16>(fx));
                    const s16 sy = (fy < -32768.0f) ? static_cast<s16>(-32768) : ((fy > 32767.0f) ? static_cast<s16>(32767) : static_cast<s16>(fy));
                    const s16 sz = (fz < -32768.0f) ? static_cast<s16>(-32768) : ((fz > 32767.0f) ? static_cast<s16>(32767) : static_cast<s16>(fz));

                    const float yRange = (maxY > minY) ? (maxY - minY) : 1.0f;
                    float t = (wy - minY) / yRange;
                    if(t < 0.0f) t = 0.0f;
                    if(t > 1.0f) t = 1.0f;
                    const u8 cR = static_cast<u8>(20.0f + 110.0f * t);
                    const u8 cG = static_cast<u8>(90.0f + 150.0f * t);
                    const u8 cB = static_cast<u8>(120.0f + 120.0f * t);

                    GX_Position3s16(sx, sy, sz);
                    if(useTerrainTexture)
                    {
                        GX_Color4u8(255, 255, 255, 255);
                    }
                    else if(textureModeRequested)
                    {
                        GX_Color4u8(255, 0, 255, 255);
                    }
                    else
                    {
                        GX_Color4u8(cR, cG, cB, 255);
                    }

                    if(textureModeRequested)
                    {
                        const float worldX = mTerrainDirectVertices[vi * 3 + 0];
                        const float worldZ = mTerrainDirectVertices[vi * 3 + 2];
                        const float outU = worldX / kTerrainPlanarTextureScale;
                        const float outV = worldZ / kTerrainPlanarTextureScale;
                        if(outU < minU) minU = outU;
                        if(outU > maxU) maxU = outU;
                        if(outV < minV) minV = outV;
                        if(outV > maxV) maxV = outV;
                        GX_TexCoord2f32(outU, outV);
                    }
                }
                GX_End();

                if(textureModeRequested && sUVRangeLogs < 64)
                {
                    WiiDebugLog("[UVDBG] batch=%u start=%u count=%u hasUV=%d invalidUV=%u rangeU=%.3f..%.3f rangeV=%.3f..%.3f tex='%s'\n",
                        static_cast<unsigned int>(bi),
                        static_cast<unsigned int>(batch.indexStart),
                        static_cast<unsigned int>(batch.indexCount),
                        batchHasUV ? 1 : 0,
                        static_cast<unsigned int>(invalidUV),
                        minU,
                        maxU,
                        minV,
                        maxV,
                        batch.textureName.empty() ? "<default>" : batch.textureName.c_str());
                    sUVRangeLogs++;
                }

                if(sTexDiagLogs < 16)
                {
                    WiiDebugLog("[VISDBG] stable centered terrain batch=%u verts=%u invalid=%u textured=%d\n",
                        static_cast<unsigned int>(bi),
                        static_cast<unsigned int>(batch.indexCount),
                        static_cast<unsigned int>(invalid),
                        useTerrainTexture ? 1 : 0);
                    sTexDiagLogs++;
                }

                if(textureModeRequested && sUVSummaryLogs < 8 && bi + 1 == drawBatches.size())
                {
                    WiiDebugLog("[UV_SUMMARY] batches=%u noUV=%u scale=%.2f\n",
                        static_cast<unsigned int>(frameBatchCount),
                        static_cast<unsigned int>(frameNoUVBatchCount),
                        kTerrainPlanarTextureScale);
                    sUVSummaryLogs++;
                }
            }
            return;
        }

        static s16 terrainVerts[WIIGX_MAX_TERRAIN_VERTICES * 3] ATTRIBUTE_ALIGN(32);
        static u8 terrainColors[WIIGX_MAX_TERRAIN_VERTICES * 4] ATTRIBUTE_ALIGN(32);

        if(vertexCount == 0)
            return;

        for(u32 v = 0; v < vertexCount; ++v)
        {
            const float fx = mTerrainDirectVertices[v * 3 + 0];
            const float fy = mTerrainDirectVertices[v * 3 + 1];
            const float fz = mTerrainDirectVertices[v * 3 + 2];

            const s16 sx = (fx < -32768.0f) ? static_cast<s16>(-32768) : ((fx > 32767.0f) ? static_cast<s16>(32767) : static_cast<s16>(fx));
            const s16 sy = (fy < -32768.0f) ? static_cast<s16>(-32768) : ((fy > 32767.0f) ? static_cast<s16>(32767) : static_cast<s16>(fy));
            const s16 sz = (fz < -32768.0f) ? static_cast<s16>(-32768) : ((fz > 32767.0f) ? static_cast<s16>(32767) : static_cast<s16>(fz));

            terrainVerts[v * 3 + 0] = sx;
            terrainVerts[v * 3 + 1] = sy;
            terrainVerts[v * 3 + 2] = sz;

            terrainColors[v * 4 + 0] = static_cast<u8>(mChunkR * 255.0f);
            terrainColors[v * 4 + 1] = static_cast<u8>(mChunkG * 255.0f);
            terrainColors[v * 4 + 2] = static_cast<u8>(mChunkB * 255.0f);
            terrainColors[v * 4 + 3] = 255;
        }

        DCFlushRange(terrainVerts, vertexCount * 3 * sizeof(s16));
        DCFlushRange(terrainColors, vertexCount * 4 * sizeof(u8));

        GX_SetArray(GX_VA_POS, terrainVerts, 3 * sizeof(s16));
        GX_SetArray(GX_VA_CLR0, terrainColors, 4 * sizeof(u8));

        static int sDrawLogCounter = 0;
        if(sDrawLogCounter < 3)
        {
            WiiDebugLog("[TERRAIN_RENDER] drawing %u triangles, first vert=(%.1f,%.1f,%.1f)\n",
                mTerrainProbeDrawCount,
                mTerrainDirectVertices[0],
                mTerrainDirectVertices[1],
                mTerrainDirectVertices[2]);
            WiiDebugLog("[TERRAIN_RENDER] verts[0]=(%.2f,%.2f,%.2f) verts[1]=(%.2f,%.2f,%.2f) verts[2]=(%.2f,%.2f,%.2f)\n",
                mTerrainDirectVertices[0], mTerrainDirectVertices[1], mTerrainDirectVertices[2],
                mTerrainDirectVertices[3], mTerrainDirectVertices[4], mTerrainDirectVertices[5],
                mTerrainDirectVertices[6], mTerrainDirectVertices[7], mTerrainDirectVertices[8]);
            sDrawLogCounter++;
        }

        static s16 testVerts[9] ATTRIBUTE_ALIGN(32) = {
            -80, 30, -220,
             80, 30, -220,
              0, 130, -220
        };
        static u8 testColors[12] ATTRIBUTE_ALIGN(32) = {
            255, 255, 0, 255,
            255, 255, 0, 255,
            255, 255, 0, 255
        };
        DCFlushRange(testVerts, sizeof(testVerts));
        DCFlushRange(testColors, sizeof(testColors));
        if(gWiiTerrainDiagStage >= 1)
        {
            GX_SetArray(GX_VA_POS, testVerts, 3 * sizeof(s16));
            GX_SetArray(GX_VA_CLR0, testColors, 4 * sizeof(u8));
            GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 3);
            GX_Position1x16(0); GX_Color1x16(0);
            GX_Position1x16(1); GX_Color1x16(1);
            GX_Position1x16(2); GX_Color1x16(2);
            GX_End();
        }

        if(gWiiTerrainDiagStage == 1)
            return;

        GX_SetArray(GX_VA_POS, terrainVerts, 3 * sizeof(s16));
        GX_SetArray(GX_VA_CLR0, terrainColors, 4 * sizeof(u8));

        const u32 drawCount =
            (gWiiTerrainDiagStage == 2) ? ((mTerrainProbeDrawCount > 900u) ? 900u : mTerrainProbeDrawCount) :
            (gWiiTerrainDiagStage == 3) ? ((mTerrainProbeDrawCount > 4500u) ? 4500u : mTerrainProbeDrawCount) :
            mTerrainProbeDrawCount;
        const bool wireframeDiag = kForceTransformIsolation ? false : (gWiiTerrainDiagStage <= 3);
        u32 maxDrawnIndex = 0;
        for(u32 i = 0; i < drawCount; ++i)
        {
            if(mTerrainDirectIndices[i] > maxDrawnIndex)
                maxDrawnIndex = mTerrainDirectIndices[i];
        }

        u32 invalidIndexCount = 0;
        u16 lastValidIdx = 0;

        static int sSubmitVertexLogCounter = 0;
        if(sSubmitVertexLogCounter < 3 && drawCount >= 3)
        {
            for(u32 i = 0; i < 3; ++i)
            {
                const u32 viRaw = mTerrainDirectIndices[i];
                const u16 vi = (viRaw < vertexCount) ? static_cast<u16>(viRaw) : static_cast<u16>(0);
                const s16 sx = terrainVerts[vi * 3 + 0];
                const s16 sy = terrainVerts[vi * 3 + 1];
                const s16 sz = terrainVerts[vi * 3 + 2];
                const float fx = mTerrainDirectVertices[vi * 3 + 0];
                const float fy = mTerrainDirectVertices[vi * 3 + 1];
                const float fz = mTerrainDirectVertices[vi * 3 + 2];
                OSReport("[TERRAIN_VTX] submit[%u] idx=%u pos_s16=(%d,%d,%d) pos_f32=(%.2f,%.2f,%.2f)\n",
                    static_cast<unsigned int>(i),
                    static_cast<unsigned int>(vi),
                    static_cast<int>(sx),
                    static_cast<int>(sy),
                    static_cast<int>(sz),
                    fx, fy, fz);
                WiiDebugLog("[TERRAIN_VTX] submit[%u] idx=%u pos_s16=(%d,%d,%d) pos_f32=(%.2f,%.2f,%.2f)\n",
                    static_cast<unsigned int>(i),
                    static_cast<unsigned int>(vi),
                    static_cast<int>(sx),
                    static_cast<int>(sy),
                    static_cast<int>(sz),
                    fx, fy, fz);
            }
            sSubmitVertexLogCounter++;
        }

        if(!wireframeDiag)
        {
            GX_Begin(GX_TRIANGLES, GX_VTXFMT0, drawCount);
            for(u32 i = 0; i < drawCount; ++i)
            {
                const u32 vi = mTerrainDirectIndices[i];
                u16 idx = lastValidIdx;
                if(vi < vertexCount)
                {
                    idx = static_cast<u16>(vi);
                    lastValidIdx = idx;
                }
                else
                {
                    ++invalidIndexCount;
                }

                GX_Position1x16(idx);
                GX_Color1x16(idx);
            }
            GX_End();
        }
        else
        {
            const u32 triCount = drawCount / 3;
            GX_Begin(GX_LINES, GX_VTXFMT0, triCount * 6);
            for(u32 t = 0; t < triCount; ++t)
            {
                const u32 iaRaw = mTerrainDirectIndices[t * 3 + 0];
                const u32 ibRaw = mTerrainDirectIndices[t * 3 + 1];
                const u32 icRaw = mTerrainDirectIndices[t * 3 + 2];

                const u16 ia = (iaRaw < vertexCount) ? static_cast<u16>(iaRaw) : lastValidIdx;
                const u16 ib = (ibRaw < vertexCount) ? static_cast<u16>(ibRaw) : lastValidIdx;
                const u16 ic = (icRaw < vertexCount) ? static_cast<u16>(icRaw) : lastValidIdx;

                if(iaRaw >= vertexCount) ++invalidIndexCount;
                if(ibRaw >= vertexCount) ++invalidIndexCount;
                if(icRaw >= vertexCount) ++invalidIndexCount;

                lastValidIdx = ia;

                GX_Position1x16(ia); GX_Color1x16(ia);
                GX_Position1x16(ib); GX_Color1x16(ib);

                GX_Position1x16(ib); GX_Color1x16(ib);
                GX_Position1x16(ic); GX_Color1x16(ic);

                GX_Position1x16(ic); GX_Color1x16(ic);
                GX_Position1x16(ia); GX_Color1x16(ia);
            }
            GX_End();
        }

        static int sTopologyLogCounter = 0;
        if(sTopologyLogCounter < 5)
        {
            WiiDebugLog("[TERRAIN_RENDER] indexed topology drawCount=%u vertexCount=%u indexCount=%u invalid=%u maxDrawnIndex=%u wire=%d\n",
                static_cast<unsigned int>(drawCount),
                static_cast<unsigned int>(vertexCount),
                static_cast<unsigned int>(indexCount),
                static_cast<unsigned int>(invalidIndexCount),
                static_cast<unsigned int>(maxDrawnIndex),
                wireframeDiag ? 1 : 0);
            sTopologyLogCounter++;
        }

        static int sDoneLogCounter = 0;
        if(sDoneLogCounter < 3)
        {
            WiiDebugLog("[TERRAIN_RENDER] draw complete\n");
            sDoneLogCounter++;
        }
    }
    
}
