#ifndef WII_OGRE_H
#define WII_OGRE_H

#include "OgreBuildSettings.h"
#include "OgrePrerequisites.h"
#include "OgreSceneManager.h"
#include "OgreMath.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <cstring>
#include <fstream>
#include <sstream>
#include <list>
#include <cassert>

#ifndef GL_VENDOR
#define GL_VENDOR 0x1F00
#endif
#ifndef GL_RENDERER
#define GL_RENDERER 0x1F01
#endif
inline const unsigned char* glGetString(unsigned int name)
{
    (void)name;
    static const unsigned char empty[] = "";
    return empty;
}

#define OGRE_NEW new
#define OGRE_DELETE delete
#define OGRE_ALLOC_T(type, size, category) new type[size]
#define OGRE_NEW_T(type, category) new type
#define OGRE_DELETE_T(ptr, type) delete[] ptr
#define OGRE_EXCEPT(num, msg, src) do { (void)(num); (void)(msg); (void)(src); } while (0)
#define OGRE_PROFILING 0
#define OGRE_DEREF_DISPLAYSTRING_ITERATOR(i) (*(i))
#define OGRE_RENDERABLE_DEFAULT_PRIORITY 0

namespace Ogre
{
    enum { MEMCATEGORY_GENERAL = 0 };

    typedef unsigned char int8;

    template <typename T>
    class SharedPtr
    {
    public:
        template <typename U> friend class SharedPtr;
        SharedPtr() {}
        SharedPtr(T* raw) : mPtr(raw) {}
        SharedPtr(const std::shared_ptr<T>& p) : mPtr(p) {}
        template <typename U>
        SharedPtr(const std::shared_ptr<U>& p) : mPtr(std::static_pointer_cast<T>(p)) {}
        template <typename U>
        SharedPtr(const SharedPtr<U>& p) : mPtr(std::static_pointer_cast<T>(p.mPtr)) {}
        T* getPointer() const { return mPtr.get(); }
        T* get() const { return mPtr.get(); }
        T* operator->() const { return mPtr.get(); }
        T& operator*() const { return *mPtr; }
        bool isNull() const { return !mPtr; }
        void setNull() { mPtr.reset(); }
        operator bool() const { return static_cast<bool>(mPtr); }
        SharedPtr& operator=(T* raw) { mPtr.reset(raw); return *this; }
        SharedPtr& operator=(const std::shared_ptr<T>& p) { mPtr = p; return *this; }
        SharedPtr& operator=(const SharedPtr& p) { mPtr = p.mPtr; return *this; }
        template <typename U>
        SharedPtr& operator=(const std::shared_ptr<U>& p) { mPtr = std::static_pointer_cast<T>(p); return *this; }
        template <typename U>
        SharedPtr& operator=(const SharedPtr<U>& p) { mPtr = std::static_pointer_cast<T>(p.mPtr); return *this; }
        const std::shared_ptr<T>& stdPtr() const { return mPtr; }
        operator std::shared_ptr<T>() const { return mPtr; }
    private:
        std::shared_ptr<T> mPtr;
    };

    class GpuProgramParameters;
    typedef SharedPtr<GpuProgramParameters> GpuProgramParametersSharedPtr;

    class Exception : public std::exception
    {
    public:
        enum ErrorCode { ERR_FILE_NOT_FOUND, ERR_INVALID_STATE, ERR_ITEM_NOT_FOUND };
        Exception(ErrorCode num, const String& msg, const String& src)
            : mNumber(num), mDescription(msg), mSource(src) {}
        const char* what() const noexcept override { return mDescription.c_str(); }
        String getFullDescription() const { return mSource + ": " + mDescription; }
    private:
        ErrorCode mNumber;
        String mDescription;
        String mSource;
    };

    class StringUtil
    {
    public:
        static const String BLANK;
    };
    inline const String StringUtil::BLANK = "";

    class DisplayString : public std::string
    {
    public:
        DisplayString() {}
        DisplayString(const std::string& s) : std::string(s) {}
        DisplayString(const char* s) : std::string(s ? s : "") {}
        DisplayString(const std::wstring& ws) : std::string(ws.begin(), ws.end()) {}
        std::wstring asWStr() const { return std::wstring(begin(), end()); }
        std::string asUTF8() const { return *this; }
        DisplayString& operator=(const std::wstring& ws)
        {
            assign(ws.begin(), ws.end());
            return *this;
        }
    };

    struct NameValuePairList : public std::map<String, String> {};

    enum LogMessageLevel {
        LL_BOREME = 0,
        LL_NORMAL = 1,
        LL_TRIVIAL = 2,
        LML_NORMAL = LL_NORMAL,
        LML_TRIVIAL = LL_TRIVIAL,
        LML_CRITICAL = 3
    };

    class Log
    {
    public:
        void logMessage(const String& msg, LogMessageLevel lml = LL_NORMAL) { (void)msg; (void)lml; }
    };

    class LogManager
    {
    public:
        static LogManager& getSingleton() { static LogManager lm; return lm; }
        void setLogDetail(int detail) { (void)detail; }
        void logMessage(LogMessageLevel lml, const String& msg) { (void)lml; (void)msg; }
        Log* getDefaultLog() { static Log l; return &l; }
    };

    class NameGenerator
    {
    public:
        explicit NameGenerator(const String& prefix = "Obj") : mPrefix(prefix), mNext(1) {}
        String generate() { return mPrefix + std::to_string(mNext++); }
    private:
        String mPrefix;
        uint32 mNext;
    };

    class FrameEvent
    {
    public:
        Real timeSinceLastEvent;
        Real timeSinceLastFrame;
        FrameEvent() : timeSinceLastEvent(1.0f / 60.0f), timeSinceLastFrame(1.0f / 60.0f) {}
    };

    class FrameListener
    {
    public:
        virtual ~FrameListener() {}
        virtual bool frameStarted(const FrameEvent& evt) { (void)evt; return true; }
        virtual bool frameEnded(const FrameEvent& evt) { (void)evt; return true; }
        virtual bool frameRenderingQueued(const FrameEvent& evt) { (void)evt; return true; }
    };

    class WindowEventListener
    {
    public:
        virtual ~WindowEventListener() {}
        virtual void windowResized(RenderWindow* rw) { (void)rw; }
        virtual void windowClosed(RenderWindow* rw) { (void)rw; }
        virtual void windowFocusChange(RenderWindow* rw) { (void)rw; }
    };

    class WindowEventUtilities
    {
    public:
        static void addWindowEventListener(RenderWindow* rw, WindowEventListener* l) { (void)rw; (void)l; }
        static void removeWindowEventListener(RenderWindow* rw, WindowEventListener* l) { (void)rw; (void)l; }
    };

    class Timer
    {
    public:
        Timer() : mTicks(0) {}
        void reset() { mTicks = 0; }
        unsigned long getMilliseconds() { mTicks += 16; return mTicks; }
    private:
        unsigned long mTicks;
    };

    class DataStream
    {
    public:
        virtual ~DataStream() {}
        virtual bool isReadable() const { return true; }
        virtual bool isWriteable() const { return false; }
        virtual size_t size() const { return 0; }
        virtual size_t tell() const { return 0; }
        virtual size_t read(void* buf, size_t count)
        {
            if (buf && count) {
                std::memset(buf, 0, count);
            }
            return count;
        }
        virtual size_t write(const void* buf, size_t count)
        {
            (void)buf;
            return count;
        }
        virtual String readLine(char* buf, size_t maxCount, const String& delim = "\n")
        {
            (void)delim;
            if (buf && maxCount) {
                buf[0] = '\0';
            }
            return String();
        }
        virtual void close() {}
        virtual String getAsString() { return String(); }
        virtual void seek(size_t pos) { (void)pos; }
    };
    typedef SharedPtr<DataStream> DataStreamPtr;

