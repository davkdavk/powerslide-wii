/*
 * OGRE Complete Stubs - Wii Port
 * Comprehensive OGRE API implementation for Wii
 */

#ifndef OGRE_COMPLETE_STUBS_H
#define OGRE_COMPLETE_STUBS_H

#include <string>
#include <vector>
#include <map>
#include <list>
#include <set>
#include <queue>
#include <memory>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <functional>

#include <gccore.h>
#include <ogc/gx.h>
#include <ogc/gu.h>

#define OGRE_NEW new
#define OGRE_DELETE delete

#define OGRE_EXCEPT(num, msg, src) throw std::runtime_error(msg)

namespace Ogre
{
    typedef int int32;
    typedef unsigned int uint32;
    typedef short int16;
    typedef unsigned short uint16;
    typedef char int8;
    typedef unsigned char uint8;
    typedef long long int64;
    typedef unsigned long long uint64;
    
    typedef float Real;
    typedef double RealD;
    
    typedef std::string String;
    typedef std::vector<String> StringVector;
    typedef std::vector<int> IntVector;
    typedef std::vector<uint32> UIntVector;
    
    // Forward declarations
    class Any;
    
    // Exception class
    class Exception : public std::exception
    {
    public:
        enum ErrorCode {
            ERR_FILE_NOT_FOUND = 1,
            ERR_INVALID_STATE = 2,
            ERR_ITEM_NOT_FOUND = 3,
            ERR_RENDERINGAPI_ERROR = 4,
            ERR_DUPLICATE_ITEM = 5
        };
        int number;
        String description;
        String source;
        String file;
        int line;
        
        Exception(int num, const String& msg, const String& src, const String& fl = "", int ln = 0)
            : number(num), description(msg), source(src), file(fl), line(ln) {}
        
        const char* what() const throw() { return description.c_str(); }
    };
    
    // Math classes
    class Radian
    {
    public:
        Real value;
        Radian(Real v = 0) : value(v) {}
        Radian(const Degree& d);
    };
    
    class Degree
    {
    public:
        Real value;
        Degree(Real v = 0) : value(v) {}
        Degree(const Radian& r);
    };
    
    inline Radian::Radian(const Degree& d) : value(d.value * 0.017453292519943295f) {}
    inline Degree::Degree(const Radian& r) : value(r.value * 57.29577951308232f) {}
    
    class Vector3
    {
    public:
        Real x, y, z;
        Vector3() : x(0), y(0), z(0) {}
        Vector3(Real _x, Real _y, Real _z) : x(_x), y(_y), z(_z) {}
        
        Vector3 operator+(const Vector3& v) const { return Vector3(x+v.x, y+v.y, z+v.z); }
        Vector3 operator-(const Vector3& v) const { return Vector3(x-v.x, y-v.y, z-v.z); }
        Vector3 operator*(Real s) const { return Vector3(x*s, y*s, z*s); }
        Vector3 operator/(Real s) const { return Vector3(x/s, y/s, z/s); }
        Vector3 operator-() const { return Vector3(-x, -y, -z); }
        
        Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
        Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
        Vector3& operator*=(Real s) { x *= s; y *= s; z *= s; return *this; }
        
        bool operator==(const Vector3& v) const { return x == v.x && y == v.y && z == v.z; }
        bool operator!=(const Vector3& v) const { return !(*this == v); }
        
        Real length() const { return sqrtf(x*x + y*y + z*z); }
        Real lengthSquared() const { return x*x + y*y + z*z; }
        Real distance(const Vector3& v) const { return (*this - v).length(); }
        
        Real dot(const Vector3& v) const { return x*v.x + y*v.y + z*v.z; }
        Vector3 cross(const Vector3& v) const { return Vector3(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x); }
        
        Vector3 normalize() const { Real l = length(); return l > 0 ? *this/l : *this; }
        Vector3 reflect(const Vector3& normal) const { return *this - normal * (2.0f * this->dot(normal)); }
        
        static const Vector3 ZERO;
        static const Vector3 UNIT_X;
        static const Vector3 UNIT_Y;
        static const Vector3 UNIT_Z;
        static const Vector3 NEGATIVE_UNIT_X;
        static const Vector3 NEGATIVE_UNIT_Y;
        static const Vector3 NEGATIVE_UNIT_Z;
    };
    
    class Vector4
    {
    public:
        Real x, y, z, w;
        Vector4() : x(0), y(0), z(0), w(1) {}
        Vector4(Real _x, Real _y, Real _z, Real _w) : x(_x), y(_y), z(_z), w(_w) {}
        Vector4(const Vector3& v, Real _w = 1.0f) : x(v.x), y(v.y), z(v.z), w(_w) {}
    };
    
    class Quaternion
    {
    public:
        Real w, x, y, z;
        Quaternion() : w(1), x(0), y(0), z(0) {}
        Quaternion(Real _w, Real _x, Real _y, Real _z) : w(_w), x(_x), y(_y), z(_z) {}
        
