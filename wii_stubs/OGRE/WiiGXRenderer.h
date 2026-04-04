#ifndef WiiGXRenderer_H
#define WiiGXRenderer_H

#include <gccore.h>
#include <ogc/gx.h>
#include <ogc/gu.h>
#include <vector>
#include <map>
#include <string>
#include <string.h>

#define WIIGX_MAX_VERTICES 10000
#define WIIGX_MAX_TERRAIN_VERTICES 65535
#define WIIGX_MAX_MESHES 100
#define WIIGX_FRAMERATE 60

namespace WiiGX
{
    struct Vec3 {
        float x, y, z;
        Vec3() : x(0), y(0), z(0) {}
        Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    };
    
    struct Vec4 {
        float x, y, z, w;
        Vec4() : x(0), y(0), z(0), w(1) {}
        Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
    };
    
    struct Color {
        u8 r, g, b, a;
        Color() : r(255), g(255), b(255), a(255) {}
        Color(u8 _r, u8 _g, u8 _b) : r(_r), g(_g), b(_b), a(255) {}
        Color(u8 _r, u8 _g, u8 _b, u8 _a = 255) : r(_r), g(_g), b(_b), a(_a) {}
        Color(int _r, int _g, int _b) : r((u8)_r), g((u8)_g), b((u8)_b), a(255) {}
        Color(int _r, int _g, int _b, int _a) : r((u8)_r), g((u8)_g), b((u8)_b), a((u8)_a) {}
    };
    
    struct Vertex {
        Vec3 position;
        Vec3 normal;
        Color color;
        float tu, tv;
    };
    
    struct Material {
        Color ambient;
        Color diffuse;
        Color specular;
        float shininess;
        bool wireframe;
        Material() : ambient(50,50,50), diffuse(200,200,200), specular(255,255,255), shininess(0), wireframe(false) {}
    };
    
    struct Matrix4x4 {
        float m[16];
        Matrix4x4() { memset(m, 0, sizeof(m)); m[0]=m[5]=m[10]=m[15]=1.0f; }
        static Matrix4x4 identity() { Matrix4x4 r; memset(r.m, 0, sizeof(r.m)); r.m[0]=r.m[5]=r.m[10]=r.m[15]=1.0f; return r; }
    };
    
    class Renderer
    {
    public:
        static Renderer& getInstance();
        
        void init();
        void shutdown();
        void beginFrame();
        void endFrame();
        void setFramePresentEnabled(bool enable);
        
        void setClearColor(u8 r, u8 g, u8 b, u8 a);
        void presentProbeColor(u8 r, u8 g, u8 b, u8 a = 255);
        void clearBuffers();
        
        void setProjection(const Matrix4x4& proj);
        void setView(const Matrix4x4& view);
        void setWorld(const Matrix4x4& world);
        
        void setMaterial(const Material& mat);
        
        void drawPrimitives(int type, const Vertex* vertices, u32 count);
        void drawIndexedPrimitives(int type, const Vertex* vertices, u32 vcount, const u16* indices, u32 icount);
        
        void enableDepthTest(bool enable);
        void enableDepthWrite(bool enable);
        void enableBlend(bool enable);
        
        void setViewport(int x, int y, int w, int h);

        bool uploadTrackBufferBE(const void* beXYZS16, u32 vertexCount);
        void renderTrackBuffer();
        void clearTerrainBuffer();
        bool uploadTerrainBufferF32(const float* xyzF32, u32 vertexCount);
        void renderTerrainBuffer();
        void setTerrainBuildProbe(u32 triCount);
        void setTerrainFocusF32(float x, float y, float z);
        void setCarStartPositionF32(float x, float y, float z);
        void setCarForwardDirF32(float x, float y, float z);
        void setTerrainTextureName(const std::string& textureName);
        void setTerrainTextureClamp(bool clampTexture);
        void setTerrainTextureScale(float scaleU, float scaleV);
        void updateCameraFromInput();
        void setTerrainChunkColorF32(float r, float g, float b);
        bool uploadTerrainIndexedData(const float* vertexPositions, u32 vertexCount, const u32* indices, u32 indexCount);
        bool uploadTerrainIndexedDataWithUV(const float* vertexPositions, u32 vertexCount, const u32* indices, u32 indexCount, const float* uvByIndexF32);
        
        float getFPS() const { return mFPS; }
        float getDeltaTime() const { return mDeltaTime; }
        
    private:
        struct TerrainChunkBatch
        {
            u32 indexStart;
            u32 indexCount;
            u32 uvStart;
            std::string textureName;
            bool clampTexture;
            float textureScaleU;
            float textureScaleV;
        };

        Renderer();
        ~Renderer();
        
        void setupGX(GXRModeObj* rmode);
        void uploadVertices(const Vertex* vertices, u32 count);
        
        u8 mClearColor[4];
        void* mXfb;
        Matrix4x4 mProjection;
        Matrix4x4 mViewMatrix;
        Matrix4x4 mWorldMatrix;
        Material mCurrentMaterial;
        
        Vertex mVertexBuffer[WIIGX_MAX_VERTICES];
        u32 mVertexCount;
        
        float mFPS;
        float mDeltaTime;
        u32 mFrameCount;
        u64 mLastFrameTime;
        
        float mRotation;
        
        bool mDepthTestEnabled;
        bool mDepthWriteEnabled;
        bool mBlendEnabled;
        
        int mViewport[4];

        s16* mTrackVertices;
        u32 mTrackVertexCount;
        s16* mTerrainVertices;
        u32 mTerrainVertexCount;
        bool mTerrainProbeCreateMeshSeen;
        u32 mTerrainProbeTriCount;
        bool mTerrainProbeUploadOk;
        u32 mTerrainProbeUploadVerts;
        bool mTerrainProbeRenderCalled;
        u32 mTerrainProbeDrawCount;
        float mTerrainFocusX;
        float mTerrainFocusY;
        float mTerrainFocusZ;
        float mCarStartX;
        float mCarStartY;
        float mCarStartZ;
        float mCarForwardX;
        float mCarForwardY;
        float mCarForwardZ;
        float mCameraOffsetX;
        float mCameraOffsetY;
        float mCameraOffsetZ;
        float mChunkR;
        float mChunkG;
        float mChunkB;
        std::vector<float> mTerrainDirectVertices;
        std::vector<u32> mTerrainDirectIndices;
        std::vector<float> mTerrainDirectUVByIndex;
        std::vector<TerrainChunkBatch> mTerrainChunkBatches;
        std::string mTerrainTextureName;
        bool mTerrainTextureClamp;
        float mTerrainTextureScaleU;
        float mTerrainTextureScaleV;
        bool mTerrainDirectDataValid;
        bool mFramePresentEnabled;
    };
    
    class Timer
    {
    public:
        Timer() : mStart(0), mFrequency(0) {
            mFrequency = getFrequency();
            mStart = getTicks();
        }
        
        float getElapsed() const {
            return (float)(getTicks() - mStart) / (float)mFrequency;
        }
        
        void reset() { mStart = getTicks(); }
        
    private:
        u64 mStart;
        u64 mFrequency;
        
        u64 getTicks() const {
            u64 ticks;
            __asm__ volatile("mftbl %0" : "=r"(ticks));
            return ticks;
        }
        
        u64 getFrequency() const {
            return 243000000 * 4 / 4;
        }
    };
}

#endif