    class MemoryDataStream : public DataStream
    {
    public:
        MemoryDataStream(void* ptr, size_t len, bool freeOnClose, bool readOnly)
            : mPtr(ptr), mLen(len), mFree(freeOnClose), mReadOnly(readOnly) {}
        virtual ~MemoryDataStream() {}
        virtual String getAsString() { return String(); }
        virtual size_t size() const { return mLen; }
        virtual size_t tell() const { return 0; }
    private:
        void* mPtr;
        size_t mLen;
        bool mFree;
        bool mReadOnly;
    };

    class Resource
    {
    public:
        Resource() {}
        explicit Resource(const String& name) : mName(name) {}
        virtual ~Resource() {}
        const String& getName() const { return mName; }
        void setName(const String& name) { mName = name; }
        const String& getGroup() const { return mGroup; }
        void setGroup(const String& group) { mGroup = group; }
        bool isReloadable() const { return true; }
        int getLoadingState() const { return 0; }
        virtual void reload() {}
    private:
        String mName;
        String mGroup;
    };
    typedef SharedPtr<Resource> ResourcePtr;

    class ManualResourceLoader
    {
    public:
        virtual ~ManualResourceLoader() {}
    };

    class ResourceManager
    {
    public:
        virtual ~ResourceManager() {}
    protected:
        virtual Resource* createImpl(const String& name, ResourceHandle handle,
                                     const String& group, bool isManual, ManualResourceLoader* loader,
                                     const NameValuePairList* params)
        {
            (void)handle;
            (void)group;
            (void)isManual;
            (void)loader;
            (void)params;
            return new Resource(name);
        }
    };

    class FileDataStream : public DataStream
    {
    public:
        explicit FileDataStream(const String& path, bool writeable = false)
            : mWriteable(writeable)
        {
            std::ios::openmode mode = writeable ? (std::ios::in | std::ios::out | std::ios::binary)
                                                : (std::ios::in | std::ios::binary);
            mStream.open(path.c_str(), mode);
            if (!mStream.is_open() && writeable) {
                std::ofstream create(path.c_str(), std::ios::out | std::ios::binary);
                create.close();
                mStream.open(path.c_str(), mode);
            }
        }

        FileDataStream(const String& name, std::ifstream* stream, bool freeOnClose)
            : mWriteable(false)
        {
            (void)name;
            (void)stream;
            if (freeOnClose && stream) {
                delete stream;
            }
        }

        FileDataStream(const String& name, std::fstream* stream, bool freeOnClose)
            : mWriteable(true)
        {
            (void)name;
            if (stream) {
                mStream.swap(*stream);
                if (freeOnClose) {
                    delete stream;
                }
            }
        }

        virtual bool isReadable() const { return mStream.is_open(); }
        virtual bool isWriteable() const { return mWriteable && mStream.is_open(); }
        virtual size_t size() const
        {
            std::fstream& stream = const_cast<std::fstream&>(mStream);
            if (!stream.is_open()) return 0;
            std::streampos cur = stream.tellg();
            stream.seekg(0, std::ios::end);
            std::streampos end = stream.tellg();
            stream.seekg(cur);
            return static_cast<size_t>(end);
        }
        virtual size_t tell() const
        {
            std::fstream& stream = const_cast<std::fstream&>(mStream);
            if (!stream.is_open()) return 0;
            return static_cast<size_t>(stream.tellg());
        }
        virtual size_t read(void* buf, size_t count)
        {
            if (!mStream.is_open() || !buf || count == 0) return 0;
            mStream.read(reinterpret_cast<char*>(buf), static_cast<std::streamsize>(count));
            return static_cast<size_t>(mStream.gcount());
        }
        virtual size_t write(const void* buf, size_t count)
        {
            if (!mStream.is_open() || !mWriteable || !buf || count == 0) return 0;
            mStream.write(reinterpret_cast<const char*>(buf), static_cast<std::streamsize>(count));
            return count;
        }
        virtual String readLine(char* buf, size_t maxCount, const String& delim = "\n")
        {
            String line;
            if (!mStream.is_open()) {
                if (buf && maxCount) buf[0] = '\0';
                return line;
            }

            std::getline(mStream, line);
            if (!delim.empty()) {
                size_t p = line.find(delim[0]);
                if (p != String::npos) {
                    line = line.substr(0, p);
                }
            }

            if (buf && maxCount) {
                size_t n = line.size() < (maxCount - 1) ? line.size() : (maxCount - 1);
                std::memcpy(buf, line.c_str(), n);
                buf[n] = '\0';
            }
            return line;
        }
        virtual void close()
        {
            if (mStream.is_open()) {
                mStream.close();
            }
        }
        virtual void seek(size_t pos)
        {
            if (!mStream.is_open()) return;
            mStream.seekg(static_cast<std::streamoff>(pos), std::ios::beg);
            if (mWriteable) {
                mStream.seekp(static_cast<std::streamoff>(pos), std::ios::beg);
            }
        }

    private:
        bool mWriteable;
        std::fstream mStream;
    };

    typedef FileDataStream FileStreamDataStream;

    class ResourceGroupManager
    {
    public:
        static const String DEFAULT_RESOURCE_GROUP_NAME;
        static const String AUTODETECT_RESOURCE_GROUP_NAME;
        static ResourceGroupManager& getSingleton() { static ResourceGroupManager rgm; return rgm; }
        void addResourceLocation(const String& name, const String& type, const String& group = "General")
        { (void)name; (void)type; (void)group; }
        void addResourceLocation(const String& name, const String& type, const String& group, bool recursive, bool readOnly)
        { (void)name; (void)type; (void)group; (void)recursive; (void)readOnly; }
        void initialiseResourceGroup(const String& group) { (void)group; }
        void initialiseAllResourceGroups() {}
        void createResourceGroup(const String& group) { (void)group; }
        void loadResourceGroup(const String& group) { (void)group; }
        void clearResourceGroup(const String& group) { (void)group; }
        void destroyResourceGroup(const String& group) { (void)group; }
        void addResourceGroupListener(void* listener) { (void)listener; }
        void removeResourceGroupListener(void* listener) { (void)listener; }
        DataStreamPtr openResource(const String& name, const String& group)
        {
            (void)group;
            return DataStreamPtr(new FileDataStream(name, false));
        }
        DataStreamPtr createResource(const String& name, const String& group, bool overwrite)
        {
            (void)group;
            (void)overwrite;
            return DataStreamPtr(new FileDataStream(name, true));
        }
    };
    inline const String ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME = "General";
    inline const String ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME = "General";

    class ConfigFile
    {
    public:
        typedef std::multimap<String, String> SettingsMultiMap;
        class SectionIterator
        {
        public:
            bool hasMoreElements() const { return false; }
            String peekNextKey() const { return String(); }
            SettingsMultiMap* getNext() { return 0; }
        };

        void load(const String& file) { (void)file; }
        SectionIterator getSectionIterator() { return SectionIterator(); }
    };

    class RenderSystem
    {
    public:
        class Capabilities
        {
        public:
            bool hasCapability(int cap) const { (void)cap; return true; }
        };

        struct ConfigOption
        {
            String currentValue;
            StringVector possibleValues;
            bool immutable;
            ConfigOption() : currentValue(""), immutable(false) {}
        };