        Quaternion operator*(const Quaternion& q) const {
            return Quaternion(
                w*q.w - x*q.x - y*q.y - z*q.z,
                w*q.x + x*q.w + y*q.z - z*q.y,
                w*q.y - x*q.z + y*q.w + z*q.w,
                w*q.z + x*q.y - y*q.x + z*q.w
            );
        }
        
        Vector3 operator*(const Vector3& v) const {
            Vector3 qv(x, y, z);
            Vector3 uv = qv.cross(v);
            Vector3 uuv = qv.cross(uv);
            return v + (uv * w + uuv) * 2.0f;
        }
        
        void fromAxisAngle(const Vector3& axis, Real angle) {
            Real half = angle * 0.5f;
            Real s = sinf(half);
            w = cosf(half);
            x = axis.x * s; y = axis.y * s; z = axis.z * s;
        }
        
        void fromAxes(const Vector3& xaxis, const Vector3& yaxis, const Vector3& zaxis);
        void toAxes(Vector3& xaxis, Vector3& yaxis, Vector3& zaxis) const;
        
        Real dot(const Quaternion& q) const { return w*q.w + x*q.x + y*q.y + z*q.z; }
        Real length() const { return sqrtf(w*w + x*x + y*y + z*z); }
        Quaternion normalize() const { Real l = length(); return Quaternion(w/l, x/l, y/l, z/l); }
        Quaternion inverse() const { Real n = w*w + x*x + y*y + z*z; return Quaternion(w/n, -x/n, -y/n, -z/n); }
        
        static const Quaternion IDENTITY;
    };
    
    class Matrix3
    {
    public:
        Real m[9];
        Matrix3() { memset(m, 0, sizeof(m)); m[0]=m[4]=m[8]=1; }
        
        Real operator()(size_t row, size_t col) const { return m[row * 3 + col]; }
        Real& operator()(size_t row, size_t col) { return m[row * 3 + col]; }
        
        Matrix3 operator*(const Matrix3& m2) const;
        Vector3 operator*(const Vector3& v) const;
    };
    
    class Matrix4
    {
    public:
        Real m[16];
        Matrix4() { identity(); }
        
        void identity() { memset(m, 0, sizeof(m)); m[0]=m[5]=m[10]=m[15]=1.0f; }
        
        Real operator()(size_t row, size_t col) const { return m[row * 4 + col]; }
        Real& operator()(size_t row, size_t col) { return m[row * 4 + col]; }
        Real operator[](size_t i) const { return m[i]; }
        Real& operator[](size_t i) { return m[i]; }
        
        Matrix4 operator*(const Matrix4& m2) const;
        Vector3 operator*(const Vector3& v) const;
        Vector4 operator*(const Vector4& v) const;
        
        Matrix4 inverse() const;
        Matrix4 transpose() const;
        
        void translation(const Vector3& v);
        void scaling(const Vector3& v);
        void rotation(const Quaternion& q);
        void rotationX(Real angle);
        void rotationY(Real angle);
        void rotationZ(Real angle);
        
        static const Matrix4 ZERO;
        static const Matrix4 IDENTITIY;
    };
    
    class ColourValue
    {
    public:
        Real r, g, b, a;
        ColourValue() : r(1), g(1), b(1), a(1) {}
        ColourValue(Real _r, Real _g, Real _b, Real _a = 1) : r(_r), g(_g), b(_b), a(_a) {}
        
        ColourValue operator*(Real s) const { return ColourValue(r*s, g*s, b*s, a*s); }
        
        bool operator==(const ColourValue& c) const { return r==c.r && g==c.g && b==c.b && a==c.a; }
        
        static const ColourValue ZERO;
        static const ColourValue Black;
        static const ColourValue White;
        static const ColourValue Red;
        static const ColourValue Green;
        static const ColourValue Blue;
    };
    
    class AxisAlignedBox
    {
    public:
        enum Extent { EXTENT_NULL, EXTENT_FINITE, EXTENT_INFINITE };
        
    private:
        Vector3 mMinimum;
        Vector3 mMaximum;
        Extent mExtent;
        
    public:
        AxisAlignedBox() : mExtent(EXTENT_NULL) {}
        AxisAlignedBox(const Vector3& min, const Vector3& max) : mMinimum(min), mMaximum(max), mExtent(EXTENT_FINITE) {}
        
        void setMinimum(const Vector3& v) { mMinimum = v; mExtent = EXTENT_FINITE; }
        void setMaximum(const Vector3& v) { mMaximum = v; mExtent = EXTENT_FINITE; }
        void setInfinite() { mExtent = EXTENT_INFINITE; }
        
        const Vector3& getMinimum() const { return mMinimum; }
        const Vector3& getMaximum() const { return mMaximum; }
        
        Vector3 getCenter() const { return (mMinimum + mMaximum) * 0.5f; }
        Vector3 getSize() const { return mMaximum - mMinimum; }
        
        bool intersects(const AxisAlignedBox& box) const;
        bool contains(const Vector3& point) const;
    };
    
