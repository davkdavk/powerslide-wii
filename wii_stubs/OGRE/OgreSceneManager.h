#ifndef OGRE_SCENEMANAGER_H
#define OGRE_SCENEMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cmath>
#include <cstring>
#include <algorithm>

#if defined(WII) || defined(__wii__)
#include "WiiGXRenderer.h"
#endif

namespace Ogre
{
    template <typename T> class SharedPtr;
    class ShadowCameraSetup;

    typedef float Real;
    typedef unsigned short ushort;
    typedef unsigned int uint;
    typedef unsigned long ulong;
    typedef unsigned char uchar;
    typedef unsigned int RGBA;
    typedef unsigned char uint8;
    typedef short int16;
    typedef unsigned short uint16;
    typedef int int32;
    typedef unsigned int uint32;
    typedef unsigned long long uint64;
    typedef std::string String;

    class Quaternion;

    class Radian
    {
    public:
        Real valueRadians;
        explicit Radian(Real v = 0.0f) : valueRadians(v) {}
    };

    class Degree
    {
    public:
        Real valueDegrees;
        explicit Degree(Real v = 0.0f) : valueDegrees(v) {}
        operator Radian() const { return Radian(valueDegrees * 3.1415926535f / 180.0f); }
        Real valueDegreesFn() const { return valueDegrees; }
        Real valueRadians() const { return valueDegrees * 3.1415926535f / 180.0f; }
    };