        typedef std::map<String, ConfigOption> ConfigOptionMap;
        ConfigOptionMap& getConfigOptions() { return mCfg; }
        void setConfigOption(const String& n, const String& v) { mCfg[n].currentValue = v; }
        Capabilities* getCapabilities() { return &mCaps; }
        Viewport* _getViewport() { static Viewport vp; return &vp; }
        void _setTextureUnitSettings(size_t unit, const class TextureUnitState& state) { (void)unit; (void)state; }
        void _disableTextureUnitsFrom(size_t unit) { (void)unit; }
        void _setSceneBlending(int src, int dst, int op = 0) { (void)src; (void)dst; (void)op; }
        void _setSeparateSceneBlending(int src, int dst, int srcAlpha, int dstAlpha, int op = 0, int opAlpha = 0)
        { (void)src; (void)dst; (void)srcAlpha; (void)dstAlpha; (void)op; (void)opAlpha; }
        void _setPointParameters(Real size, bool atten, Real c, Real l, Real q, Real minSize, Real maxSize)
        { (void)size; (void)atten; (void)c; (void)l; (void)q; (void)minSize; (void)maxSize; }
        void _setPointSpritesEnabled(bool enabled) { (void)enabled; }
        void _setDepthBufferFunction(int fn) { (void)fn; }
        void _setDepthBufferCheckEnabled(bool enabled) { (void)enabled; }
        void _setDepthBufferWriteEnabled(bool enabled) { (void)enabled; }
        void _setDepthBias(Real constantBias, Real slopeScaleBias) { (void)constantBias; (void)slopeScaleBias; }
        void _setAlphaRejectSettings(int fn, unsigned char value, bool alphaToCoverage)
        { (void)fn; (void)value; (void)alphaToCoverage; }
        void _setColourBufferWriteEnabled(bool r, bool g, bool b, bool a) { (void)r; (void)g; (void)b; (void)a; }
        void _setCullingMode(int mode) { (void)mode; }
        void setShadingType(int mode) { (void)mode; }
        void _setPolygonMode(int mode) { (void)mode; }
        bool isGpuProgramBound(int type) const { (void)type; return false; }
        void unbindGpuProgram(int type) { (void)type; }
        void bindGpuProgramParameters(int type, const GpuProgramParametersSharedPtr& params, unsigned short mask)
        { (void)type; (void)params; (void)mask; }
        void _setFog(int mode, const ColourValue& colour, Real density, Real start, Real end)
        { (void)mode; (void)colour; (void)density; (void)start; (void)end; }
        void setLightingEnabled(bool enabled) { (void)enabled; }
    private:
        ConfigOptionMap mCfg;
        Capabilities mCaps;
    };

    typedef std::vector<RenderSystem*> RenderSystemList;
    typedef RenderSystem::ConfigOption ConfigOption;
    typedef RenderSystem::ConfigOptionMap ConfigOptionMap;

    class ControllerManager
    {
    public:
        static ControllerManager& getSingleton() { static ControllerManager cm; return cm; }
        void setTimeFactor(Real factor) { mTimeFactor = factor; }
        Real getTimeFactor() const { return mTimeFactor; }
    private:
        Real mTimeFactor = 1.0f;
    };

    class Plugin { public: virtual ~Plugin() {} };
    class GLPlugin : public Plugin {};
    class GLES2Plugin : public Plugin {};
    class ParticleFXPlugin : public Plugin {};

    class SceneManagerFactory;
    class TextureManager;

    class RenderTargetEvent
    {
    public:
        RenderWindow* source = 0;
    };
    class RenderTargetViewportEvent
    {
    public:
        Viewport* source = 0;
    };

    class Profiler
    {
    public:
        static Profiler* getSingletonPtr() { return 0; }
        void addListener(class OverlayProfileSessionListener* listener) { (void)listener; }
        void removeListener(class OverlayProfileSessionListener* listener) { (void)listener; }
    };

    class RenderTargetListener
    {
    public:
        virtual ~RenderTargetListener() {}
        virtual void preRenderTargetUpdate(const RenderTargetEvent& evt) { (void)evt; }
        virtual void postRenderTargetUpdate(const RenderTargetEvent& evt) { (void)evt; }
        virtual void preViewportUpdate(const RenderTargetViewportEvent& evt) { (void)evt; }
    };

    class ResourceGroupListener
    {
    public:
        virtual ~ResourceGroupListener() {}
        virtual void resourceGroupScriptingStarted(const String& groupName, size_t scriptCount) { (void)groupName; (void)scriptCount; }
        virtual void scriptParseStarted(const String& scriptName, bool& skipThisScript) { (void)scriptName; (void)skipThisScript; }
        virtual void scriptParseEnded(const String& scriptName, bool skipped) { (void)scriptName; (void)skipped; }
        virtual void resourceGroupScriptingEnded(const String& groupName) { (void)groupName; }
        virtual void resourceGroupLoadStarted(const String& groupName, size_t resourceCount) { (void)groupName; (void)resourceCount; }
        virtual void resourceGroupLoadEnded(const String& groupName) { (void)groupName; }
        virtual void resourceLoadStarted(const ResourcePtr& resource) { (void)resource; }
        virtual void resourceLoadEnded(void) {}
        virtual void worldGeometryStageStarted(const String& description) { (void)description; }
        virtual void worldGeometryStageEnded(void) {}
    };

    class Root
    {
    public:
        Root() : mSceneMgr(0), mWindow(0), mFallbackWindow(0), mFrameListener(0) {}
        explicit Root(const String& pluginFileName) : mSceneMgr(0), mWindow(0), mFallbackWindow(0), mFrameListener(0) { (void)pluginFileName; }
        ~Root() {}

        static Root* getSingletonPtr() { static Root r; return &r; }
        static Root& getSingleton() { return *getSingletonPtr(); }

        void installPlugin(Plugin* p) { mPlugins.push_back(std::unique_ptr<Plugin>(p)); }

        const RenderSystemList& getAvailableRenderers()
        {
            if (mRenderers.empty()) {
                mRenderers.push_back(new RenderSystem());
            }
            return mRenderers;
        }

        void setRenderSystem(RenderSystem* rs) { mCurrentRS = rs; }

        void addSceneManagerFactory(SceneManagerFactory* factory) { (void)factory; }
        void removeSceneManagerFactory(SceneManagerFactory* factory) { (void)factory; }

        SceneManager* createSceneManager(const String& typeName)
        {
            (void)typeName;
            if (!mSceneMgr) mSceneMgr = new SceneManager("default");
            return mSceneMgr;
        }
        SceneManager* createSceneManager(int typeMask)
        {
            (void)typeMask;
            return createSceneManager(String("default"));
        }
        void destroySceneManager(SceneManager* sceneManager)
        {
            if (sceneManager == mSceneMgr) {
                delete mSceneMgr;
                mSceneMgr = 0;
            }
        }

        RenderWindow* initialise(bool autoCreateWindow = true, const String& windowTitle = "Powerslide")
        {
            (void)autoCreateWindow;
            (void)windowTitle;
            if (!mWindow) mWindow = new RenderWindow();
            return mWindow;
        }

        RenderWindow* createRenderWindow(const String& name, unsigned int w, unsigned int h, bool fullscreen, void* opt = 0)
        {
            (void)name; (void)w; (void)h; (void)fullscreen; (void)opt;
            if (!mWindow) mWindow = new RenderWindow();
            mWindow->setFullscreen(fullscreen);
            return mWindow;
        }

        void addFrameListener(FrameListener* listener) { mFrameListener = listener; }
        bool removeFrameListener(FrameListener* listener) { if (mFrameListener == listener) mFrameListener = 0; return true; }

        void startRendering()
        {
            if (!mFrameListener) return;
            FrameEvent evt;
            while (true) {
                if (!mFrameListener->frameStarted(evt)) break;
                if (!mFrameListener->frameRenderingQueued(evt)) break;
                if (mWindow) {
                    mWindow->update();
                }
                if (!mFrameListener->frameEnded(evt)) break;
            }
        }

        bool renderOneFrame()
        {
            if (mWindow) {
                mWindow->update();
            }
            return true;
        }
        bool renderOneFrame(Real timeSinceLastFrame)
        {
            (void)timeSinceLastFrame;
            if (mWindow) {
                mWindow->update();
            }
            return true;
        }