    class Sphere
    {
    public:
        Vector3 center;
        Real radius;
        Sphere() : radius(0) {}
        Sphere(const Vector3& c, Real r) : center(c), radius(r) {}
    };
    
    class Plane
    {
    public:
        Vector3 normal;
        Real d;
        Plane() : d(0) {}
        Plane(const Vector3& n, Real d) : normal(n), d(d) {}
        Plane(const Vector3& p1, const Vector3& p2, const Vector3& p3);
        
        Real getDistance(const Vector3& p) const { return normal.dot(p) + d; }
    };
    
    class Ray
    {
    public:
        Vector3 origin;
        Vector3 direction;
        Ray() {}
        Ray(const Vector3& o, const Vector3& d) : origin(o), direction(d) {}
        
        std::pair<bool, Real> intersects(const Sphere& s) const;
        std::pair<bool, Real> intersects(const Plane& p) const;
    };
    
    // String utilities
    class StringUtil
    {
    public:
        static String trim(const String& str);
        static String toLowerCase(const String& str);
        static String toUpperCase(const String& str);
        static bool startsWith(const String& str, const String& pattern);
        static bool endsWith(const String& str, const String& pattern);
        static String replaceAll(const String& str, const String& replace, const String& with);
        static String format(const char* fmt, ...);
        
        static const String BLANK;
    };
    
    // Log manager
    enum LogMessageLevel {
        LL_TRIVIAL = 0,
        LL_NORMAL = 1,
        LL_CRITICAL = 2
    };
    
    class Log
    {
    public:
        void logMessage(const String& msg, LogMessageLevel lml = LL_NORMAL);
        void logMessage(const char* msg, LogMessageLevel lml = LL_NORMAL);
    };
    
    class LogManager
    {
    public:
        static LogManager& getSingleton();
        static LogManager* getSingletonPtr();
        
        Log* createLog(const String& name, bool defaultLog = false);
        void setLogDetail(int detail);
        Log* getDefaultLog();
    };
    
    // Any class for variant types
    class Any
    {
    public:
        Any() : content(NULL) {}
        
        template<typename T>
        Any(const T& value) : content(new holder<T>(value)) {}
        
        Any(const Any& other) : content(other.content ? other.content->clone() : NULL) {}
        
        ~Any() { if (content) delete content; }
        
        Any& operator=(const Any& other) {
            if (this != &other) {
                if (content) delete content;
                content = other.content ? other.content->clone() : NULL;
            }
            return *this;
        }
        
        template<typename T>
        T* get() const {
            holder<T>* h = dynamic_cast<holder<T>*>(content);
            return h ? &h->value : NULL;
        }
        
        template<typename T>
        bool is() const {
            return dynamic_cast<holder<T>*>(content) != NULL;
        }
        
        bool isEmpty() const { return !content; }
        
    private:
        struct placeholder {
            virtual ~placeholder() {}
            virtual placeholder* clone() const = 0;
        };
        
        template<typename T>
        struct holder : placeholder {
            T value;
            holder(const T& v) : value(v) {}
            placeholder* clone() const { return new holder<T>(value); }
        };
        
        placeholder* content;
    };
    
    // Frame listener
    class FrameEvent
    {
    public:
        Real timeSinceLastEvent;
        Real timeSinceLastFrame;
    };
    
    class FrameListener
    {
    public:
        virtual ~FrameListener() {}
        virtual bool frameStarted(const FrameEvent& evt) { return true; }
        virtual bool frameEnded(const FrameEvent& evt) { return true; }
        virtual bool frameRenderingQueued(const FrameEvent& evt) { return true; }
    };
    
    class WindowEventListener
    {
    public:
        virtual ~WindowEventListener() {}
        virtual void windowResized(uint32_t width, uint32_t height) {}
        virtual void windowClosed() {}
    };
    
    // Resource group manager
    class ResourceGroupManager
    {
    public:
        static ResourceGroupManager& getSingleton();
        
        void addResourceLocation(const String& name, const String& locType, const String& group = "General");
        void initialiseAllResourceGroups();
        void loadResourceGroup(const String& group);
        void unloadResourceGroup(const String& group);
        
        static const String DEFAULT_RESOURCE_GROUP_NAME;
    };
    
    // Timer
    class Timer
    {
    public:
        Timer();
        ~Timer();
        
        void reset();
        unsigned long getMilliseconds() const;
        unsigned long getMicroseconds() const;
        unsigned long getMillisecondsCPU() const;
        unsigned long getMicrosecondsCPU() const;
    };
    
    // MovableObject base
    class MovableObject
    {
    public:
        MovableObject() : mParentNode(NULL) {}
        virtual ~MovableObject() {}
        
        virtual const String& getName() const { return mName; }
        virtual void setName(const String& name) { mName = name; }
        
        virtual void setVisible(bool visible) { mVisible = visible; }
        virtual bool isVisible() const { return mVisible; }
        
        virtual void setParent(Sentient* p) { mParent = p; }
        
