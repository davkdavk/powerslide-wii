/*
 * Wii GX Renderer - Based on devkitPro triangle example
 */

#include "WiiGXRenderer.h"
#include "../../orig_src/WiiDebugLog.h"
#include <gccore.h>
#include <ogc/gx.h>
#include <ogc/gu.h>
#include <ogc/system.h>
#include <ogc/console.h>
#include <wiiuse/wpad.h>
#include <malloc.h>
#include <string.h>
#include <math.h>

#if defined(WII) || defined(__wii__)
#define OSReport SYS_Report
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
           mChunkB(0.2f)
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
        GX_DrawDone();
        sReadyForCopy = GX_TRUE;
        VIDEO_WaitVSync();
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
        const u32 floatCount = vertexCount * 3;
        mTerrainDirectVertices.insert(mTerrainDirectVertices.end(), vertexPositions, vertexPositions + floatCount);

        for(u32 i = 0; i < indexCount; ++i)
            mTerrainDirectIndices.push_back(indices[i] + baseVert);

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

    void Renderer::updateCameraFromInput()
    {
        WPAD_ScanPads();
        const u32 held = WPAD_ButtonsHeld(0);
        const float speed = 100.0f;
        if(held & WPAD_BUTTON_UP)    mCameraOffsetZ -= speed;
        if(held & WPAD_BUTTON_DOWN)  mCameraOffsetZ += speed;
        if(held & WPAD_BUTTON_LEFT)  mCameraOffsetX -= speed;
        if(held & WPAD_BUTTON_RIGHT) mCameraOffsetX += speed;
        if(held & WPAD_BUTTON_A)     mCameraOffsetY += speed;
        if(held & WPAD_BUTTON_B)     mCameraOffsetY -= speed;
    }

    void Renderer::renderTerrainBuffer()
    {
        OSReport("[TERRAIN] render vertCount=%d\n", static_cast<int>(mTerrainVertexCount));
        updateCameraFromInput();

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
            WiiDebugLog("[TERRAIN_RENDER] camera at (%.1f,%.1f,%.1f) looking at (%.1f,%.1f,%.1f) fwd=(%.2f,%.2f,%.2f)\n",
                useX - fwdX * cameraBackDist + mCameraOffsetX,
                useY + cameraHeight + mCameraOffsetY,
                useZ - fwdZ * cameraBackDist + mCameraOffsetZ,
                useX + fwdX * lookAheadDist + mCameraOffsetX,
                useY + 15.0f + mCameraOffsetY,
                useZ + fwdZ * lookAheadDist + mCameraOffsetZ,
                fwdX, fwdY, fwdZ);
            sCamLogCounter++;
        }

        Mtx view;
        Mtx modelView;
        const f32 focusX = useX + fwdX * lookAheadDist + mCameraOffsetX;
        const f32 focusY = useY + 15.0f + mCameraOffsetY;
        const f32 focusZ = useZ + fwdZ * lookAheadDist + mCameraOffsetZ;
        guVector camera = {
            useX - fwdX * cameraBackDist + mCameraOffsetX,
            useY + cameraHeight + mCameraOffsetY,
            useZ - fwdZ * cameraBackDist + mCameraOffsetZ
        };
        guVector up = {0.0f, 1.0f, 0.0f};
        guVector look = {focusX, focusY, focusZ};
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
            sDrawLogCounter++;
        }

        u32 invalidIndexCount = 0;
        u16 lastValidIdx = 0;
        GX_Begin(GX_TRIANGLES, GX_VTXFMT0, mTerrainProbeDrawCount);
        for(u32 i = 0; i < mTerrainProbeDrawCount; ++i)
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

        static int sTopologyLogCounter = 0;
        if(sTopologyLogCounter < 5)
        {
            WiiDebugLog("[TERRAIN_RENDER] indexed topology drawCount=%u vertexCount=%u indexCount=%u invalid=%u\n",
                static_cast<unsigned int>(mTerrainProbeDrawCount),
                static_cast<unsigned int>(vertexCount),
                static_cast<unsigned int>(indexCount),
                static_cast<unsigned int>(invalidIndexCount));
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