        RenderSystem* getRenderSystem() { return mCurrentRS; }
        RenderWindow* getAutoCreatedWindow() { return mWindow; }
        RenderWindow* getRenderTarget(const String& name) { (void)name; return mWindow; }
        TextureManager* getTextureManager();
        void convertColourValue(const ColourValue& colour, RGBA* pDest)
        {
            if (pDest) {
                *pDest = colour.getAsRGBA();
            }
        }

    private:
        SceneManager* mSceneMgr;
        RenderWindow* mWindow;
        RenderWindow* mFallbackWindow;
        FrameListener* mFrameListener;
        RenderSystem* mCurrentRS = 0;
        RenderSystemList mRenderers;
        std::vector<std::unique_ptr<Plugin> > mPlugins;
    };

    class UTFString
    {
    public:
        UTFString() {}
        UTFString(const String& s) : mString(s) {}
        operator String() const { return mString; }
    private:
        String mString;
    };

    class Material;
    class PanelOverlayElement;
    class Texture;
    class Mesh;
    class Font;
    typedef SharedPtr<Material> MaterialPtr;
    typedef SharedPtr<Texture> TexturePtr;
    typedef SharedPtr<Mesh> MeshPtr;
    typedef SharedPtr<Font> FontPtr;
    typedef PanelOverlayElement OverlayContainer;

    class OverlayAlloc {};

    enum GuiMetricsMode
    {
        GMM_PIXELS = 0,
        GMM_RELATIVE = 1
    };

    enum GuiHorizontalAlignment
    {
        GHA_LEFT = 0,
        GHA_CENTER = 1,
        GHA_RIGHT = 2
    };

    enum GuiVerticalAlignment
    {
        GVA_TOP = 0,
        GVA_CENTER = 1,
        GVA_BOTTOM = 2
    };

    class OverlayElement
    {
    public:
        virtual ~OverlayElement() {}
        virtual void addChild(OverlayElement* e) { (void)e; }
        virtual void removeChild(const String& childName) { (void)childName; }
        virtual void show() { mVisible = true; }
        virtual void hide() { mVisible = false; }
        virtual bool isVisible() const { return mVisible; }
        virtual void setMetricsMode(GuiMetricsMode mode) { (void)mode; }
        virtual void setWidth(Real w) { mWidth = w; }
        virtual void setHeight(Real h) { mHeight = h; }
        virtual void setTop(Real t) { mTop = t; }
        virtual void setLeft(Real l) { mLeft = l; }
        virtual Real getWidth() const { return mWidth; }
        virtual Real getHeight() const { return mHeight; }
        virtual Real getTop() const { return mTop; }
        virtual Real getLeft() const { return mLeft; }
        virtual void setHorizontalAlignment(GuiHorizontalAlignment a) { (void)a; }
        virtual void setVerticalAlignment(GuiVerticalAlignment a) { (void)a; }
        virtual void setCaption(const DisplayString& c) { mCaption = c; }
        virtual const DisplayString& getCaption() const { return mCaption; }
        virtual void setMaterialName(const String& material) { (void)material; }
        virtual const String& getName() const { return mName; }
        void setName(const String& name) { mName = name; }
    protected:
        bool mVisible = true;
        Real mWidth = 0.0f;
        Real mHeight = 0.0f;
        Real mTop = 0.0f;
        Real mLeft = 0.0f;
        String mName;
        DisplayString mCaption;
    };
    class RenderQueueListener
    {
    public:
        virtual ~RenderQueueListener() {}
        virtual void renderQueueStarted(uint8 queueGroupId, const String& invocation, bool& skipThisInvocation)
        { (void)queueGroupId; (void)invocation; (void)skipThisInvocation; }
    };

    class PanelOverlayElement : public OverlayElement
    {
    public:
        void addChild(OverlayElement* e) { (void)e; }
        void removeChild(const String& childName) { (void)childName; }
        void setUV(Real u1, Real v1, Real u2, Real v2) { (void)u1; (void)v1; (void)u2; (void)v2; }
        void setMaterialName(const String& material) { mMaterialName = material; }
        const String& getMaterialName() const { return mMaterialName; }
    private:
        String mMaterialName;
    };
    class TextAreaOverlayElement : public OverlayElement
    {
    public:
        enum Alignment { Left = 0, Center = 1, Right = 2 };
        void setCharHeight(Real h) { mCharHeight = h; }
        Real getCharHeight() const { return mCharHeight; }
        void setFontName(const String& font) { mFontName = font; }
        const String& getFontName() const { return mFontName; }
        void setSpaceWidth(Real w) { mSpaceWidth = w; }
        Real getSpaceWidth() const { return mSpaceWidth; }
        void setAlignment(Alignment a) { mAlignment = a; }
        void setColour(const ColourValue& c) { mColour = c; }
        const ColourValue& getColour() const { return mColour; }
        void _update() { mComputedWidth = static_cast<Real>(mCaption.size()) * (mSpaceWidth > 0 ? mSpaceWidth : (mCharHeight * 0.5f)); }
        Real getWidth() const { return mComputedWidth; }
        void setWidth(Real w) { mComputedWidth = w; }
    private:
        String mFontName;
        Real mCharHeight = 0.0f;
        Real mSpaceWidth = 0.0f;
        Real mComputedWidth = 0.0f;
        Alignment mAlignment = Left;
        ColourValue mColour = ColourValue::White;
    };

    enum PixelFormat
    {
        PF_UNKNOWN = 0,
        PF_A8R8G8B8,
        PF_B8G8R8A8,
        PF_R8G8B8,
        PF_B8G8R8,
        PF_R5G6B5,
        PF_R8,
        PF_FLOAT16_R,
        PF_BYTE_RGB,
        PF_BYTE_BGRA
    };

    struct PixelBox
    {
        PixelBox(size_t w = 0, size_t h = 0, size_t d = 1, PixelFormat pf = PF_A8R8G8B8, void* p = 0)
            : width(w), height(h), depth(d), format(pf), data(p) {}
        size_t width;
        size_t height;
        size_t depth;
        PixelFormat format;
        void* data;
    };