    protected:
        String mName;
        bool mVisible;
        void* mParentNode;
        void* mParent;
    };
    typedef MovableObject Sentient;
    
    // Renderable
    class Renderable
    {
    public:
        virtual ~Renderable() {}
        virtual void setMaterial(void* mat) {}
    };
    
    // Node attachment
    class Node
    {
    public:
        Node() : mParent(NULL), mScale(1,1,1) {}
        virtual ~Node() {}
        
        virtual void setPosition(const Vector3& pos) { mPosition = pos; }
        virtual void setOrientation(const Quaternion& q) { mOrientation = q; }
        virtual void setScale(const Vector3& scale) { mScale = scale; }
        
        virtual const Vector3& getPosition() const { return mPosition; }
        virtual const Quaternion& getOrientation() const { return mOrientation; }
        virtual const Vector3& getScale() const { return mScale; }
        
        virtual void translate(const Vector3& d, TransformSpace ts = TS_LOCAL) {}
        virtual void rotate(const Quaternion& q, TransformSpace ts = TS_LOCAL) {}
        virtual void scale(const Vector3& scale) { mScale = mScale * scale; }
        
        virtual void addChild(Node* child) {}
        virtual void removeChild(Node* child) {}
        virtual Node* getParent() const { return mParent; }
        
        virtual const Matrix4& _getFullTransform() const { return mCachedTransform; }
        
        enum TransformSpace { TS_LOCAL, TS_PARENT, TS_WORLD };
        
    protected:
        Node* mParent;
        Vector3 mPosition;
        Quaternion mOrientation;
        Vector3 mScale;
        mutable Matrix4 mCachedTransform;
    };
    
    // SceneNode
    class SceneNode : public Node
    {
    public:
        SceneNode(const String& name);
        SceneNode();
        virtual ~SceneNode();
        
        virtual void attachObject(MovableObject* obj);
        virtual MovableObject* detachObject(const String& name);
        virtual void detachAllObjects();
        
        virtual void setVisible(bool visible, bool cascade = true);
        virtual bool isVisible() const { return mVisible; }
        
        virtual void show() { setVisible(true); }
        virtual void hide() { setVisible(false); }
        
        virtual void setInheritanceEnabled(bool inherit) {}
        virtual bool getInheritScale() const { return true; }
        
        virtual const Quaternion& getWorldOrientation() const;
        virtual const Vector3& getWorldPosition() const;
        virtual Matrix4 getWorldTransforms() const;
        
    protected:
        String mName;
        bool mVisible;
    };
    
    // Light types
    enum LightTypes
    {
        LT_POINT,
        LT_SPOTLIGHT,
        LT_DIRECTIONAL
    };
    
    // Light
    class Light : public MovableObject
    {
    public:
        Light(const String& name);
        virtual ~Light();
        
        void setType(LightTypes type) { mLightType = type; }
        LightTypes getType() const { return mLightType; }
        
        void setDiffuseColour(Real r, Real g, Real b) { mDiffuse = ColourValue(r, g, b); }
        void setSpecularColour(Real r, Real g, Real b) { mSpecular = ColourValue(r, g, b); }
        void setAmbientColour(Real r, Real g, Real b) { mAmbient = ColourValue(r, g, b); }
        
        const ColourValue& getDiffuseColour() const { return mDiffuse; }
        const ColourValue& getSpecularColour() const { return mSpecular; }
        
        void setDirection(const Vector3& dir) { mDirection = dir; }
        void setPosition(const Vector3& pos) { mPosition = pos; }
        
        void setSpotlightRange(Real innerAngle, Real outerAngle, Real falloff) {}
        void setAttenuation(Real range, Real constant, Real linear, Real quadratic) {}
        
        void setPowerScale(Real power) { mPowerScale = power; }
        
        Real getDistance(const Vector3& objectPosition) const;
        
    protected:
        LightTypes mLightType;
        ColourValue mDiffuse;
        ColourValue mSpecular;
        ColourValue mAmbient;
        Vector3 mDirection;
        Vector3 mPosition;
        Real mPowerScale;
    };
    
    // Entity
    class Entity : public MovableObject, public Renderable
    {
    public:
        Entity(const String& name);
        Entity(const String& name, const String& meshName);
        virtual ~Entity();
        
        void setMaterialName(const String& matName);
        
        virtual void setVisible(bool visible);
        
        virtual void setCastShadows(bool enabled) {}
        virtual bool getCastShadows() const { return mCastShadows; }
        
        virtual void setQueryFlags(uint32 flags) {}
        virtual uint32 getQueryFlags() const { return 0; }
        
    protected:
        bool mCastShadows;
        bool mVisible;
    };
    
    // Camera
    class Camera : public MovableObject
    {
    public:
        Camera(const String& name);
        virtual ~Camera();
        
        void setPosition(Real x, Real y, Real z);
        void setPosition(const Vector3& pos);
        
        void setOrientation(const Quaternion& q);
        