    class Vector2
    {
    public:
        Real x, y;
        Vector2() : x(0), y(0) {}
        Vector2(Real _x, Real _y) : x(_x), y(_y) {}
        Vector2 operator+(const Vector2& v) const { return Vector2(x + v.x, y + v.y); }
        Vector2 operator-(const Vector2& v) const { return Vector2(x - v.x, y - v.y); }
        Vector2 operator*(Real s) const { return Vector2(x * s, y * s); }
        Vector2 operator/(Real s) const { return s != 0.0f ? Vector2(x / s, y / s) : Vector2(); }
        Vector2 operator*(const Vector2& v) const { return Vector2(x * v.x, y * v.y); }
        Vector2& operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }
        Vector2& operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }
        Real dotProduct(const Vector2& v) const { return x * v.x + y * v.y; }
        Real squaredLength() const { return x * x + y * y; }
        bool operator==(const Vector2& o) const { return x == o.x && y == o.y; }
        bool operator!=(const Vector2& o) const { return !(*this == o); }
        static const Vector2 ZERO;
    };

    class Vector3
    {
    public:
        Real x, y, z;
        Vector3() : x(0), y(0), z(0) {}
        explicit Vector3(Real v) : x(v), y(v), z(v) {}
        Vector3(Real _x, Real _y, Real _z) : x(_x), y(_y), z(_z) {}
        Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
        Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
        Vector3 operator-() const { return Vector3(-x, -y, -z); }
        Vector3 operator+(Real s) const { return Vector3(x + s, y + s, z + s); }
        Vector3 operator-(Real s) const { return Vector3(x - s, y - s, z - s); }
        Vector3 operator*(Real s) const { return Vector3(x * s, y * s, z * s); }
        Vector3 operator*(const Vector3& v) const { return Vector3(x * v.x, y * v.y, z * v.z); }
        Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
        Vector3& operator+=(Real s) { x += s; y += s; z += s; return *this; }
        Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
        Vector3& operator*=(Real s) { x *= s; y *= s; z *= s; return *this; }
        Vector3& operator/=(Real s) { if (s != 0.0f) { x /= s; y /= s; z /= s; } return *this; }
        Vector3 operator/(Real s) const { return s != 0.0f ? Vector3(x / s, y / s, z / s) : Vector3(); }
        Real operator[](size_t i) const { return i == 0 ? x : (i == 1 ? y : z); }
        Real& operator[](size_t i) { return i == 0 ? x : (i == 1 ? y : z); }
        bool operator==(const Vector3& o) const { return x == o.x && y == o.y && z == o.z; }
        bool operator!=(const Vector3& o) const { return !(*this == o); }
        Real dotProduct(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
        Quaternion getRotationTo(const Vector3& dest) const;
        Vector3 crossProduct(const Vector3& v) const
        {
            return Vector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
        }
        Real normalise()
        {
            Real l = length();
            if (l > 0.0f) {
                x /= l;
                y /= l;
                z /= l;
            }
            return l;
        }
        Real length() const { return std::sqrt(x * x + y * y + z * z); }
        Real squaredLength() const { return x * x + y * y + z * z; }
        Real distance(const Vector3& v) const { return (*this - v).length(); }
        Real squaredDistance(const Vector3& v) const { return (*this - v).squaredLength(); }
        Vector3 normalisedCopy() const { Real l = length(); return l > 0 ? Vector3(x / l, y / l, z / l) : *this; }
        void makeFloor(const Vector3& cmp)
        {
            x = x < cmp.x ? x : cmp.x;
            y = y < cmp.y ? y : cmp.y;
            z = z < cmp.z ? z : cmp.z;
        }
        void makeCeil(const Vector3& cmp)
        {
            x = x > cmp.x ? x : cmp.x;
            y = y > cmp.y ? y : cmp.y;
            z = z > cmp.z ? z : cmp.z;
        }
        static const Vector3 ZERO;
        static const Vector3 UNIT_X;
        static const Vector3 UNIT_Y;
        static const Vector3 UNIT_Z;
        static const Vector3 NEGATIVE_UNIT_X;
        static const Vector3 NEGATIVE_UNIT_Y;
        static const Vector3 NEGATIVE_UNIT_Z;
        static const Vector3 UNIT_SCALE;
    };

    class Matrix3
    {
    public:
        Real m[9];
        Matrix3()
        {
            for (int i = 0; i < 9; ++i) m[i] = (i % 4 == 0) ? 1.0f : 0.0f;
        }
        Matrix3 inverse() const { return Matrix3(); }
        Vector3 operator*(const Vector3& v) const { return v; }
        Matrix3 operator*(const Matrix3& rhs) const
        {
            (void)rhs;
            return *this;
        }
        const Real* operator[](size_t row) const { return &m[row * 3]; }
        Real* operator[](size_t row) { return &m[row * 3]; }
        void SetColumn(size_t i, const Vector3& v)
        {
            if (i > 2) return;
            m[i] = v.x;
            m[3 + i] = v.y;
            m[6 + i] = v.z;
        }
        void FromAxes(const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis)
        {
            SetColumn(0, xAxis);
            SetColumn(1, yAxis);
            SetColumn(2, zAxis);
        }
        static const Matrix3 IDENTITY;
    };

    class Vector4
    {
    public:
        Real x, y, z, w;
        Vector4() : x(0), y(0), z(0), w(1) {}
        Vector4(Real _x, Real _y, Real _z, Real _w) : x(_x), y(_y), z(_z), w(_w) {}
        Vector4 operator+(const Vector4& v) const { return Vector4(x + v.x, y + v.y, z + v.z, w + v.w); }
        Vector4 operator-(const Vector4& v) const { return Vector4(x - v.x, y - v.y, z - v.z, w - v.w); }
        Vector4 operator*(Real s) const { return Vector4(x * s, y * s, z * s, w * s); }
        Vector4& operator/=(Real s) { if (s != 0.0f) { x /= s; y /= s; z /= s; w /= s; } return *this; }
        Real dotProduct(const Vector4& v) const { return x * v.x + y * v.y + z * v.z + w * v.w; }
        static const Vector4 ZERO;
    };

    class Ray
    {
    public:
        Ray() : mOrigin(Vector3::ZERO), mDirection(Vector3::UNIT_Z) {}
        Ray(const Vector3& origin, const Vector3& direction) : mOrigin(origin), mDirection(direction) {}
        const Vector3& getOrigin() const { return mOrigin; }
        const Vector3& getDirection() const { return mDirection; }
        Vector3 getPoint(Real t) const { return mOrigin + mDirection * t; }
    private:
        Vector3 mOrigin;
        Vector3 mDirection;
    };

    class Quaternion
    {
    public:
        Real w, x, y, z;
        Quaternion() : w(1), x(0), y(0), z(0) {}
        Quaternion(Real _w, Real _x, Real _y, Real _z) : w(_w), x(_x), y(_y), z(_z) {}
        Quaternion(const Degree& angle, const Vector3& axis)
        {
            FromAngleAxis(static_cast<Radian>(angle), axis);
        }
        Vector3 operator*(const Vector3& v) const { return v; }
        Quaternion operator*(const Quaternion& q) const { (void)q; return *this; }
        Quaternion operator-() const { return Quaternion(-w, -x, -y, -z); }
        Real Dot(const Quaternion& q) const { return w * q.w + x * q.x + y * q.y + z * q.z; }
        Quaternion Inverse() const { return Quaternion(w, -x, -y, -z); }
        void ToAngleAxis(Radian& angle, Vector3& axis) const { angle = Radian(0.0f); axis = Vector3::UNIT_Y; }
        void FromAngleAxis(const Radian& angle, const Vector3& axis)
        {
            Real half = angle.valueRadians * 0.5f;
            Real s = std::sin(half);
            Real c = std::cos(half);
            Vector3 n = axis.normalisedCopy();
            w = c;
            x = n.x * s;
            y = n.y * s;
            z = n.z * s;
        }
        void ToRotationMatrix(Matrix3& m) const { (void)m; }
        void FromRotationMatrix(const Matrix3& m) { (void)m; }
        void FromAxes(const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis)
        {
            (void)xAxis;
            (void)yAxis;
            (void)zAxis;
            w = 1.0f;
            x = y = z = 0.0f;
        }
        static Quaternion Slerp(Real t, const Quaternion& p, const Quaternion& q)
        {
            (void)t;
            (void)p;
            return q;
        }
        static const Quaternion IDENTITY;
    };

    class Matrix4
    {
    public:
        Real m[16];
        Matrix4(Real m00, Real m01, Real m02, Real m03,
                Real m10, Real m11, Real m12, Real m13,
                Real m20, Real m21, Real m22, Real m23,
                Real m30, Real m31, Real m32, Real m33)
        {
            m[0] = m00; m[1] = m01; m[2] = m02; m[3] = m03;
            m[4] = m10; m[5] = m11; m[6] = m12; m[7] = m13;
            m[8] = m20; m[9] = m21; m[10] = m22; m[11] = m23;
            m[12] = m30; m[13] = m31; m[14] = m32; m[15] = m33;
        }
        Matrix4()
        {
            for (int i = 0; i < 16; ++i) m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        }
        Vector3 getTrans() const { return Vector3(m[3], m[7], m[11]); }
        Quaternion extractQuaternion() const { return Quaternion::IDENTITY; }
        Matrix4 inverse() const { return Matrix4(); }
        Matrix4& operator=(const Matrix3& mat)
        {
            for (int i = 0; i < 16; ++i) m[i] = 0.0f;
            m[0] = mat.m[0]; m[1] = mat.m[1]; m[2] = mat.m[2];
            m[4] = mat.m[3]; m[5] = mat.m[4]; m[6] = mat.m[5];
            m[8] = mat.m[6]; m[9] = mat.m[7]; m[10] = mat.m[8];
            m[15] = 1.0f;
            return *this;
        }
        void setTrans(const Vector3& v)
        {
            m[3] = v.x;
            m[7] = v.y;
            m[11] = v.z;
        }
        void makeTransform(const Vector3& position, const Vector3& scale, const Quaternion& orientation)
        {
            (void)scale;
            (void)orientation;
            for (int i = 0; i < 16; ++i) m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
            m[3] = position.x;
            m[7] = position.y;
            m[11] = position.z;
        }
        const Real* operator[](size_t row) const { return &m[row * 4]; }
        Real* operator[](size_t row) { return &m[row * 4]; }
        Vector4 operator*(const Vector4& v) const
        {
            return Vector4(
                m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w,
                m[4] * v.x + m[5] * v.y + m[6] * v.z + m[7] * v.w,
                m[8] * v.x + m[9] * v.y + m[10] * v.z + m[11] * v.w,
                m[12] * v.x + m[13] * v.y + m[14] * v.z + m[15] * v.w);
        }
        Vector3 operator*(const Vector3& v) const
        {
            return Vector3(
                m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3],
                m[4] * v.x + m[5] * v.y + m[6] * v.z + m[7],
                m[8] * v.x + m[9] * v.y + m[10] * v.z + m[11]);
        }
    };

    class ColourValue
    {
    public:
        Real r, g, b, a;
        ColourValue() : r(1), g(1), b(1), a(1) {}
        ColourValue(Real _r, Real _g, Real _b, Real _a = 1.0f) : r(_r), g(_g), b(_b), a(_a) {}
        ColourValue operator*(Real s) const { return ColourValue(r * s, g * s, b * s, a * s); }
        ColourValue operator*(const ColourValue& o) const { return ColourValue(r * o.r, g * o.g, b * o.b, a * o.a); }
        ColourValue operator-(const ColourValue& o) const { return ColourValue(r - o.r, g - o.g, b - o.b, a - o.a); }
        ColourValue& operator+=(const ColourValue& o) { r += o.r; g += o.g; b += o.b; a += o.a; return *this; }
        bool operator==(const ColourValue& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
        bool operator!=(const ColourValue& o) const { return !(*this == o); }
        void saturate()
        {
            r = r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r);
            g = g < 0.0f ? 0.0f : (g > 1.0f ? 1.0f : g);
            b = b < 0.0f ? 0.0f : (b > 1.0f ? 1.0f : b);
            a = a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a);
        }
        RGBA getAsRGBA() const
        {
            unsigned int rr = static_cast<unsigned int>(r * 255.0f) & 0xFF;
            unsigned int gg = static_cast<unsigned int>(g * 255.0f) & 0xFF;
            unsigned int bb = static_cast<unsigned int>(b * 255.0f) & 0xFF;
            unsigned int aa = static_cast<unsigned int>(a * 255.0f) & 0xFF;
            return (aa << 24) | (rr << 16) | (gg << 8) | bb;
        }
        RGBA getAsABGR() const
        {
            unsigned int rr = static_cast<unsigned int>(r * 255.0f) & 0xFF;
            unsigned int gg = static_cast<unsigned int>(g * 255.0f) & 0xFF;
            unsigned int bb = static_cast<unsigned int>(b * 255.0f) & 0xFF;
            unsigned int aa = static_cast<unsigned int>(a * 255.0f) & 0xFF;
            return (aa << 24) | (bb << 16) | (gg << 8) | rr;
        }
        void setAsRGBA(RGBA val)
        {
            a = static_cast<Real>((val >> 24) & 0xFF) / 255.0f;
            r = static_cast<Real>((val >> 16) & 0xFF) / 255.0f;
            g = static_cast<Real>((val >> 8) & 0xFF) / 255.0f;
            b = static_cast<Real>(val & 0xFF) / 255.0f;
        }
        static const ColourValue ZERO;
        static const ColourValue Black;
        static const ColourValue White;
        static const ColourValue Red;
        static const ColourValue Green;
        static const ColourValue Blue;
    };

    enum PolygonMode { PM_POINTS = 0, PM_WIREFRAME = 1, PM_SOLID = 2 };
    enum ProjectionType { PT_PERSPECTIVE = 0, PT_ORTHOGRAPHIC = 1 };
    enum SceneTypeMask { ST_GENERIC = 1 };
    enum FrameBufferType { FBT_COLOUR = 1, FBT_DEPTH = 2, FBT_STENCIL = 4 };

    class AxisAlignedBox
    {
    public:
        AxisAlignedBox() : mMinimum(Vector3::ZERO), mMaximum(Vector3::ZERO) {}
        AxisAlignedBox(const Vector3& minimum, const Vector3& maximum) : mMinimum(minimum), mMaximum(maximum) {}
        const Vector3& getMinimum() const { return mMinimum; }
        const Vector3& getMaximum() const { return mMaximum; }
        void setMinimum(const Vector3& v) { mMinimum = v; }
        void setMaximum(const Vector3& v) { mMaximum = v; }
        void merge(const AxisAlignedBox& other)
        {
            mMinimum.makeFloor(other.mMinimum);
            mMaximum.makeCeil(other.mMaximum);
        }
        void merge(const Vector3& v)
        {
            mMinimum.makeFloor(v);
            mMaximum.makeCeil(v);
        }
        void setInfinite() {}
    private:
        Vector3 mMinimum;
        Vector3 mMaximum;
    };

    class Camera;
    class Viewport;
    class RenderTargetListener;
    class Material;
    class Mesh;
    class RenderQueue;
    void drawManualObjects();
    class Entity;
    void registerMeshEntity(Entity* entity);
    void unregisterMeshEntity(Entity* entity);
    void drawMeshEntities();
    void bindEntityToMeshName(Entity* entity, const String& meshName);
    inline bool gSetupCalled = false;
    inline bool gAssetsLoaded = false;

    class SubEntity
    {
    public:
        SubEntity() {}
        std::shared_ptr<Material> getMaterial() const { return mMaterial; }
        void setMaterialName(const String& name) { (void)name; }
    private:
        std::shared_ptr<Material> mMaterial;
    };

    class RenderWindow
    {
    public:
        RenderWindow() : mWidth(640), mHeight(480), mClosed(false), mVsync(false), mFullscreen(false)
        {
#if defined(WII) || defined(__wii__)
            if (!sRendererInitialised) {
                WiiGX::Renderer& renderer = WiiGX::Renderer::getInstance();
                renderer.init();
                renderer.setClearColor(80, 100, 160, 255);
                renderer.enableDepthTest(true);
                renderer.enableDepthWrite(true);
                sRendererInitialised = true;
            }
#endif
        }
        bool isClosed() const { return mClosed; }
        void getMetrics(unsigned int& width, unsigned int& height, unsigned int& depth) { width = mWidth; height = mHeight; depth = 32; }
        void getMetrics(unsigned int& width, unsigned int& height, unsigned int& depth, int& left, int& top)
        { width = mWidth; height = mHeight; depth = 32; left = 0; top = 0; }
        unsigned int getWidth() const { return mWidth; }
        unsigned int getHeight() const { return mHeight; }
        void getCustomAttribute(const String& name, size_t* pData)
        {
            if (pData && name == "WINDOW") {
                *pData = 1;
            }
        }
        bool isVSyncEnabled() const { return mVsync; }
        bool isFullScreen() const { return mFullscreen; }
        void setVSyncEnabled(bool v) { mVsync = v; }
        void setVSyncInterval(unsigned int interval) { (void)interval; }
        void setFullscreen(bool v) { mFullscreen = v; }
        void setFullscreen(bool v, unsigned int width, unsigned int height)
        {
            mFullscreen = v;
            mWidth = width;
            mHeight = height;
        }
        void resize(unsigned int width, unsigned int height)
        {
            mWidth = width;
            mHeight = height;
        }
        Viewport* addViewport(Camera* cam, int zorder = 0, Real left = 0.0f, Real top = 0.0f, Real width = 1.0f, Real height = 1.0f);
        void removeViewport(int zorder) { (void)zorder; }
        void removeAllViewports();
        void update()
        {
#if defined(WII) || defined(__wii__)
            WiiGX::Renderer& renderer = WiiGX::Renderer::getInstance();
            renderer.beginFrame();
#if defined(WII_NATIVE_ASSET_PIPELINE)
            // Native terrain bring-up path renders terrain directly via
            // WiiGXRenderer::renderTerrainBuffer(). Skip legacy OGRE mesh and
            // manual-object passes to avoid conflicting GX state/output.
#else
            drawMeshEntities();
            drawManualObjects();
#endif

            renderer.endFrame();
#endif
        }
        void update(bool swapBuffers)
        {
            (void)swapBuffers;
            update();
        }
        void addListener(RenderTargetListener* listener) { (void)listener; }
        void removeListener(RenderTargetListener* listener) { (void)listener; }
        float getAverageFPS() const { return 60.0f; }
    private:
        unsigned int mWidth, mHeight;
        bool mClosed;
        bool mVsync;
        bool mFullscreen;
        inline static bool sRendererInitialised = false;
    };

    class SceneNode;
    class ParticleSystem;
    class ManualObject;
    void drawManualObjects();

    class MovableObject
    {
    public:
        class Listener { public: virtual ~Listener() {} };
        virtual ~MovableObject() {}
        bool isVisible() const { return mVisible; }
        void setVisible(bool visible) { mVisible = visible; }
        virtual Real getBoundingRadius() const { return 1.0f; }
        uint32 getLightMask() const { return mLightMask; }
        void setLightMask(uint32 mask) { mLightMask = mask; }
        SceneNode* getParentSceneNode() const { return mParentSceneNode; }
        void _notifyAttached(SceneNode* node) { mParentSceneNode = node; mParentNode = node; }
    protected:
        bool mVisible = true;
        uint8 mRenderQueueID = 0;
        uint32 mLightMask = 0xFFFFFFFFu;
        SceneNode* mParentSceneNode = 0;
        SceneNode* mParentNode = 0;
    };

    class Light;
    class ManualObject;
    class Entity : public MovableObject
    {
    public:
        explicit Entity(const String& n = "") : mName(n), mVisible(true) {}
        ~Entity() { unregisterMeshEntity(this); }
        bool isVisible() const { return mVisible; }
        void setVisible(bool v) { mVisible = v; }
        void setCastShadows(bool v) { (void)v; }
        void setMaterialName(const String& material) { (void)material; }
        std::shared_ptr<Mesh> getMesh() const { return mMesh; }
        void _setMesh(const std::shared_ptr<Mesh>& mesh)
        {
            mMesh = mesh;
            if (mMesh) {
                registerMeshEntity(this);
            } else {
                unregisterMeshEntity(this);
            }
        }
        void queryLights() {}
        void setBoundingBox(const AxisAlignedBox& box) { (void)box; }
        void setLightMask(uint32 mask) { MovableObject::setLightMask(mask); }
        void setListener(MovableObject::Listener* listener) { mListener = listener; }
        const String& getName() const { return mName; }
        SubEntity* getSubEntity(size_t index)
        {
            if (index >= mSubEntities.size()) {
                mSubEntities.resize(index + 1);
                for (size_t i = 0; i < mSubEntities.size(); ++i) {
                    if (!mSubEntities[i]) {
                        mSubEntities[i] = std::unique_ptr<SubEntity>(new SubEntity());
                    }
                }
            }
            return mSubEntities[index].get();
        }
    private:
        String mName;
        bool mVisible;
        std::shared_ptr<Mesh> mMesh;
        std::vector<std::unique_ptr<SubEntity> > mSubEntities;
        MovableObject::Listener* mListener = 0;
    };

    class SceneNode
    {
    public:
        explicit SceneNode(const String& n = "") : mName(n), mPos(), mScale(1, 1, 1), mOri() {}
        SceneNode* createChildSceneNode(const String& n = "") { mChildren.push_back(std::unique_ptr<SceneNode>(new SceneNode(n))); return mChildren.back().get(); }
        void attachObject(Entity* e) { if (e) e->_notifyAttached(this); }
        void attachObject(Light* l);
        void attachObject(ManualObject* m);
        void attachObject(ParticleSystem* p);
        void attachObject(MovableObject* m);
        void setPosition(const Vector3& v) { mPos = v; }
        void setPosition(Real x, Real y, Real z) { mPos = Vector3(x, y, z); }
        const Vector3& getPosition() const { return mPos; }
        void setScale(Real x, Real y, Real z) { mScale = Vector3(x, y, z); }
        void setScale(const Vector3& v) { mScale = v; }
        const Vector3& getScale() const { return mScale; }
        void setOrientation(const Quaternion& q) { mOri = q; }
        const Quaternion& getOrientation() const { return mOri; }
        const Vector3& _getDerivedPosition() const { return mPos; }
        const Vector3& _getDerivedScale() const { return mScale; }
        const Quaternion& _getDerivedOrientation() const { return mOri; }
        const String& getName() const { return mName; }
        SceneNode* getParentSceneNode() const { return mParent; }
        void removeAndDestroyChild(const String& name)
        {
            for (std::vector<std::unique_ptr<SceneNode> >::iterator it = mChildren.begin(); it != mChildren.end(); ++it) {
                if ((*it).get() && (*it)->getName() == name) {
                    mChildren.erase(it);
                    return;
                }
            }
        }
        void showBoundingBox(bool show) { (void)show; }
        void setVisible(bool visible) { mVisible = visible; }
        bool getVisible() const { return mVisible; }
    private:
        String mName;
        Vector3 mPos, mScale;
        Quaternion mOri;
        SceneNode* mParent = 0;
        bool mVisible = true;
        std::vector<std::unique_ptr<SceneNode> > mChildren;
    };

    enum LightTypes { LT_POINT, LT_DIRECTIONAL, LT_SPOTLIGHT };

    class Light : public MovableObject
    {
    public:
        static constexpr LightTypes LT_POINT = Ogre::LT_POINT;
        static constexpr LightTypes LT_DIRECTIONAL = Ogre::LT_DIRECTIONAL;
        static constexpr LightTypes LT_SPOTLIGHT = Ogre::LT_SPOTLIGHT;

        explicit Light(const String& n = "") : mName(n), mType(Ogre::LT_POINT) {}
        void setType(LightTypes t) { mType = t; }
        LightTypes getType() const { return mType; }
        void setPosition(const Vector3& p) { mPos = p; }
        void setPosition(Real x, Real y, Real z) { mPos = Vector3(x, y, z); }
        const Vector3& getPosition() const { return mPos; }
        const Vector3& getDerivedPosition() const { return mPos; }
        void setDirection(const Vector3& d) { mDir = d; }
        void setDiffuseColour(Real r, Real g, Real b) { mDiffuse = ColourValue(r, g, b, 1.0f); }
        void setSpecularColour(Real r, Real g, Real b) { mSpecular = ColourValue(r, g, b, 1.0f); }
        const ColourValue& getDiffuseColour() const { return mDiffuse; }
        void setAttenuation(Real range, Real constant, Real linear, Real quadratic)
        {
            mAttenuationRange = range;
            mAttenuationConstant = constant;
            (void)linear;
            (void)quadratic;
        }
        Real getAttenuationConstant() const { return mAttenuationConstant; }
        Real getAttenuationRange() const { return mAttenuationRange; }
        void setLightMask(uint32 mask) { mLightMask = mask; }
        uint32 getLightMask() const { return mLightMask; }
        void _calcTempSquareDist(const Vector3& position)
        {
            mTempSquareDist = (mPos - position).squaredLength();
            tempSquareDist = mTempSquareDist;
        }
        void _notifyIndexInFrame(size_t index) { mIndexInFrame = index; }
        void setSpotlightOuterAngle(const Degree& angle) { (void)angle; }
        void setSpotlightInnerAngle(const Degree& angle) { (void)angle; }
        const String& getName() const { return mName; }
        void setCastShadows(bool enabled) { (void)enabled; }
    private:
        String mName;
        LightTypes mType;
        Vector3 mPos, mDir;
        ColourValue mDiffuse = ColourValue::White;
        ColourValue mSpecular = ColourValue::White;
        Real mAttenuationConstant = 1.0f;
        Real mAttenuationRange = 1000.0f;
        uint32 mLightMask = 0xFFFFFFFFu;
        Real mTempSquareDist = 0.0f;
        size_t mIndexInFrame = 0;
    public:
        Real tempSquareDist = 0.0f;
    };

    class Camera
    {
    public:
        explicit Camera(const String& n = "") : mName(n), mPoly(PM_SOLID) {}
        void setPosition(Real x, Real y, Real z) { mPos = Vector3(x, y, z); }
        void setPosition(const Vector3& p) { mPos = p; }
        const Vector3& getPosition() const { return mPos; }
        void lookAt(const Vector3& t) { (void)t; }
        void setFOVy(const Degree& fovy) { (void)fovy; }
        void setProjectionType(ProjectionType t) { mProjectionType = t; }
        void setOrthoWindow(Real width, Real height) { (void)width; (void)height; }
        void setNearClipDistance(Real distance) { (void)distance; }
        void setFarClipDistance(Real distance) { (void)distance; }
        void setAspectRatio(Real ratio) { (void)ratio; }
        void setAutoTracking(bool enabled, SceneNode* target = 0)
        {
            (void)enabled;
            (void)target;
        }
        PolygonMode getPolygonMode() const { return mPoly; }
        void setPolygonMode(PolygonMode p) { mPoly = p; }
        void setOrientation(const Quaternion& q) { mOrientation = q; }
        const Quaternion& getOrientation() const { return mOrientation; }
        const Quaternion& getDerivedOrientation() const { static Quaternion q; return q; }
        bool isVisible(const AxisAlignedBox& box) const { (void)box; return true; }
        class SceneManager* getSceneManager() const { return mSceneMgr; }
        void _notifySceneManager(class SceneManager* sm) { mSceneMgr = sm; }
    private:
        String mName;
        Vector3 mPos;
        Quaternion mOrientation;
        PolygonMode mPoly;
        ProjectionType mProjectionType = PT_PERSPECTIVE;
        class SceneManager* mSceneMgr = 0;
    };

    class ParticleSystem : public MovableObject
    {
    public:
        class Emitter
        {
        public:
            void setColour(const ColourValue& c) { (void)c; }
            void setDirection(const Vector3& dir) { (void)dir; }
            void setEmissionRate(Real rate) { (void)rate; }
            void setAngle(const Degree& angle) { (void)angle; }
            void setTimeToLive(Real ttl) { (void)ttl; }
            void setParticleVelocity(Real vel) { (void)vel; }
            void setParameter(const String& name, const String& value) { (void)name; (void)value; }
        };
        class Affector
        {
        public:
            void setParameter(const String& name, const String& value) { (void)name; (void)value; }
        };

        explicit ParticleSystem(const String& n = "") : mName(n) {}
        void setMaterialName(const String& material) { (void)material; }
        void setEmitting(bool emitting) { mEmitting = emitting; }
        Emitter* getEmitter(size_t i)
        {
            if (i >= mEmitters.size()) mEmitters.resize(i + 1);
            return &mEmitters[i];
        }
        Affector* getAffector(size_t i)
        {
            if (i >= mAffectors.size()) mAffectors.resize(i + 1);
            return &mAffectors[i];
        }
    private:
        String mName;
        bool mEmitting = true;
        std::vector<Emitter> mEmitters;
        std::vector<Affector> mAffectors;
    };

    enum ManualOpType
    {
        ManualOpLineList = 1,
        ManualOpTriangleList = 2,
        ManualOpTriangleStrip = 3,
        ManualOpLineStrip = 4
    };

    struct ManualObjectData
    {
        std::vector<short> positions;
        std::vector<unsigned char> colors;
        std::vector<unsigned char> indices;
        int opType = ManualOpTriangleList;
        const ManualObject* src = nullptr;
    };

    extern std::vector<ManualObjectData> gManualObjects;

    class ManualObject : public MovableObject
    {
    public:
        explicit ManualObject(const String& n = "") : mName(n) {}
        void begin(const String& materialName, int opType)
        {
            (void)materialName;
            if (opType == 1)
                mOpType = ManualOpLineList;
            else if (opType == 4)
                mOpType = ManualOpLineStrip;
            else if (opType == 3)
                mOpType = ManualOpTriangleStrip;
            else
                mOpType = ManualOpTriangleList;
            mVertices.clear();
            mIndices.clear();
            mCurrentColour = ColourValue::White;
        }
        void position(const Vector3& v)
        {
            addVertex(v, mCurrentColour);
        }
        void position(Real x, Real y, Real z)
        {
            addVertex(Vector3(x, y, z), mCurrentColour);
        }
        void colour(const ColourValue& c)
        {
            mCurrentColour = c;
        }
        void colour(Real r, Real g, Real b, Real a)
        {
            mCurrentColour = ColourValue(r, g, b, a);
        }
        void textureCoord(Real u, Real v) { (void)u; (void)v; }
        void index(ushort i)
        {
            mIndices.push_back(i);
        }
        void end()
        {
#if defined(WII) || defined(__wii__)
            if (mVertices.empty()) return;

            const size_t vertCount = mVertices.size();
            if (vertCount == 0) return;

            ManualObjectData data;
            data.opType = mOpType;
            data.positions.reserve(vertCount * 3);
            data.colors.reserve(vertCount * 4);
            for (size_t i = 0; i < vertCount; ++i)
            {
                const ManualVertex& v = mVertices[i];
                const auto clamp01 = [](Ogre::Real v)
                {
                    if (v < 0.0f) return 0.0f;
                    if (v > 1.0f) return 1.0f;
                    return v;
                };
                data.positions.push_back(static_cast<short>(v.pos.x));
                data.positions.push_back(static_cast<short>(v.pos.y));
                data.positions.push_back(static_cast<short>(v.pos.z));
                data.colors.push_back(static_cast<unsigned char>(clamp01(v.colour.r) * 255.0f));
                data.colors.push_back(static_cast<unsigned char>(clamp01(v.colour.g) * 255.0f));
                data.colors.push_back(static_cast<unsigned char>(clamp01(v.colour.b) * 255.0f));
                data.colors.push_back(static_cast<unsigned char>(clamp01(v.colour.a) * 255.0f));
            }
            if (!mIndices.empty())
            {
                data.indices.reserve(mIndices.size());
                for (size_t i = 0; i < mIndices.size(); ++i)
                {
                    ushort idx = mIndices[i];
                    if (idx > 255)
                    {
                        idx = 0;
                    }
                    data.indices.push_back(static_cast<unsigned char>(idx));
                }
            }
            data.src = this;
            gManualObjects.push_back(std::move(data));
#endif
        }
        void setCastShadows(bool enabled) { (void)enabled; }
        void setVisible(bool v) { mVisible = v; }
        void setMaterialName(size_t subindex, const String& material) { (void)subindex; (void)material; }
        void setMaterialName(size_t subindex, const String& material, const String& group)
        {
            (void)subindex;
            (void)material;
            (void)group;
        }
        void setBoundingBox(const AxisAlignedBox& box) { (void)box; }
        void setRenderQueueGroup(uint8 group) { mRenderQueueID = group; }
    private:
        struct ManualVertex
        {
            Vector3 pos;
            ColourValue colour;
        };
        void addVertex(const Vector3& v, const ColourValue& c)
        {
            ManualVertex mv;
            mv.pos = v;
            mv.colour = c;
            mVertices.push_back(mv);
        }
        String mName;
        int mOpType = 0;
        ColourValue mCurrentColour = ColourValue::White;
        std::vector<ManualVertex> mVertices;
        std::vector<ushort> mIndices;
        bool mVisible = true;
    };

    inline void drawManualObjects()
    {
#if defined(WII) || defined(__wii__)
        for (size_t objIndex = 0; objIndex < gManualObjects.size(); ++objIndex)
        {
            const ManualObjectData& data = gManualObjects[objIndex];
            if (data.src && !data.src->isVisible())
            {
                continue;
            }
            const size_t vertCount = data.positions.size() / 3;
            if (vertCount == 0) continue;

            GX_SetArray(GX_VA_POS, const_cast<short*>(data.positions.data()), 3 * sizeof(short));
            GX_SetArray(GX_VA_CLR0, const_cast<unsigned char*>(data.colors.data()), 4 * sizeof(unsigned char));
            DCFlushRange(const_cast<short*>(data.positions.data()), data.positions.size() * sizeof(short));
            DCFlushRange(const_cast<unsigned char*>(data.colors.data()), data.colors.size() * sizeof(unsigned char));
            if (!data.indices.empty())
            {
                DCFlushRange(const_cast<unsigned char*>(data.indices.data()), data.indices.size() * sizeof(unsigned char));
            }

            Mtx view;
            Mtx modelView;
            guVector camera = {0.0f, 0.0f, 0.0f};
            guVector up = {0.0f, 1.0f, 0.0f};
            guVector look = {0.0f, 0.0f, -1.0f};
            guLookAt(view, &camera, &up, &look);

            guMtxIdentity(modelView);
            guMtxTransApply(modelView, modelView, 0.0f, 0.0f, -50.0f);
            guMtxConcat(view, modelView, modelView);
            GX_LoadPosMtxImm(modelView, GX_PNMTX0);
            GX_SetCurrentMtx(GX_PNMTX0);

            int gxType = GX_TRIANGLES;
            if (data.opType == ManualOpLineList || data.opType == ManualOpLineStrip)
            {
                gxType = GX_LINES;
            }
            else if (data.opType == ManualOpTriangleStrip)
            {
                gxType = GX_TRIANGLESTRIP;
            }

            if (!data.indices.empty())
            {
                GX_Begin(gxType, GX_VTXFMT0, data.indices.size());
                for (size_t i = 0; i < data.indices.size(); ++i)
                {
                    const unsigned char idx = data.indices[i];
                    GX_Position1x8(idx);
                    GX_Color1x8(idx);
                }
                GX_End();
            }
            else
            {
                GX_Begin(gxType, GX_VTXFMT0, vertCount);
                for (size_t i = 0; i < vertCount; ++i)
                {
                    GX_Position1x8(static_cast<unsigned char>(i));
                    GX_Color1x8(static_cast<unsigned char>(i));
                }
                GX_End();
            }
        }
#endif
    }

    class Viewport
    {
    public:
        Viewport() : mCamera(0), mOverlaysEnabled(true) {}
        explicit Viewport(Camera* camera) : mCamera(camera), mOverlaysEnabled(true) {}
        Camera* getCamera() const { return mCamera; }
        bool getOverlaysEnabled() const { return mOverlaysEnabled; }
        void setOverlaysEnabled(bool enabled) { mOverlaysEnabled = enabled; }
        int getActualWidth() const { return 640; }
        int getActualHeight() const { return 480; }
        void setBackgroundColour(const ColourValue& colour) { (void)colour; }
        void setClearEveryFrame(bool clear, unsigned int buffers = FBT_COLOUR | FBT_DEPTH)
        {
            (void)clear;
            (void)buffers;
        }
        void setSkiesEnabled(bool enabled) { (void)enabled; }
    private:
        Camera* mCamera;
        bool mOverlaysEnabled;
    };

    enum VertexElementType
    {
        VET_FLOAT2 = 0,
        VET_FLOAT3,
        VET_COLOUR
    };

    enum VertexElementSemantic
    {
        VES_POSITION = 0,
        VES_TEXTURE_COORDINATES,
        VES_DIFFUSE,
        VES_NORMAL,
        VES_TANGENT
    };

    class VertexElement
    {
    public:
        VertexElement(ushort source, size_t offset, VertexElementType type, VertexElementSemantic semantic, ushort index)
            : mSource(source), mOffset(offset), mType(type), mSemantic(semantic), mIndex(index) {}
        VertexElementSemantic getSemantic() const { return mSemantic; }
        ushort getSource() const { return mSource; }
        VertexElementType getType() const { return mType; }
        void baseVertexPointerToElement(void* base, float** pReal) const
        {
            if (!pReal) return;
            *pReal = reinterpret_cast<float*>(reinterpret_cast<unsigned char*>(base) + mOffset);
        }
        void baseVertexPointerToElement(const void* base, const float** pReal) const
        {
            if (!pReal) return;
            *pReal = reinterpret_cast<const float*>(reinterpret_cast<const unsigned char*>(base) + mOffset);
        }
        void baseVertexPointerToElement(void* base, RGBA** pRGBA) const
        {
            if (!pRGBA) return;
            *pRGBA = reinterpret_cast<RGBA*>(reinterpret_cast<unsigned char*>(base) + mOffset);
        }
        void baseVertexPointerToElement(const void* base, const RGBA** pRGBA) const
        {
            if (!pRGBA) return;
            *pRGBA = reinterpret_cast<const RGBA*>(reinterpret_cast<const unsigned char*>(base) + mOffset);
        }
        static size_t getTypeSize(VertexElementType type)
        {
            switch (type) {
                case VET_FLOAT2: return sizeof(float) * 2;
                case VET_FLOAT3: return sizeof(float) * 3;
                case VET_COLOUR: return sizeof(uint32);
                default: return 0;
            }
        }
    private:
        ushort mSource;
        size_t mOffset;
        VertexElementType mType;
        VertexElementSemantic mSemantic;
        ushort mIndex;
    };

    class VertexDeclaration
    {
    public:
        typedef std::vector<VertexElement> VertexElementList;

        const VertexElement& addElement(ushort source, size_t offset, VertexElementType type, VertexElementSemantic semantic, ushort index = 0)
        {
            mElements.push_back(VertexElement(source, offset, type, semantic, index));
            return mElements.back();
        }
        const VertexElement* findElementBySemantic(VertexElementSemantic semantic, ushort index = 0) const
        {
            (void)index;
            for (size_t i = 0; i < mElements.size(); ++i) {
                if (mElements[i].getSemantic() == semantic) {
                    return &mElements[i];
                }
            }
            return 0;
        }
        VertexElementList findElementsBySource(ushort source) const
        {
            VertexElementList out;
            for (size_t i = 0; i < mElements.size(); ++i) {
                if (mElements[i].getSource() == source) {
                    out.push_back(mElements[i]);
                }
            }
            return out;
        }
        size_t getVertexSize(ushort source) const
        {
            size_t total = 0;
            for (size_t i = 0; i < mElements.size(); ++i) {
                if (mElements[i].getSource() == source) {
                    total += VertexElement::getTypeSize(mElements[i].getType());
                }
            }
            return total;
        }
    private:
        std::vector<VertexElement> mElements;
    };

    class HardwareVertexBuffer
    {
    public:
        explicit HardwareVertexBuffer(size_t bytes = 0) : mData(bytes) {}
        void* lock(int mode)
        {
            (void)mode;
            return mData.empty() ? 0 : mData.data();
        }
        void unlock() {}
        const void* getPointer() const { return mData.empty() ? 0 : mData.data(); }
        size_t getVertexSize() const { return mVertexSize; }
        void setVertexSize(size_t size) { mVertexSize = size; }
        void setUsage(int usage) { mUsage = usage; }
        size_t getNumVertices() const { return mVertexSize ? (mData.size() / mVertexSize) : 0; }
        int getUsage() const { return mUsage; }
        bool hasShadowBuffer() const { return false; }
        size_t getSizeInBytes() const { return mData.size(); }
        void writeData(size_t offset, size_t length, const void* src, bool discardWholeBuffer)
        {
            (void)discardWholeBuffer;
            if (!src || offset + length > mData.size()) return;
            std::memcpy(mData.data() + offset, src, length);
        }
    private:
        std::vector<uchar> mData;
        size_t mVertexSize = 0;
        int mUsage = 0;
    };
    typedef std::shared_ptr<HardwareVertexBuffer> HardwareVertexBufferSharedPtr;

    class HardwareIndexBuffer
    {
    public:
        enum IndexType { IT_16BIT = 0, IT_32BIT = 1 };
        explicit HardwareIndexBuffer(IndexType type = IT_16BIT, size_t count = 0)
            : mType(type), mData(count * (type == IT_32BIT ? 4 : 2), 0) {}
        IndexType getType() const { return mType; }
        size_t getSizeInBytes() const { return mData.size(); }
        void* lock(int mode)
        {
            (void)mode;
            return mData.empty() ? 0 : mData.data();
        }
        const void* lock(int mode) const
        {
            (void)mode;
            return mData.empty() ? 0 : mData.data();
        }
        void unlock() {}
        void writeData(size_t offset, size_t length, const void* src, bool discardWholeBuffer)
        {
            (void)discardWholeBuffer;
            if (!src || offset + length > mData.size()) return;
            std::memcpy(mData.data() + offset, src, length);
        }
    private:
        IndexType mType;
        std::vector<unsigned char> mData;
    };
    typedef std::shared_ptr<HardwareIndexBuffer> HardwareIndexBufferSharedPtr;

    class IndexData
    {
    public:
        HardwareIndexBufferSharedPtr indexBuffer;
        size_t indexStart = 0;
        size_t indexCount = 0;
    };

    class VertexBufferBinding
    {
    public:
        void setBinding(ushort index, const HardwareVertexBufferSharedPtr& buffer) { mBindings[index] = buffer; }
        HardwareVertexBufferSharedPtr getBuffer(ushort index) const
        {
            std::map<ushort, HardwareVertexBufferSharedPtr>::const_iterator it = mBindings.find(index);
            if (it != mBindings.end()) {
                return it->second;
            }
            return HardwareVertexBufferSharedPtr();
        }
    private:
        std::map<ushort, HardwareVertexBufferSharedPtr> mBindings;
    };

    class VertexData
    {
    public:
        VertexData() : vertexStart(0), vertexCount(0), vertexDeclaration(new VertexDeclaration()), vertexBufferBinding(new VertexBufferBinding()) {}
        ~VertexData()
        {
            delete vertexDeclaration;
            delete vertexBufferBinding;
        }
        size_t vertexStart;
        size_t vertexCount;
        VertexDeclaration* vertexDeclaration;
        VertexBufferBinding* vertexBufferBinding;
    };

    class HardwareBufferManager
    {
    public:
        static HardwareBufferManager& getSingleton() { static HardwareBufferManager m; return m; }
        HardwareVertexBufferSharedPtr createVertexBuffer(size_t vertexSize, size_t numVerts, int usage)
        {
            HardwareVertexBufferSharedPtr buf = std::make_shared<HardwareVertexBuffer>(vertexSize * numVerts);
            buf->setVertexSize(vertexSize);
            buf->setUsage(usage);
            return buf;
        }
        HardwareVertexBufferSharedPtr createVertexBuffer(size_t vertexSize, size_t numVerts, int usage, bool useShadowBuffer)
        {
            (void)useShadowBuffer;
            HardwareVertexBufferSharedPtr buf = std::make_shared<HardwareVertexBuffer>(vertexSize * numVerts);
            buf->setVertexSize(vertexSize);
            buf->setUsage(usage);
            return buf;
        }
        HardwareIndexBufferSharedPtr createIndexBuffer(int type, size_t count, int usage)
        {
            (void)usage;
            return std::make_shared<HardwareIndexBuffer>(static_cast<HardwareIndexBuffer::IndexType>(type), count);
        }
    };

    class Renderable
    {
    public:
        class Visitor
        {
        public:
            virtual ~Visitor() {}
        };
        virtual ~Renderable() {}
    };

    class RenderQueue
    {
    public:
        void addRenderable(Renderable* renderable, uint8 id, ushort priority)
        {
            (void)renderable;
            (void)id;
            (void)priority;
        }
    };

    struct RenderOperation
    {
        enum OperationType { OT_LINE_LIST = 1, OT_TRIANGLE_LIST = 2, OT_TRIANGLE_STRIP = 3, OT_LINE_STRIP = 4 };
        RenderOperation() : vertexData(0), indexData(0), operationType(OT_TRIANGLE_LIST), useIndexes(false) {}
        VertexData* vertexData;
        void* indexData;
        OperationType operationType;
        bool useIndexes;
    };

    class SimpleSpline
    {
    public:
        void clear() { mPts.clear(); }
        void addPoint(const Vector3& p) { mPts.push_back(p); }
        Vector3 interpolate(Real t) const { (void)t; return mPts.empty() ? Vector3() : mPts[0]; }
        Vector3 interpolate(size_t fromIndex, Real t) const
        {
            (void)t;
            return fromIndex < mPts.size() ? mPts[fromIndex] : Vector3();
        }
    private:
        std::vector<Vector3> mPts;
    };

    typedef std::vector<Light*> LightList;

    class SceneManager
    {
    public:
        enum PrefabType { PT_SPHERE = 0 };
        enum IlluminationRenderStage { IRS_NONE = 0, IRS_RENDER_TO_TEXTURE = 1 };
        struct lightLess
        {
            bool operator()(const Light* a, const Light* b) const
            {
                return a < b;
            }
        };

        explicit SceneManager(const String& n = "") : mName(n), mRoot("root") {}
        virtual ~SceneManager() {}
        virtual Light* createLight(const String& name) { mLights[name] = std::unique_ptr<Light>(new Light(name)); return mLights[name].get(); }
        virtual Camera* createCamera(const String& name) { mCameras[name] = std::unique_ptr<Camera>(new Camera(name)); mCameras[name]->_notifySceneManager(this); return mCameras[name].get(); }
        virtual Entity* createEntity(const String& name) { mEntities[name] = std::unique_ptr<Entity>(new Entity(name)); return mEntities[name].get(); }
        virtual Entity* createEntity(const String& name, PrefabType type)
        {
            (void)type;
            return createEntity(name);
        }
        virtual Entity* createEntity(const String& name, const String& meshName, const String& groupName)
        {
            bindEntityToMeshName(0, meshName);
            (void)groupName;
            Entity* e = createEntity(name);
            bindEntityToMeshName(e, meshName);
            return e;
        }
        virtual Entity* createEntity(const String& name, const std::shared_ptr<Mesh>& mesh)
        {
            Entity* e = createEntity(name);
            if (e) {
                e->setVisible(true);
                e->_setMesh(mesh);
            }
            return e;
        }
        virtual Entity* createEntity(PrefabType type)
        {
            (void)type;
            static size_t sCounter = 0;
            return createEntity(String("Prefab") + std::to_string(++sCounter));
        }
        virtual SceneNode* createSceneNode(const String& name) { mNodes[name] = std::unique_ptr<SceneNode>(new SceneNode(name)); return mNodes[name].get(); }
        virtual ParticleSystem* createParticleSystem(const String& name, const String& tpl)
        {
            (void)tpl;
            return new ParticleSystem(name);
        }
        virtual ManualObject* createManualObject(const String& name)
        {
            mManuals[name] = std::unique_ptr<ManualObject>(new ManualObject(name));
            return mManuals[name].get();
        }
        virtual void destroyParticleSystem(const String& name)
        {
            (void)name;
        }
        virtual void destroyParticleSystem(ParticleSystem* ps)
        {
            delete ps;
        }
        virtual void destroyEntity(Entity* entity) { (void)entity; }
        virtual void addRenderQueueListener(void* listener) { (void)listener; }
        virtual void clearScene() {}
        virtual void setAmbientLight(const ColourValue& colour) { (void)colour; }
        virtual MovableObject* createMovableObject(const String& name, const String& typeName, const void* params = 0)
        {
            (void)params;
            if (typeName == "ParticleSystem") {
                return createParticleSystem(name, "");
            }
            return createEntity(name);
        }
        virtual void destroyEntity(const String& name)
        {
            mEntities.erase(name);
        }
        virtual void destroyAllCameras()
        {
            mCameras.clear();
        }
        virtual void setShadowTextureCount(size_t count) { mShadowTextureCount = count; }
        virtual void setShadowTextureSelfShadow(bool enabled) { (void)enabled; }
        virtual void setShadowTextureCasterMaterial(const String& mat) { (void)mat; }
        virtual void setShadowTexturePixelFormat(int fmt) { (void)fmt; }
        virtual void setShadowCasterRenderBackFaces(bool enabled) { (void)enabled; }
        virtual void setShadowTechnique(int technique) { (void)technique; }
        virtual void setShadowTextureSize(unsigned short size) { (void)size; }
        virtual void setShadowCameraSetup(const std::shared_ptr<void>& setup) { (void)setup; }
        virtual void setShadowCameraSetup(const SharedPtr<ShadowCameraSetup>& setup) { (void)setup; }
        virtual bool isShadowTechniqueTextureBased() const { return true; }
        virtual size_t getShadowTextureCount() const { return mShadowTextureCount; }
        virtual ulong _getLightsDirtyCounter() const { return 0; }
        virtual LightList _getLightsAffectingFrustum() const
        {
            LightList out;
            for (std::map<String, std::unique_ptr<Light> >::const_iterator it = mLights.begin(); it != mLights.end(); ++it) {
                out.push_back(it->second.get());
            }
            return out;
        }
        virtual void setFog(int mode, const ColourValue& colour, Real density, Real start, Real end)
        {
            (void)mode;
            (void)colour;
            (void)density;
            (void)start;
            (void)end;
        }
        SceneNode* getRootSceneNode() { return &mRoot; }
        int _getCurrentRenderStage() const { return IRS_NONE; }
        RenderQueue* getRenderQueue() { return &mRenderQueue; }
    protected:
        String mName;
        SceneNode mRoot;
        std::map<String, std::unique_ptr<Light> > mLights;
        std::map<String, std::unique_ptr<Camera> > mCameras;
        std::map<String, std::unique_ptr<Entity> > mEntities;
        std::map<String, std::unique_ptr<SceneNode> > mNodes;
        std::map<String, std::unique_ptr<ManualObject> > mManuals;
        RenderQueue mRenderQueue;
        size_t mShadowTextureCount = 1;
    };

    class DefaultSceneManager : public SceneManager
    {
    public:
        explicit DefaultSceneManager(const String& n = "") : SceneManager(n) {}
        virtual ~DefaultSceneManager() {}
        virtual ParticleSystem* createParticleSystem(const String& name, const String& tpl)
        { (void)tpl; return new ParticleSystem(name); }
    };

    class SceneManagerFactory
    {
    public:
        struct MetaData {
            String typeName;
            String description;
            uint32 sceneTypeMask;
            bool worldGeometrySupported;
        };
        virtual ~SceneManagerFactory() {}
    protected:
        mutable MetaData mMetaData;
    };

    class Root;

    inline Viewport* RenderWindow::addViewport(Camera* cam, int zorder, Real left, Real top, Real width, Real height)
    {
        (void)zorder;
        (void)left;
        (void)top;
        (void)width;
        (void)height;
        static Viewport vp(cam);
        return &vp;
    }
    inline void RenderWindow::removeAllViewports() {}
    inline const Vector3 Vector3::ZERO(0, 0, 0);
    inline const Vector2 Vector2::ZERO(0, 0);
    inline const Vector3 Vector3::UNIT_X(1, 0, 0);
    inline const Vector3 Vector3::UNIT_Y(0, 1, 0);
    inline const Vector3 Vector3::UNIT_Z(0, 0, 1);
    inline const Vector3 Vector3::NEGATIVE_UNIT_X(-1, 0, 0);
    inline const Vector3 Vector3::NEGATIVE_UNIT_Y(0, -1, 0);
    inline const Vector3 Vector3::NEGATIVE_UNIT_Z(0, 0, -1);
    inline const Vector3 Vector3::UNIT_SCALE(1, 1, 1);
    inline const Vector4 Vector4::ZERO(0, 0, 0, 0);
    inline const Matrix3 Matrix3::IDENTITY;
    inline Vector3 operator*(Real s, const Vector3& v) { return v * s; }
    inline Vector2 operator*(Real s, const Vector2& v) { return v * s; }
    inline Quaternion Vector3::getRotationTo(const Vector3& dest) const
    {
        Vector3 from = normalisedCopy();
        Vector3 to = dest.normalisedCopy();
        Real d = from.dotProduct(to);
        if (d >= 1.0f) {
            return Quaternion::IDENTITY;
        }
        if (d <= -1.0f) {
            return Quaternion(0.0f, 1.0f, 0.0f, 0.0f);
        }
        Vector3 c = from.crossProduct(to);
        Real s = std::sqrt((1.0f + d) * 2.0f);
        Real invs = 1.0f / s;
        return Quaternion(s * 0.5f, c.x * invs, c.y * invs, c.z * invs);
    }
    inline const Quaternion Quaternion::IDENTITY(1, 0, 0, 0);
    inline const ColourValue ColourValue::ZERO(0, 0, 0, 0);
    inline const ColourValue ColourValue::Black(0, 0, 0, 1);
    inline const ColourValue ColourValue::White(1, 1, 1, 1);
    inline const ColourValue ColourValue::Red(1, 0, 0, 1);
    inline const ColourValue ColourValue::Green(0, 1, 0, 1);
    inline const ColourValue ColourValue::Blue(0, 0, 1, 1);
    inline void SceneNode::attachObject(Light* l) { if (l) l->_notifyAttached(this); }
    inline void SceneNode::attachObject(ManualObject* m)
    {
        if (m) {
            m->_notifyAttached(this);
        }
    }
    inline void SceneNode::attachObject(ParticleSystem* p) { if (p) p->_notifyAttached(this); }
    inline void SceneNode::attachObject(MovableObject* m) { if (m) m->_notifyAttached(this); }
}

#endif