    class Image
    {
    public:
        Image() : mWidth(0), mHeight(0), mDepth(1), mBpp(32), mFormat(PF_A8R8G8B8) {}
        void load(const DataStreamPtr& stream, const String& ext) { (void)stream; (void)ext; mWidth = 64; mHeight = 64; mData.assign(mWidth * mHeight * 4, 0); }
        void load(const String& name, const String& group)
        {
            (void)name;
            (void)group;
            mWidth = 64;
            mHeight = 64;
            mData.assign(mWidth * mHeight * 4, 0);
        }
        void loadDynamicImage(uchar* data, uint width, uint height, uint depth, int format, bool autoDelete)
        {
            mFormat = static_cast<PixelFormat>(format);
            mWidth = width;
            mHeight = height;
            mDepth = depth;
            size_t elemBytes = 4;
            switch (mFormat)
            {
                case PF_R8:
                    elemBytes = 1;
                    break;
                case PF_R5G6B5:
                    elemBytes = 2;
                    break;
                case PF_R8G8B8:
                case PF_B8G8R8:
                case PF_BYTE_RGB:
                    elemBytes = 3;
                    break;
                case PF_A8R8G8B8:
                case PF_B8G8R8A8:
                case PF_BYTE_BGRA:
                default:
                    elemBytes = 4;
                    break;
            }
            size_t size = static_cast<size_t>(mWidth) * static_cast<size_t>(mHeight) * static_cast<size_t>(mDepth) * elemBytes;
            mBpp = static_cast<uint>(elemBytes * 8);
            mData.assign(size, 0);
            if (data && size) {
                std::memcpy(mData.data(), data, size);
            }
            if (autoDelete) {
                delete[] data;
            }
        }
        void resize(ushort width, ushort height)
        {
            mWidth = width;
            mHeight = height;
            mData.assign(static_cast<size_t>(mWidth) * static_cast<size_t>(mHeight) * 4, 0);
        }
        uint getWidth() const { return mWidth; }
        uint getHeight() const { return mHeight; }
        PixelFormat getFormat() const { return mFormat; }
        size_t getSize() const { return mData.size(); }
        uint getBPP() const { return mBpp; }
        uchar* getData() { return mData.empty() ? 0 : mData.data(); }
        const uchar* getData() const { return mData.empty() ? 0 : mData.data(); }
        ColourValue getColourAt(uint x, uint y, uint z) const
        {
            (void)z;
            if (mData.empty() || x >= mWidth || y >= mHeight) {
                return ColourValue::Black;
            }
            const size_t pixel = static_cast<size_t>(y) * mWidth + x;
            switch (mFormat)
            {
                case PF_R5G6B5:
                {
                    const size_t i = pixel * 2;
                    const unsigned short packed = static_cast<unsigned short>(mData[i + 0] | (static_cast<unsigned short>(mData[i + 1]) << 8));
                    const float r = static_cast<float>((packed >> 11) & 0x1F) / 31.0f;
                    const float g = static_cast<float>((packed >> 5) & 0x3F) / 63.0f;
                    const float b = static_cast<float>(packed & 0x1F) / 31.0f;
                    return ColourValue(r, g, b, 1.0f);
                }
                case PF_BYTE_BGRA:
                case PF_B8G8R8A8:
                {
                    const size_t i = pixel * 4;
                    return ColourValue(mData[i + 2] / 255.0f, mData[i + 1] / 255.0f, mData[i + 0] / 255.0f, mData[i + 3] / 255.0f);
                }
                case PF_A8R8G8B8:
                {
                    const size_t i = pixel * 4;
                    return ColourValue(mData[i + 1] / 255.0f, mData[i + 2] / 255.0f, mData[i + 3] / 255.0f, mData[i + 0] / 255.0f);
                }
                case PF_R8G8B8:
                case PF_BYTE_RGB:
                {
                    const size_t i = pixel * 3;
                    return ColourValue(mData[i + 0] / 255.0f, mData[i + 1] / 255.0f, mData[i + 2] / 255.0f, 1.0f);
                }
                case PF_B8G8R8:
                {
                    const size_t i = pixel * 3;
                    return ColourValue(mData[i + 2] / 255.0f, mData[i + 1] / 255.0f, mData[i + 0] / 255.0f, 1.0f);
                }
                default:
                {
                    const size_t i = pixel * 4;
                    return ColourValue(mData[i] / 255.0f, mData[i + 1] / 255.0f, mData[i + 2] / 255.0f, mData[i + 3] / 255.0f);
                }
            }
        }
        void applyGamma(uchar* data, Real gamma, size_t size, uint bpp)
        {
            (void)data;
            (void)gamma;
            (void)size;
            (void)bpp;
        }
        void setColourAt(const ColourValue& c, uint x, uint y, uint z)
        {
            (void)z;
            if (mData.empty() || x >= mWidth || y >= mHeight) return;
            const size_t pixel = static_cast<size_t>(y) * mWidth + x;
            const uchar cr = static_cast<uchar>(c.r * 255.0f);
            const uchar cg = static_cast<uchar>(c.g * 255.0f);
            const uchar cb = static_cast<uchar>(c.b * 255.0f);
            const uchar ca = static_cast<uchar>(c.a * 255.0f);
            switch (mFormat)
            {
                case PF_R5G6B5:
                {
                    const size_t i = pixel * 2;
                    const unsigned short r5 = static_cast<unsigned short>((cr >> 3) & 0x1F);
                    const unsigned short g6 = static_cast<unsigned short>((cg >> 2) & 0x3F);
                    const unsigned short b5 = static_cast<unsigned short>((cb >> 3) & 0x1F);
                    const unsigned short packed = static_cast<unsigned short>((r5 << 11) | (g6 << 5) | b5);
                    mData[i + 0] = static_cast<uchar>(packed & 0xFF);
                    mData[i + 1] = static_cast<uchar>((packed >> 8) & 0xFF);
                    break;
                }
                case PF_BYTE_BGRA:
                case PF_B8G8R8A8:
                {
                    const size_t i = pixel * 4;
                    mData[i + 0] = cb;
                    mData[i + 1] = cg;
                    mData[i + 2] = cr;
                    mData[i + 3] = ca;
                    break;
                }
                case PF_A8R8G8B8:
                {
                    const size_t i = pixel * 4;
                    mData[i + 0] = ca;
                    mData[i + 1] = cr;
                    mData[i + 2] = cg;
                    mData[i + 3] = cb;
                    break;
                }
                case PF_R8G8B8:
                case PF_BYTE_RGB:
                {
                    const size_t i = pixel * 3;
                    mData[i + 0] = cr;
                    mData[i + 1] = cg;
                    mData[i + 2] = cb;
                    break;
                }
                case PF_B8G8R8:
                {
                    const size_t i = pixel * 3;
                    mData[i + 0] = cb;
                    mData[i + 1] = cg;
                    mData[i + 2] = cr;
                    break;
                }
                default:
                {
                    const size_t i = pixel * 4;
                    mData[i + 0] = cr;
                    mData[i + 1] = cg;
                    mData[i + 2] = cb;
                    mData[i + 3] = ca;
                    break;
                }
            }
        }
        PixelBox getPixelBox()
        {
            return PixelBox(mWidth, mHeight, mDepth, mFormat, mData.empty() ? 0 : mData.data());
        }
        const PixelBox getPixelBox() const
        {
            return PixelBox(mWidth, mHeight, mDepth, mFormat, mData.empty() ? 0 : const_cast<uchar*>(mData.data()));
        }
        static void scale(const PixelBox& src, const PixelBox& dst)
        {
            if (!src.data || !dst.data) return;
            size_t srcBytes = src.width * src.height * 4;
            size_t dstBytes = dst.width * dst.height * 4;
            size_t bytes = srcBytes < dstBytes ? srcBytes : dstBytes;
            std::memcpy(dst.data, src.data, bytes);
        }
    private:
        uint mWidth;
        uint mHeight;
        uint mDepth;
        uint mBpp;
        PixelFormat mFormat;
        std::vector<uchar> mData;
    };

    struct PixelUtil
    {
        static size_t getMemorySize(size_t width, size_t height, size_t depth, PixelFormat format)
        {
            return width * height * depth * getNumElemBytes(format);
        }
        static size_t getNumElemBytes(PixelFormat format)
        {
            switch (format)
            {
                case PF_R8:
                    return 1;
                case PF_R5G6B5:
                    return 2;
                case PF_R8G8B8:
                case PF_B8G8R8:
                case PF_BYTE_RGB:
                    return 3;
                case PF_A8R8G8B8:
                case PF_B8G8R8A8:
                case PF_BYTE_BGRA:
                    return 4;
                default:
                    return 4;
            }
        }
        static void packColour(const ColourValue& c, PixelFormat format, void* dest)
        {
            (void)format;
            if (!dest) return;
            unsigned char* p = static_cast<unsigned char*>(dest);
            p[0] = static_cast<unsigned char>(c.r * 255.0f);
            p[1] = static_cast<unsigned char>(c.g * 255.0f);
            p[2] = static_cast<unsigned char>(c.b * 255.0f);
            p[3] = static_cast<unsigned char>(c.a * 255.0f);
        }
        static void unpackColour(ColourValue* c, PixelFormat format, const void* src)
        {
            (void)format;
            if (!c || !src) return;
            const unsigned char* p = static_cast<const unsigned char*>(src);
            c->r = p[0] / 255.0f;
            c->g = p[1] / 255.0f;
            c->b = p[2] / 255.0f;
            c->a = p[3] / 255.0f;
        }
        static void bulkPixelConversion(const PixelBox& src, const PixelBox& dst)
        {
            if (!src.data || !dst.data) return;
            const size_t srcElem = getNumElemBytes(src.format);
            const size_t dstElem = getNumElemBytes(dst.format);
            size_t srcBytes = src.width * src.height * srcElem;
            size_t dstBytes = dst.width * dst.height * dstElem;
            size_t bytes = srcBytes < dstBytes ? srcBytes : dstBytes;
            std::memcpy(dst.data, src.data, bytes);
        }
    };