        void lookAt(const Vector3& target);
        void lookAt(Real x, Real y, Real z);
        
        void setProjectionType(int type) {}
        
        void setFOVy(const Radian& fovy);
        Radian getFOVy() const { return mFOVy; }
        
        void setAspectRatio(Real ratio);
        Real getAspectRatio() const { return mAspect; }
        
        void setNearClipDistance(Real nearDist);
        void setFarClipDistance(Real farDist);
        
        Real getNearClipDistance() const { return mNear; }
        Real getFarClipDistance() const { return mFar; }
        
        void setAutoAspectRatio(bool autoAspect) { mAutoAspect = autoAspect; }
        
        virtual void setVisible(bool visible);
        
        const Vector3& getPosition() const { return mPosition; }
        const Quaternion& getOrientation() const { return mOrientation; }
        
        Vector3 getDirection() const;
        Vector3 getUp() const;
        Vector3 getRight() const;
        
        void setPolygonMode(PolygonMode md) { mPolyMode = md; }
        PolygonMode getPolygonMode() const { return mPolyMode; }
        
        enum PolygonMode { PM_POINTS = 0, PM_WIREFRAME = 1, PM_SOLID = 2 };
        
    protected:
        Vector3 mPosition;
        Quaternion mOrientation;
        Radian mFOVy;
        Real mAspect;
        Real mNear;
        Real mFar;
        bool mAutoAspect;
        PolygonMode mPolyMode;
    };
    
    // Viewport
    class Viewport
    {
    public:
        Viewport(Camera* cam, int left, int top, int width, int height, int ZOrder = 0);
        virtual ~Viewport();
        
        Camera* getCamera() const { return mCamera; }
        
        void setBackgroundColour(const ColourValue& colour);
        const ColourValue& getBackgroundColour() const { return mBgColour; }
        
        void setClearEveryFrame(bool enabled) { mClearEveryFrame = enabled; }
        
        void setDimensions(Real left, Real top, Real width, Real height);
        
        void setOverlaysEnabled(bool enabled) { mOverlays = enabled; }
        bool getOverlaysEnabled() const { return mOverlays; }
        
        int getActualLeft() const { return mLeft; }
        int getActualTop() const { return mTop; }
        int getActualWidth() const { return mWidth; }
        int getActualHeight() const { return mHeight; }
        
    protected:
        Camera* mCamera;
        int mLeft, mTop, mWidth, mHeight;
        int mZOrder;
        ColourValue mBgColour;
        bool mClearEveryFrame;
        bool mOverlays;
    };
    
    // RenderWindow
    class RenderWindow
    {
    public:
        RenderWindow();
        virtual ~RenderWindow();
        
        virtual void create(const String& name, int width, int height, bool fullscreen);
        virtual void destroy() {}
        
        virtual void resize(uint32 width, uint32 height);
        
        virtual void setFullscreen(bool fullscreen, uint32 width, uint32 height) {}
        
        virtual bool isClosed() const { return mClosed; }
        virtual bool isActive() const { return true; }
        virtual bool isVisible() const { return true; }
        
        virtual void setVSyncEnabled(bool enabled) {}
        virtual bool isVSyncEnabled() const { return false; }
        
        virtual void setTitle(const String& title) {}
        
        uint32 getWidth() const { return mWidth; }
        uint32 getHeight() const { return mHeight; }
        
        Viewport* addViewport(Camera* cam, int ZOrder = 0, float left = 0, float top = 0, float width = 1, float height = 1);
        void removeViewport(int ZOrder);
        void removeAllViewports();
        
        virtual void swapBuffers() {}
        
    protected:
        String mName;
        uint32 mWidth;
        uint32 mHeight;
        bool mFullscreen;
        bool mClosed;
        typedef std::map<int, Viewport*> ViewportList;
        ViewportList mViewports;
    };
    
    // SceneManager
    class SceneManager : public FrameListener
    {
    public:
        SceneManager();
        virtual ~SceneManager();
        
        virtual const String& getTypeName() const { return mTypeName; }
        
        virtual Camera* createCamera(const String& name);
        virtual Camera* createCamera(const String& name, const String& guid);
        virtual void destroyCamera(const String& name);
        virtual void destroyAllCameras();
        Camera* getCamera(const String& name) const;
        
        virtual SceneNode* createSceneNode(const String& name);
        virtual SceneNode* createSceneNode();
        virtual void destroySceneNode(const String& name);
        virtual void destroyAllSceneNodes();
        SceneNode* getSceneNode(const String& name) const;
        
        virtual Entity* createEntity(const String& name, const String& meshName);
        virtual Entity* createEntity(const String& name, int meshType);
        virtual void destroyEntity(const String& name);
        virtual void destroyAllEntities();
        
        virtual Light* createLight(const String& name);
        virtual Light* createLight(const String& name, int type);
        virtual void destroyLight(const String& name);
        virtual void destroyAllLights();
        
        virtual void setAmbientLight(const ColourValue& colour);
        virtual const ColourValue& getAmbientLight() const { return mAmbientLight; }
        
        virtual void setSkyBox(bool enable, const String& materialName, float distance = 5000, bool drawFirst = true) {}
        virtual void setSkyDome(bool enable, const String& materialName, int curvature = 10, float tiling = 8, float distance = 4000) {}
        
        virtual void setFog(FogMode mode = FOG_NONE, const ColourValue& colour = ColourValue(), float density = 0.001, float start = 0, float end = 1) {}
        virtual void setFog(FogMode mode, const ColourValue& colour, float density) {}
        virtual FogMode getFogMode() const { return FOG_NONE; }
        virtual const ColourValue& getFogColour() const { return mFogColor; }
        virtual float getFogDensity() const { return 0; }
        
        virtual void clearScene() {}
        
        virtual void _updateSceneGraph(Camera* cam) {}
        
        virtual RenderWindow* getRenderWindow() { return mWindow; }
        virtual void setRenderWindow(RenderWindow* win) { mWindow = win; }
        
        virtual void addFrameListener(FrameListener* l);
        virtual void removeFrameListener(FrameListener* l);
        
        virtual bool frameStarted(const FrameEvent& evt);
        virtual bool frameEnded(const FrameEvent& evt);
        
        enum FogMode { FOG_NONE, FOG_EXP, FOG_EXP2, FOG_LINEAR };
        
    protected:
        String mTypeName;
        RenderWindow* mWindow;
        
        typedef std::map<String, Camera*> CameraMap;
        CameraMap mCameras;
        
        typedef std::map<String, SceneNode*> SceneNodeMap;
        SceneNodeMap mSceneNodes;
        
        typedef std::map<String, Entity*> EntityMap;
        EntityMap mEntities;
        
        typedef std::map<String, Light*> LightMap;
        LightMap mLights;
        
        ColourValue mAmbientLight;
        ColourValue mFogColor;
        
        std::vector<FrameListener*> mFrameListeners;
    };
    
    // Root
    class Root
    {
    public:
        Root();
        Root(const String& pluginFileName);
        ~Root();
        
        static Root* getSingletonPtr();
        static Root& getSingleton();
        
        SceneManager* createSceneManager(const String& typeName);
        
        RenderWindow* initialise(bool autoCreateWindow = true, const String& windowTitle = "OGRE Render");
        RenderWindow* initialise(const RenderWindow& window, const String& windowTitle = "");
        
        void startRendering();
        void renderOneFrame();
        bool renderOneFrame(Real timeSinceLastFrame);
        
        void shutdown();
        
        SceneManager* getSceneManager(const String& name) const;
        SceneManager* getSceneManager(size_t index) const;
        size_t getNumSceneManagers() const;
        
        void addFrameListener(FrameListener* l);
        void removeFrameListener(FrameListener* l);
        
        void setDebuggerConnected(bool connected) {}
        
    protected:
        static Root* mSingleton;
        
        bool mInitialized;
        RenderWindow* mWindow;
        
        typedef std::vector<SceneManager*> SceneManagerList;
        SceneManagerList mSceneManagers;
        
        std::vector<FrameListener*> mFrameListeners;
    };
    
    // Config dialog
    class ConfigDialog
    {
    public:
        ConfigDialog() {}
        virtual ~ConfigDialog() {}
        
        virtual bool display() { return true; }
    };
    
    // Overlay
    class Overlay
    {
    public:
        Overlay(const String& name);
        virtual ~Overlay();
        
        virtual void show();
        virtual void hide();
        virtual bool isVisible() const { return mVisible; }
        
        virtual void add2D(OverlayElement* element) {}
        virtual void remove2D(OverlayElement* element) {}
        
        virtual void setZOrder(int z) { mZOrder = z; }
        virtual int getZOrder() const { return mZOrder; }
        
    protected:
        String mName;
        bool mVisible;
        int mZOrder;
    };
    
    // OverlayElement
    class OverlayElement
    {
    public:
        OverlayElement(const String& name);
        virtual ~OverlayElement();
        
        virtual void setPosition(Real left, Real top);
        virtual void setDimensions(Real width, Real height);
        
        virtual void setCaption(const String& text);
        virtual void setMaterialName(const String& matName);
        
        virtual void show();
        virtual void hide();
        
        virtual bool isVisible() const { return mVisible; }
        
    protected:
        String mName;
        bool mVisible;
    };
    
    class OverlayManager
    {
    public:
        static OverlayManager& getSingleton();
        
        Overlay* create(const String& name);
        void destroy(const String& name);
        Overlay* getByName(const String& name) const;
    };
    
    // Material
    class Material : public std::enable_shared_from_this<Material>
    {
    public:
        typedef std::shared_ptr<Material> Ptr;
        
        Material(const String& name);
        virtual ~Material();
        
        static MaterialPtr create(const String& name);
        
        void setDiffuse(Real r, Real g, Real b, Real a = 1.0f);
        void setSpecular(Real r, Real g, Real b, Real a = 1.0f);
        void setAmbient(Real r, Real g, Real b);
        void setEmissive(Real r, Real g, Real b);
        