    enum TextureType
    {
        TEX_TYPE_2D = 0,
        TEX_TYPE_2D_ARRAY = 1
    };

    class RenderTexture
    {
    public:
        void addListener(RenderTargetListener* l) { (void)l; }
        void removeAllViewports() {}
        Viewport* addViewport(Camera* camera) { static Viewport vp(camera); return &vp; }
    };

    class Texture
    {
    public:
        Texture() : mWidth(0), mHeight(0), mFormat(PF_UNKNOWN) {}
        class Box
        {
        public:
            Box(size_t l = 0, size_t t = 0, size_t f = 0, size_t r = 0, size_t b = 0, size_t bk = 1)
                : left(l), top(t), front(f), right(r), bottom(b), back(bk) {}
            size_t left, top, front, right, bottom, back;
        };

        class HardwarePixelBuffer
        {
        public:
            typedef Ogre::PixelBox PixelBox;

            const PixelBox& lock(const Box& box, int mode)
            {
                (void)box;
                (void)mode;
                return mBox;
            }
            void lock(int mode) { (void)mode; }
            void unlock() {}
            const PixelBox& getCurrentLock() const { return mBox; }
            void setData(void* data) { mBox.data = data; }
            RenderTexture* getRenderTarget() { static RenderTexture rt; return &rt; }
            void blitFromMemory(const PixelBox& pb) { mBox = pb; }

        private:
            PixelBox mBox{0};
        };

        typedef SharedPtr<HardwarePixelBuffer> HardwarePixelBufferSharedPtr;

        HardwarePixelBufferSharedPtr getBuffer()
        {
            if (!mBuffer) {
                mBuffer = HardwarePixelBufferSharedPtr(new HardwarePixelBuffer());
            }
            return mBuffer;
        }
        HardwarePixelBufferSharedPtr getBuffer(size_t face)
        {
            (void)face;
            return getBuffer();
        }

        void allocate(size_t bytes)
        {
            mRaw.assign(bytes, 0);
            getBuffer()->setData(mRaw.data());
        }

        void setImageData(uint width, uint height, PixelFormat format, const uchar* src, size_t bytes)
        {
            mWidth = width;
            mHeight = height;
            mFormat = format;
            mRaw.assign(bytes, 0);
            if (src && bytes) {
                std::memcpy(mRaw.data(), src, bytes);
            }
            getBuffer()->setData(mRaw.empty() ? 0 : mRaw.data());
        }

        uint getWidth() const { return mWidth; }
        uint getHeight() const { return mHeight; }
        PixelFormat getFormat() const { return mFormat; }
        const uchar* getRawData() const { return mRaw.empty() ? 0 : mRaw.data(); }
        size_t getRawDataSize() const { return mRaw.size(); }

    private:
        HardwarePixelBufferSharedPtr mBuffer;
        std::vector<uchar> mRaw;
        uint mWidth;
        uint mHeight;
        PixelFormat mFormat;
    };

    typedef Texture::HardwarePixelBufferSharedPtr HardwarePixelBufferSharedPtr;
    typedef Texture::Box Box;

    class HardwareBuffer
    {
    public:
        enum { HBL_NORMAL = 0 };
        enum { HBL_READ_ONLY = 2 };
        enum { HBL_DISCARD = 1 };
        enum { HBU_WRITE_ONLY = 3 };
        enum { HBU_DYNAMIC_WRITE_ONLY = 1 };
        enum { HBU_STATIC = 2 };
    };

    enum FilterOptions
    {
        FO_NONE = 0,
        FO_POINT = 1,
        FO_LINEAR = 2
    };

    class TextureManager
    {
    public:
        static TextureManager& getSingleton() { static TextureManager tm; return tm; }
        static TextureManager* getSingletonPtr() { return &getSingleton(); }
        TexturePtr loadImage(const String& name, const String& group, const Image& image, TextureType type, int mipmaps = 0)
        {
            (void)group;
            (void)type;
            (void)mipmaps;
            if (mTextures.find(name) == mTextures.end()) {
                mTextures[name] = TexturePtr(new Texture());
            }
            const size_t bytes = image.getSize();
            const uchar* src = image.getData();
            mTextures[name]->setImageData(image.getWidth(), image.getHeight(), image.getFormat(), src, bytes);
            return mTextures[name];
        }
        TexturePtr getByName(const String& name, const String& group = "General")
        {
            (void)group;
            if (mTextures.find(name) == mTextures.end()) {
                mTextures[name] = TexturePtr(new Texture());
            }
            return mTextures[name];
        }
        TexturePtr getFirstPopulatedTexture(String* outName = 0)
        {
            for (std::map<String, TexturePtr>::iterator it = mTextures.begin(); it != mTextures.end(); ++it)
            {
                Texture* t = it->second.get();
                if (t && t->getRawData() && t->getWidth() > 0 && t->getHeight() > 0)
                {
                    if (outName)
                        *outName = it->first;
                    return it->second;
                }
            }
            return TexturePtr();
        }
        TexturePtr createManual(const String& name, const String& group, TextureType type, uint width, uint height, int mipmaps, PixelFormat format, int usage)
        {
            (void)group;
            (void)type;
            (void)mipmaps;
            (void)format;
            (void)usage;
            TexturePtr t(new Texture());
            t->allocate(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
            mTextures[name] = t;
            return t;
        }
        TexturePtr createManual(const String& name, const String& group, TextureType type, uint width, uint height, uint depth, int mipmaps, PixelFormat format, int usage)
        {
            (void)depth;
            return createManual(name, group, type, width, height, mipmaps, format, usage);
        }
        TexturePtr createManual(const String& name, const String& group, TextureType type, uint width, uint height, uint depth, int mipmaps, int format, int usage)
        {
            return createManual(name, group, type, width, height, depth, mipmaps, static_cast<PixelFormat>(format), usage);
        }
    private:
        std::map<String, TexturePtr> mTextures;
    };

    inline TextureManager* Root::getTextureManager() { return TextureManager::getSingletonPtr(); }

    class GpuProgramParameters
    {
    public:
        enum AutoConstantType
        {
            ACT_WORLDVIEWPROJ_MATRIX,
            ACT_TEXTURE_MATRIX
        };

        void setNamedAutoConstant(const String& name, AutoConstantType type) { (void)name; (void)type; }
        void setNamedConstant(const String& name, int value) { (void)name; (void)value; }
    };
    class HighLevelGpuProgram
    {
    public:
        HighLevelGpuProgram(const String& name = "") : mName(name) {}
        const String& getName() const { return mName; }
        void setSource(const String& src) { (void)src; }
    private:
        String mName;
    };
    typedef SharedPtr<HighLevelGpuProgram> HighLevelGpuProgramPtr;

    enum GpuProgramType
    {
        GPT_VERTEX_PROGRAM = 0,
        GPT_FRAGMENT_PROGRAM = 1,
        GPT_GEOMETRY_PROGRAM = 2
    };

    class HighLevelGpuProgramManager
    {
    public:
        static HighLevelGpuProgramManager* getSingletonPtr() { static HighLevelGpuProgramManager m; return &m; }
        HighLevelGpuProgramPtr createProgram(const String& name, const String& group, const String& lang, GpuProgramType type)
        {
            (void)group;
            (void)lang;
            (void)type;
            return HighLevelGpuProgramPtr(new HighLevelGpuProgram(name));
        }
    };