        void setShininess(Real val);
        void setAlpha(Real alpha);
        
        void setSceneBlending(SceneBlendType sbt);
        void setDepthWriteEnabled(bool enabled);
        void setCullingMode(int mode);
        
        void setLightingEnabled(bool enabled);
        
        void load() {}
        
        static MaterialPtr getByName(const String& name);
        
    private:
        String mName;
    };
    typedef Material::Ptr MaterialPtr;
    
    enum SceneBlendType { SBT_REPLACE, SBT_ADD, SBT_MODULATE, SBT_ALPHA_BLEND };
    
    // Texture
    class Texture : public std::enable_shared_from_this<Texture>
    {
    public:
        typedef std::shared_ptr<Texture> Ptr;
        
        Texture(const String& name, TextureType texType = TEX_TYPE_2D);
        virtual ~Texture();
        
        static TexturePtr create(const String& name);
        static TexturePtr getByName(const String& name);
        
        void setWidth(uint32 w);
        void setHeight(uint32 h);
        void setFormat(PixelFormat format);
        
        void load() {}
        
        void* getBuffer() { return NULL; }
        
        enum TextureType { TEX_TYPE_1D, TEX_TYPE_2D, TEX_TYPE_3D, TEX_TYPE_CUBE_MAP };
    };
    typedef Texture::Ptr TexturePtr;
    
    enum PixelFormat { PF_UNKNOWN, PF_L8, PF_A8, PF_R8G8B8, PF_B8G8R8, PF_R8G8B8A8, PF_B8G8R8A8 };
    
    // Texture manager
    class TextureManager
    {
    public:
        static TextureManager& getSingleton();
        
        TexturePtr create(const String& name, const String& group = "General");
        TexturePtr getByName(const String& name) const;
        
        void unload(const String& name);
        void unloadAll() {}
    };
    
    // Material manager
    class MaterialManager
    {
    public:
        static MaterialManager& getSingleton();
        
        MaterialPtr create(const String& name, const String& group = "General");
        MaterialPtr getByName(const String& name) const;
        
        void unload(const String& name);
        void unloadAll() {}
    };
    
    // Mesh
    class Mesh : public std::enable_shared_from_this<Mesh>
    {
    public:
        typedef std::shared_ptr<Mesh> Ptr;
        
        Mesh(const String& name);
        virtual ~Mesh();
        
        static MeshPtr create(const String& name, const String& group = "General");
        static MeshPtr getByName(const String& name);
        
        void setSkeletonLink(const String& skelName) {}
        
        void setSubMeshNameIndex(uint32 index, const String& name) {}
        
        SubMesh* createSubMesh(const String& name);
        SubMesh* getSubMesh(const String& name) const;
        
        void buildEdgeList() {}
        void setLodStrategy(LodStrategy* strategy) {}
        
        void load() {}
        
        const AxisAlignedBox& getBoundingBox() const { return mAABB; }
        Real getBoundingRadius() const { return mBoundRadius; }
        
        void _updateBoundsFromVertexBuffer() {}
        
    protected:
        String mName;
        AxisAlignedBox mAABB;
        Real mBoundRadius;
    };
    typedef Mesh::Ptr MeshPtr;
    
    // SubMesh
    class SubMesh
    {
    public:
        SubMesh();
        virtual ~SubMesh();
        
        void setMaterialName(const String& name);
        
        void setVertexData(void* vertexData) { mVertexData = vertexData; }
        void setIndexData(void* indexData) { mIndexData = indexData; }
        
        void setOperationType(int opType) { mOpType = opType; }
        
        void* getVertexData() const { return mVertexData; }
        void* getIndexData() const { return mIndexData; }
        
        enum OperationType { OT_TRIANGLE_LIST, OT_TRIANGLE_STRIP, OT_TRIANGLE_FAN };
        
    protected:
        void* mVertexData;
        void* mIndexData;
        int mOpType;
    };
    
    // Mesh manager
    class MeshManager
    {
    public:
        static MeshManager& getSingleton();
        
        MeshPtr create(const String& name, const String& group = "General");
        MeshPtr getByName(const String& name) const;
        
        void unload(const String& name);
        void unloadAll() {}
    };
    
    // Vertex data
    class VertexData
    {
    public:
        VertexData();
        ~VertexData();
        
        struct VertexElement
        {
            unsigned int source;
            unsigned int offset;
            VertexElementType type;
            VertexElementSemantic semantic;
            unsigned short index;
        };
        
        typedef std::vector<VertexElement> VertexElementList;
        VertexElementList elements;
        
        unsigned int vertexCount;
        void* vertexBuffer;
    };
    
    enum VertexElementType { VET_FLOAT1, VET_FLOAT2, VET_FLOAT3, VET_FLOAT4, VET_COLOUR, VET_SHORT1, VET_SHORT2, VET_SHORT3, VET_SHORT4 };
    enum VertexElementSemantic { VES_POSITION, VES_NORMAL, VES_DIFFUSE, VES_SPECULAR, VES_TEXTURE_COORDINATES };
    
    // Index data  
    class IndexData
    {
    public:
        IndexData();
        ~IndexData();
        
        unsigned int indexCount;
        void* indexBuffer;
        bool use32bit;
    };
    
    // Hardware buffer
    class HardwareBuffer
    {
    public:
        enum Usage { HBU_STATIC = 1, HBU_DYNAMIC = 2, HBU_WRITE_ONLY = 4 };
        enum LockOptions { BL_NORMAL, BL_READ_ONLY, BL_DISCARD, BL_NO_OVERWRITE };
    };
    
    class HardwareVertexBuffer
    {
    public:
        HardwareVertexBuffer(size_t vertexSize, size_t numVertices, HardwareBuffer::Usage usage);
        ~HardwareVertexBuffer();
        
        void* lock(HardwareBuffer::LockOptions options);
        void unlock();
        
        size_t getVertexSize() const { return mVertexSize; }
        size_t getNumVertices() const { return mNumVertices; }
        
    protected:
        size_t mVertexSize;
        size_t mNumVertices;
    };
    
    // Lod strategy
    class LodStrategy
    {
    public:
        virtual ~LodStrategy() {}
        virtual const String& getName() const = 0;
    };
    
    // Particle system
    class ParticleSystem : public MovableObject
    {
    public:
        ParticleSystem(const String& name);
        virtual ~ParticleSystem();
        
        void setMaterialName(const String& matName);
        void setParticleQuota(int quota);
        
        void setEmitter(ParticleEmitter* emitter) {}
        
        void setDefaultDimensions(Real width, Real height);
        
        void setSpeed(float speed) {}
        void setLifetime(float lifetime) {}
        
        void setEmitRate(float rate) {}
        
        void setDirection(const Vector3& direction) {}
        void setEmissionRate(float rate) {}
        
        void setTimeToLive(float ttl) {}
        void setColour(const ColourValue& colour) {}
        
        void setGravity(const Vector3& gravity) {}
        
        void _update(Real time) {}
        
        void setVisible(bool visible);
        
        size_t getNumParticles() const { return 0; }
        
    protected:
        float mSpeed;
        float mLifetime;
        float mEmitRate;
        int mQuota;
    };
    
    class ParticleEmitter
    {
    public:
        ParticleEmitter(ParticleSystem* ps) {}
        virtual ~ParticleEmitter() {}
        
        void setDirection(const Vector3& dir) {}
        void setEmissionRate(float rate) {}
        void setTimeToLive(float ttl) {}
        void setColour(const ColourValue& colour) {}
        void setInitialVelocity(float vel) {}
    };
    
    // Font
    class Font : public std::enable_shared_from_this<Font>
    {
    public:
        typedef std::shared_ptr<Font> Ptr;
        
        Font(const String& name);
        virtual ~Font();
        
        static FontPtr create(const String& name, const String& group = "General");
        static FontPtr getByName(const String& name);
        
        void setType(int type) {}
        void setSize(int size) {}
        void setMaterialName(const String& name) {}
        
        void load() {}
    };
    typedef Font::Ptr FontPtr;
    
    // Font manager
    class FontManager
    {
    public:
        static FontManager& getSingleton();
        
        FontPtr create(const String& name, const String& group = "General");
        FontPtr getByName(const String& name) const;
    };
    
    // Simple renderer
    class SimpleRenderer
    {
    public:
        static SimpleRenderer& getInstance();
        
        void addRenderable(void* rend) {}
        void clear() {}
    };
    
    // Inline implementations
    inline const Vector3 Vector3::ZERO(0, 0, 0);
    inline const Vector3 Vector3::UNIT_X(1, 0, 0);
    inline const Vector3 Vector3::UNIT_Y(0, 1, 0);
    inline const Vector3 Vector3::UNIT_Z(0, 0, 1);
    inline const Vector3 Vector3::NEGATIVE_UNIT_X(-1, 0, 0);
    inline const Vector3 Vector3::NEGATIVE_UNIT_Y(0, -1, 0);
    inline const Vector3 Vector3::NEGATIVE_UNIT_Z(0, 0, -1);
    
    inline const Quaternion Quaternion::IDENTITY(1, 0, 0, 0);
    
    inline const Matrix4 Matrix4::IDENTITIY;
    inline const Matrix4 Matrix4::ZERO;
    
    inline const ColourValue ColourValue::ZERO(0, 0, 0, 0);
    inline const ColourValue ColourValue::Black(0, 0, 0, 1);
    inline const ColourValue ColourValue::White(1, 1, 1, 1);
    inline const ColourValue ColourValue::Red(1, 0, 0, 1);
    inline const ColourValue ColourValue::Green(0, 1, 0, 1);
    inline const ColourValue ColourValue::Blue(0, 0, 1, 1);
    
    const String ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME = "General";
    const String StringUtil::BLANK = "";
}

#endif