    class TextureUnitState
    {
    public:
        enum TextureAddressingMode { TAM_WRAP, TAM_CLAMP };
        enum EffectType { ET_PROJECTIVE_TEXTURE };
        enum ContentType { CONTENT_NAMED, CONTENT_SHADOW, CONTENT_COMPOSITOR };
        struct Effect {
            Camera* frustum;
            Effect() : frustum(0) {}
        };
        typedef std::map<EffectType, Effect> EffectMap;

        void setTextureName(const String& name) { mTextureName = name; }
        const String& getTextureName() const { return mTextureName; }
        void setTextureScale(Real u, Real v) { mScaleU = u; mScaleV = v; }
        Real getTextureScaleU() const { return mScaleU; }
        Real getTextureScaleV() const { return mScaleV; }
        void setTextureAddressingMode(TextureAddressingMode mode) { mAddressMode = mode; }
        TextureAddressingMode getTextureAddressingMode() const { return mAddressMode; }
        void setTextureFiltering(int minFilter, int magFilter, int mipFilter)
        { (void)minFilter; (void)magFilter; (void)mipFilter; }
        void setTextureScroll(Real u, Real v) { (void)u; (void)v; }
        void setTextureRotate(const Degree& angle) { (void)angle; }
        void setTexture(const TexturePtr& tex) { mTexture = tex; }
        void _setTexturePtr(const TexturePtr& tex) { mTexture = tex; }
        void setProjectiveTexturing(bool enabled, Camera* cam = 0) { (void)enabled; (void)cam; }
        ContentType getContentType() const { return mContentType; }
        const EffectMap& getEffects() const { return mEffects; }
        const String& getReferencedCompositorName() const { return mRefComp; }
        const String& getReferencedTextureName() const { return mRefTex; }
        size_t getReferencedMRTIndex() const { return 0; }
    private:
        String mTextureName;
        TexturePtr mTexture;
        Real mScaleU = 1.0f;
        Real mScaleV = 1.0f;
        TextureAddressingMode mAddressMode = TAM_WRAP;
        ContentType mContentType = CONTENT_NAMED;
        EffectMap mEffects;
        String mRefComp;
        String mRefTex;
    };

    class Pass
    {
    public:
        typedef std::vector<TextureUnitState>::const_iterator ConstTextureUnitStateIterator;
        Pass() : mVertexParams(std::make_shared<GpuProgramParameters>()), mFragmentParams(std::make_shared<GpuProgramParameters>())
        {
            mTexUnits.resize(1);
        }
        TextureUnitState* getTextureUnitState(ushort i)
        {
            if (i >= mTexUnits.size()) {
                mTexUnits.resize(i + 1);
            }
            return &mTexUnits[i];
        }
        ushort getNumTextureUnitStates() const { return static_cast<ushort>(mTexUnits.size()); }
        void setDepthCheckEnabled(bool enabled) { (void)enabled; }
        void setDepthWriteEnabled(bool enabled) { (void)enabled; }
        bool getDepthWriteEnabled() const { return true; }
        bool getDepthCheckEnabled() const { return true; }
        void setLightingEnabled(bool enabled) { (void)enabled; }
        bool getLightingEnabled() const { return false; }
        void setSceneBlending(int mode) { (void)mode; }
        void setPolygonMode(PolygonMode mode) { (void)mode; }
        TextureUnitState* createTextureUnitState()
        {
            mTexUnits.push_back(TextureUnitState());
            return &mTexUnits.back();
        }
        TextureUnitState* createTextureUnitState(const String& textureName)
        {
            TextureUnitState* tus = createTextureUnitState();
            tus->setTextureName(textureName);
            return tus;
        }
        void setVertexProgram(const String& name) { (void)name; }
        void setFragmentProgram(const String& name) { (void)name; }
        GpuProgramParametersSharedPtr getVertexProgramParameters() { return mVertexParams; }
        GpuProgramParametersSharedPtr getFragmentProgramParameters() { return mFragmentParams; }
    private:
        std::vector<TextureUnitState> mTexUnits;
        GpuProgramParametersSharedPtr mVertexParams;
        GpuProgramParametersSharedPtr mFragmentParams;
    };

    class Technique
    {
    public:
        Technique() { mPasses.resize(1); }
        Pass* getPass(ushort i)
        {
            if (i >= mPasses.size()) {
                mPasses.resize(i + 1);
            }
            return &mPasses[i];
        }
    private:
        std::vector<Pass> mPasses;
    };

    class Material : public Resource
    {
    public:
        Material() : Resource() { mTechniques.resize(1); }
        explicit Material(const String& name) : Resource(name) { mTechniques.resize(1); }
        bool isNull() const { return false; }
        bool isLoaded() const { return true; }
        void setNull() {}
        void load() {}
        void reload() {}
        MaterialPtr clone(const String& newName, bool changeGroup = true, const String& groupName = "General") const
        {
            (void)changeGroup;
            MaterialPtr m(new Material(newName));
            m->setGroup(groupName);
            return m;
        }
        Technique* getTechnique(ushort i)
        {
            if (i >= mTechniques.size()) {
                mTechniques.resize(i + 1);
            }
            return &mTechniques[i];
        }
        void setAmbient(Real r, Real g, Real b) { (void)r; (void)g; (void)b; }
        void setAmbient(const ColourValue& c) { (void)c; }
        void setDiffuse(Real r, Real g, Real b, Real a = 1.0f) { (void)r; (void)g; (void)b; (void)a; }
        void setSpecular(Real r, Real g, Real b, Real a = 1.0f) { (void)r; (void)g; (void)b; (void)a; }
        void setShininess(Real value) { (void)value; }
        void setDepthCheckEnabled(bool enabled) { (void)enabled; }
        void setDepthWriteEnabled(bool enabled) { (void)enabled; }
        void setDepthBias(Real constantBias, Real slopeScaleBias) { (void)constantBias; (void)slopeScaleBias; }
        void setLightingEnabled(bool enabled) { (void)enabled; }
        bool isReloadable() const { return true; }
    private:
        std::vector<Technique> mTechniques;
    };

    class Mesh : public Resource
    {
    public:
        class SubMesh
        {
        public:
            bool useSharedVertices = true;
            VertexData* vertexData = 0;
            IndexData* indexData = new IndexData();
            void setMaterialName(const String& name) { mMaterialName = name; }
            const String& getMaterialName() const { return mMaterialName; }
            ~SubMesh() { delete indexData; }
        private:
            String mMaterialName;
        };

        Mesh() : Resource() {}
        explicit Mesh(const String& name) : Resource(name) {}
        MeshPtr clone(const String& newName, const String& group)
        {
            MeshPtr m(new Mesh(newName));
            m->setGroup(group);
            return m;
        }
        void _setBounds(const AxisAlignedBox& box, bool pad) { (void)box; (void)pad; }
        void _setBoundingSphereRadius(Real r) { mBoundingSphereRadius = r; }
        Real getBoundingSphereRadius() const { return mBoundingSphereRadius; }
        void load() {}
        SubMesh* createSubMesh()
        {
            mSubMeshes.push_back(std::unique_ptr<SubMesh>(new SubMesh()));
            return mSubMeshes.back().get();
        }
        bool suggestTangentVectorBuildParams(VertexElementSemantic semantic, unsigned short& sourceCoordSet, unsigned short& index)
        {
            (void)semantic;
            sourceCoordSet = 0;
            index = 0;
            return false;
        }
        void buildTangentVectors(VertexElementSemantic semantic, unsigned short sourceCoordSet = 0, unsigned short index = 0)
        {
            (void)semantic;
            (void)sourceCoordSet;
            (void)index;
        }
        SubMesh* getSubMesh(unsigned short i)
        {
            if (i >= mSubMeshes.size()) {
                mSubMeshes.resize(i + 1);
                for (size_t idx = 0; idx < mSubMeshes.size(); ++idx) {
                    if (!mSubMeshes[idx]) {
                        mSubMeshes[idx] = std::unique_ptr<SubMesh>(new SubMesh());
                    }
                }
            }
            return mSubMeshes[i].get();
        }
        unsigned short getNumSubMeshes() const { return static_cast<unsigned short>(mSubMeshes.size()); }
        VertexData* sharedVertexData = 0;
    private:
        std::vector<std::unique_ptr<SubMesh> > mSubMeshes;
        Real mBoundingSphereRadius = 0.0f;
    };

    typedef Mesh::SubMesh SubMesh;

    class ShadowCameraSetup
    {
    public:
        virtual ~ShadowCameraSetup() {}
    };

    class DefaultShadowCameraSetup : public ShadowCameraSetup
    {
    public:
        DefaultShadowCameraSetup() {}
    };

    typedef SharedPtr<ShadowCameraSetup> ShadowCameraSetupPtr;

    class GpuProgramManager
    {
    public:
        static GpuProgramManager& getSingleton() { static GpuProgramManager m; return m; }
        bool isSyntaxSupported(const String& syntax) const
        {
            (void)syntax;
            return false;
        }
    };

    enum ShadowTechnique
    {
        SHADOWTYPE_TEXTURE_MODULATIVE = 1
    };

    enum SceneBlendType
    {
        SBT_TRANSPARENT_ALPHA = 1,
        SBT_TRANSPARENT_COLOUR = 2
    };

    enum FogMode
    {
        FOG_NONE = FO_NONE,
        FOG_LINEAR = FO_LINEAR,
        FOG_EXP = FO_POINT
    };

    class ParticleSystemFactory
    {
    public:
        static const String FACTORY_TYPE_NAME;
    };
    inline const String ParticleSystemFactory::FACTORY_TYPE_NAME = "ParticleSystem";

    class ParticleSystemManager
    {
    public:
        static ParticleSystemManager& getSingleton() { static ParticleSystemManager m; return m; }
        void removeAllTemplates() {}
    };


    class MeshManager
    {
    public:
        static MeshManager& getSingleton() { static MeshManager m; return m; }
        MeshPtr createManual(const String& name, const String& group)
        {
            MeshPtr m(new Mesh(name));
            m->setGroup(group);
            mMeshes[name] = m;
            return m;
        }
        MeshPtr getByName(const String& name)
        {
            std::map<String, MeshPtr>::iterator it = mMeshes.find(name);
            if (it != mMeshes.end()) {
                return it->second;
            }
            return MeshPtr();
        }
    private:
        std::map<String, MeshPtr> mMeshes;
    };

    class MaterialManager
    {
    public:
        typedef std::map<String, ResourcePtr> ResourceHandleMap;
        typedef ResourceHandleMap ResourceMapIterator;

        static MaterialManager& getSingleton() { static MaterialManager m; return m; }
        static MaterialManager* getSingletonPtr() { return &getSingleton(); }

        MaterialPtr create(const String& name, const String& group = "General")
        {
            MaterialPtr m(new Material(name));
            m->setGroup(group);
            mResources[name] = m;
            return m;
        }
        MaterialPtr getByName(const String& name)
        {
            ResourceHandleMap::iterator it = mResources.find(name);
            if (it != mResources.end()) {
                return MaterialPtr(it->second);
            }
            return MaterialPtr();
        }
        bool resourceExists(const String& name) const { return mResources.find(name) != mResources.end(); }
        void remove(const String& name) { mResources.erase(name); }
        ResourceMapIterator getResourceIterator() { return mResources; }

    private:
        ResourceHandleMap mResources;
    };

    class OverlayElementFactory
    {
    public:
        virtual ~OverlayElementFactory() {}
    };

    class PanelOverlayElementFactory : public OverlayElementFactory {};
    class BorderPanelOverlayElementFactory : public OverlayElementFactory {};
    class TextAreaOverlayElementFactory : public OverlayElementFactory {};

    class OverlayManager
    {
    public:
        static OverlayManager& getSingleton() { static OverlayManager om; return om; }
        static OverlayManager* getSingletonPtr() { return &getSingleton(); }
        void addOverlayElementFactory(OverlayElementFactory* factory)
        {
            mFactories.push_back(std::unique_ptr<OverlayElementFactory>(factory));
        }
        OverlayElement* createOverlayElement(const String& typeName, const String& instanceName)
        {
            OverlayElement* elem = typeName == "TextArea" ? static_cast<OverlayElement*>(new TextAreaOverlayElement())
                                                           : static_cast<OverlayElement*>(new PanelOverlayElement());
            elem->setName(instanceName);
            return elem;
        }
        void destroyOverlayElement(OverlayElement* e) { delete e; }
        Real getViewportWidth() const { return 640.0f; }
        Real getViewportHeight() const { return 480.0f; }
        void _queueOverlaysForRendering(Camera* camera, RenderQueue* queue, Viewport* vp)
        {
            (void)camera;
            (void)queue;
            (void)vp;
        }
    private:
        std::vector<std::unique_ptr<OverlayElementFactory> > mFactories;
    };

    class Font : public Resource
    {
    public:
        typedef uint32 CodePoint;
        struct UVRect
        {
            Real left;
            Real top;
            Real right;
            Real bottom;
            UVRect() : left(0.0f), top(0.0f), right(1.0f), bottom(1.0f) {}
        };

        Font(ResourceManager* creator, const String& name, ResourceHandle handle,
             const String& group, bool isManual = false, ManualResourceLoader* loader = 0)
            : Resource(name), mCreator(creator), mHandle(handle), mManual(isManual), mLoader(loader)
        {
            setGroup(group);
            mMaterial = MaterialManager::getSingleton().create(name + "/Material", group);
        }
        virtual ~Font() {}
        virtual void load() { loadImpl(); }
        virtual void loadImpl() {}
        Real getGlyphAspectRatio(CodePoint c) const { (void)c; return 0.5f; }
        UVRect getGlyphTexCoords(CodePoint c) const { (void)c; return UVRect(); }
        MaterialPtr getMaterial() const { return mMaterial; }
    protected:
        ResourceManager* mCreator;
        ResourceHandle mHandle;
        bool mManual;
        ManualResourceLoader* mLoader;
        MaterialPtr mMaterial;
    };
    class FontManager : public ResourceManager
    {
    public:
        static FontManager& getSingleton() { static FontManager fm; return fm; }
        static FontManager* getSingletonPtr() { return &getSingleton(); }
        FontPtr getByName(const String& name)
        {
            std::map<String, FontPtr>::iterator it = mFonts.find(name);
            if (it != mFonts.end()) {
                return it->second;
            }
            FontPtr f(new Font(this, name, 0, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, false, 0));
            mFonts[name] = f;
            return f;
        }
    protected:
        virtual Resource* createImpl(const String& name, ResourceHandle handle,
                                     const String& group, bool isManual, ManualResourceLoader* loader,
                                     const NameValuePairList* params)
        {
            (void)params;
            return new Font(this, name, handle, group, isManual, loader);
        }
    private:
        std::map<String, FontPtr> mFonts;
    };

    class OverlayProfileSessionListener {};

    enum
    {
        RSC_NON_POWER_OF_2_TEXTURES = 1
    };

    enum
    {
        RENDER_QUEUE_OVERLAY = 100
    };

    enum
    {
        MIP_UNLIMITED = -1,
        TU_DEFAULT = 0,
        TU_RENDERTARGET = 1
    };

    class Session
    {
    public:
        virtual ~Session() {}
    };

    class SessionCallback
    {
    public:
        virtual ~SessionCallback() {}
    };

    enum { MIP_DEFAULT = -1 };
}

#endif
