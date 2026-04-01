
#include <set>
#include <algorithm>
#include <cstdint>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>

#if defined(_WIN32)
#include <direct.h>
#else
#include <unistd.h>
#endif

#if defined(WII) || defined(__wii__)
#include <fat.h>
#endif

#if defined(WII) || defined(__wii__)
#include <ogc/system.h>
#include "../WiiDebugLog.h"
#endif

#include "PFLoader.h"

#include "../WiiMemoryBudget.h"

#include "../tools/Tools.h"


const std::string PFLoader::mPackedFileSubdir = "";

namespace
{
    class NullStubDataStream : public Ogre::DataStream
    {
    public:
        virtual bool isReadable() const { return true; }
        virtual size_t size() const { return 0; }
        virtual size_t tell() const { return 0; }
        virtual size_t read(void*, size_t) { return 0; }
    };

    class WiiAlignedDataStream : public Ogre::DataStream
    {
    public:
        WiiAlignedDataStream(void* data, size_t dataSize)
            : mData(static_cast<unsigned char*>(data)), mSize(dataSize), mPos(0)
        {
        }

        virtual ~WiiAlignedDataStream()
        {
            close();
        }

        virtual bool isReadable() const { return mData != NULL; }
        virtual bool isWriteable() const { return false; }
        virtual size_t size() const { return mSize; }
        virtual size_t tell() const { return mPos; }

        virtual size_t read(void* buf, size_t count)
        {
            if(!mData || !buf || count == 0 || mPos >= mSize)
                return 0;

            const size_t remaining = mSize - mPos;
            const size_t toRead = remaining < count ? remaining : count;
            std::memcpy(buf, mData + mPos, toRead);
            mPos += toRead;
            return toRead;
        }

        virtual size_t write(const void*, size_t) { return 0; }

        virtual void close()
        {
            if(mData)
            {
                Wii_Free(mData);
                mData = NULL;
            }
            mSize = 0;
            mPos = 0;
        }

        virtual void seek(size_t pos)
        {
            mPos = (pos < mSize) ? pos : mSize;
        }

    private:
        unsigned char* mData;
        size_t mSize;
        size_t mPos;
    };

    static Ogre::DataStreamPtr makeNullStubStream()
    {
        return Ogre::DataStreamPtr(new NullStubDataStream());
    }

    static std::string normalizeInternalPath(const std::string& relativeDir, const std::string& file)
    {
        std::string internal;
        if(!relativeDir.empty())
            internal = relativeDir + "/";
        internal += file;

        for(size_t i = 0; i < internal.size(); ++i)
        {
            if(internal[i] == '\\')
                internal[i] = '/';
        }

        while(!internal.empty() && (internal[0] == '/' || internal[0] == '.'))
            internal.erase(internal.begin());

        while(internal.find("//") != std::string::npos)
            internal.erase(internal.find("//"), 1);

        return internal;
    }

    static std::string mapTextureExtensionToTPL(const std::string& path)
    {
        if(path.size() < 4)
            return path;

        std::string ext = path.substr(path.size() - 4);
        for(size_t i = 0; i < ext.size(); ++i)
            ext[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(ext[i])));

        if(ext == ".tga" || ext == ".bmp")
            return path.substr(0, path.size() - 4) + ".tpl";

        return path;
    }

    static bool isTrackOrTerrainRequest(const std::string& relativeDir, const std::string& file)
    {
        const bool trackDir = relativeDir.find("data/tracks/") != std::string::npos;
        const bool terrainFile =
            file.find("terrain") != std::string::npos ||
            file.find("laptrack") != std::string::npos ||
            file.find("partgraph") != std::string::npos;
        return trackDir || terrainFile;
    }

    static Ogre::DataStreamPtr openLooseWiiDataFile(const std::string& relativeDir, const std::string& file)
    {
        const std::string internalPath = normalizeInternalPath(relativeDir, file);
        std::string mappedPath = mapTextureExtensionToTPL(internalPath);
        const std::string fullPath = "sd:/powerslide/wii_data/" + mappedPath;

#if defined(WII) || defined(__wii__)
        WiiDebugLog("[WII_DATA] open '%s'\n", fullPath.c_str());
#endif

        FILE* fp = std::fopen(fullPath.c_str(), "rb");
        if(!fp)
        {
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[WII_DATA] missing '%s'\n", fullPath.c_str());
#endif
            return Ogre::DataStreamPtr();
        }

        if(std::fseek(fp, 0, SEEK_END) != 0)
        {
            std::fclose(fp);
            return Ogre::DataStreamPtr();
        }

        const long endPos = std::ftell(fp);
        if(endPos < 0)
        {
            std::fclose(fp);
            return Ogre::DataStreamPtr();
        }
        const size_t fileSize = static_cast<size_t>(endPos);

        if(!isTrackOrTerrainRequest(relativeDir, file) && fileSize > (512u * 1024u))
        {
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[WII_DATA] budget gate '%s' size=%u -> NullStub\n", fullPath.c_str(), static_cast<unsigned int>(fileSize));
#endif
            std::fclose(fp);
            return makeNullStubStream();
        }

        // WII_NATIVE_ASSET_PIPELINE: allow all files through as loose files
        // No size gate for vehicle/cars/objects/physics files

        if(std::fseek(fp, 0, SEEK_SET) != 0)
        {
            std::fclose(fp);
            return Ogre::DataStreamPtr();
        }

        void* mem = Wii_Alloc(fileSize, 32);
        if(!mem)
        {
#if defined(WII) || defined(__wii__)
            WiiDebugLog("[WII_DATA] alloc failed '%s' size=%u\n", fullPath.c_str(), static_cast<unsigned int>(fileSize));
#endif
            std::fclose(fp);
            return Ogre::DataStreamPtr();
        }

        const size_t got = std::fread(mem, 1, fileSize, fp);
        std::fclose(fp);
        if(got != fileSize)
        {
            Wii_Free(mem);
            return Ogre::DataStreamPtr();
        }

#if defined(WII) || defined(__wii__)
        WiiDebugLog("[WII_DATA] loaded '%s' size=%u mem_used=%u\n",
            fullPath.c_str(),
            static_cast<unsigned int>(fileSize),
            static_cast<unsigned int>(Wii_GetAllocatedBytes()));
#endif

        return Ogre::DataStreamPtr(new WiiAlignedDataStream(mem, fileSize));
    }

    class RawFileDataStream : public Ogre::DataStream
    {
    public:
        RawFileDataStream(FILE* fp, size_t fileSize)
            : mFile(fp), mSize(fileSize), mPos(0)
        {
        }

        virtual ~RawFileDataStream()
        {
            close();
        }

        virtual bool isReadable() const { return mFile != NULL; }
        virtual bool isWriteable() const { return false; }
        virtual size_t size() const { return mSize; }
        virtual size_t tell() const { return mPos; }

        virtual size_t read(void* buf, size_t count)
        {
            if(!mFile || !buf || count == 0)
                return 0;

            const size_t got = std::fread(buf, 1, count, mFile);
            mPos += got;
            return got;
        }

        virtual size_t write(const void*, size_t)
        {
            return 0;
        }

        virtual Ogre::String readLine(char* buf, size_t maxCount, const Ogre::String&)
        {
            Ogre::String ret;
            if(!buf || maxCount == 0)
                return ret;

            size_t out = 0;
            while(out + 1 < maxCount)
            {
                char ch = 0;
                if(read(&ch, 1) != 1)
                    break;
                if(ch == '\n')
                    break;
                buf[out++] = ch;
                ret.push_back(ch);
            }
            buf[out] = '\0';
            return ret;
        }

        virtual void close()
        {
            if(mFile)
            {
                std::fclose(mFile);
                mFile = NULL;
            }
            mPos = 0;
            mSize = 0;
        }

        virtual Ogre::String getAsString()
        {
            Ogre::String out;
            if(!mFile)
                return out;

            const long oldPos = std::ftell(mFile);
            std::fseek(mFile, 0, SEEK_SET);
            out.resize(mSize);
            if(mSize > 0)
            {
                const size_t got = std::fread(&out[0], 1, mSize, mFile);
                if(got < mSize)
                    out.resize(got);
            }
            if(oldPos >= 0)
            {
                std::fseek(mFile, oldPos, SEEK_SET);
                mPos = static_cast<size_t>(oldPos);
            }
            return out;
        }

        virtual void seek(size_t pos)
        {
            if(!mFile)
                return;

            std::fseek(mFile, static_cast<long>(pos), SEEK_SET);
            mPos = pos;
        }

    private:
        FILE* mFile;
        size_t mSize;
        size_t mPos;
    };
}

namespace
{
    static void appendWiiPFTrace(const char* stage, const std::string& path)
    {
#if defined(WII) || defined(__wii__)
        std::FILE* trace = std::fopen("pfloader_trace.txt", "a");
        if(trace)
        {
            std::fprintf(trace, "[PFLoader] %s %s\n", stage, path.c_str());
            std::fclose(trace);
        }
#else
        (void)stage;
        (void)path;
#endif
    }

    static Ogre::DataStreamPtr openRawFile(const std::string& path)
    {
#if defined(WII) || defined(__wii__)
        WiiDebugLog("[PF] fopen path='%s'\n", path.c_str());
#endif

        FILE* fp = std::fopen(path.c_str(), "rb");

#if defined(WII) || defined(__wii__)
        WiiDebugLog("[PF] fopen result path='%s' ok=%d\n", path.c_str(), fp ? 1 : 0);
#endif

        if(!fp)
            return Ogre::DataStreamPtr();

        if(std::fseek(fp, 0, SEEK_END) != 0)
        {
            std::fclose(fp);
            return Ogre::DataStreamPtr();
        }

        const long endPos = std::ftell(fp);
        if(endPos < 0)
        {
            std::fclose(fp);
            return Ogre::DataStreamPtr();
        }

        if(std::fseek(fp, 0, SEEK_SET) != 0)
        {
            std::fclose(fp);
            return Ogre::DataStreamPtr();
        }

        return Ogre::DataStreamPtr(new RawFileDataStream(fp, static_cast<size_t>(endPos)));
    }

    static bool readLE16(const Ogre::DataStreamPtr& stream, std::uint16_t& value)
    {
        unsigned char bytes[2] = {0, 0};
        if (!stream.get() || stream->read(bytes, 2) != 2)
        {
            value = 0;
            return false;
        }
        value = static_cast<std::uint16_t>(bytes[0] | (bytes[1] << 8));
        return true;
    }

    static bool readLE32(const Ogre::DataStreamPtr& stream, std::uint32_t& value)
    {
        unsigned char bytes[4] = {0, 0, 0, 0};
        if (!stream.get() || stream->read(bytes, 4) != 4)
        {
            value = 0;
            return false;
        }
        value = static_cast<std::uint32_t>(
            static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8) |
            (static_cast<std::uint32_t>(bytes[2]) << 16) |
            (static_cast<std::uint32_t>(bytes[3]) << 24));
        return true;
    }

    static std::string makeAbsolutePath(const std::string& path)
    {
        if(path.empty())
            return path;

        const size_t colonPos = path.find(':');
        const size_t slashPos = path.find('/');
        if(colonPos != std::string::npos && (slashPos == std::string::npos || colonPos < slashPos))
            return path;

        if(path[0] == '/')
            return path;

        if(path.size() > 1 && path[1] == ':')
            return path;

        char cwdBuf[1024];
        std::memset(cwdBuf, 0, sizeof(cwdBuf));
#if defined(_WIN32)
        if(_getcwd(cwdBuf, static_cast<int>(sizeof(cwdBuf) - 1)) != NULL)
#else
        if(getcwd(cwdBuf, sizeof(cwdBuf) - 1) != NULL)
#endif
        {
            return std::string(cwdBuf) + "/" + path;
        }

        return path;
    }

    static void appendSourceRootCandidates(std::vector<std::string>& out, const std::string& fileName)
    {
#if defined(WII) || defined(__wii__)
        std::string fileMacro = __FILE__;
        const std::string needle = "/orig_src/loaders/PFLoader.cpp";
        size_t pos = fileMacro.find(needle);
        if(pos != std::string::npos)
        {
            std::string root = fileMacro.substr(0, pos);
            out.push_back(root + "/wii_build/" + fileName);
            out.push_back(root + "/wii_build/powerslide/" + fileName);
        }
#else
        (void)out;
        (void)fileName;
#endif
    }
}


PFLoader::PFLoader()
    : mFileName("data.pf")
{ }

Ogre::DataStreamPtr Wii_SafeOpen(const std::string& relativeDir, const std::string& file)
{
#if defined(WII) || defined(__wii__)
#if defined(WII_NATIVE_ASSET_PIPELINE)
    return openLooseWiiDataFile(relativeDir, file);
#else
    (void)relativeDir;
    (void)file;
    return Ogre::DataStreamPtr();
#endif
#else
    (void)relativeDir;
    (void)file;
    return Ogre::DataStreamPtr();
#endif
}

Ogre::DataStreamPtr PFLoader::openPackedStream() const
{
    Ogre::DataStreamPtr ret;

    if(mDataDir.empty())
    {
        ret = Ogre::ResourceGroupManager::getSingleton().openResource(mFileName.c_str(), "PF");
    }
    else if(!mResolvedFilePath.empty())
    {
        ret = openRawFile(mResolvedFilePath);
    }

    return ret;
}

bool PFLoader::init(const std::string& file, const std::string& dataDir)
{
    bool res = false;

    mFileName = file;
    mDataDir = dataDir;
    mResolvedFilePath.clear();

#if defined(WII) || defined(__wii__)
#if defined(WII_NATIVE_ASSET_PIPELINE)
    fileSystem.clear();
    fileSystem.push_back(PF::PackedFileItem(".", 1, 0));
    WiiDebugLog("[WII_DATA] PF init bypass '%s' -> loose files in sd:/powerslide/wii_data\n", mFileName.c_str());
    return true;
#endif
#endif

    Ogre::DataStreamPtr stream;
    if(mDataDir.empty())
    {
#if defined(WII) || defined(__wii__)
        appendWiiPFTrace("try", "resource://PF/" + mFileName);
#endif
        stream = Ogre::ResourceGroupManager::getSingleton().openResource( mFileName.c_str(), "PF" );

#if defined(WII) || defined(__wii__)
        appendWiiPFTrace(stream.get() && stream->isReadable() ? "open_ok" : "open_fail", "resource://PF/" + mFileName);
#endif
    }
    else
    {
        std::vector<std::string> candidates;

        const auto pushCandidate = [&candidates](const std::string& path)
        {
            if(path.empty())
                return;
            if(std::find(candidates.begin(), candidates.end(), path) == candidates.end())
                candidates.push_back(path);
        };

        if(!mPackedFileSubdir.empty())
            pushCandidate(mDataDir + "/" + mPackedFileSubdir + "/" + mFileName);

        pushCandidate(mDataDir + "/" + mFileName);

#if defined(WII) || defined(__wii__)
        pushCandidate("sd:/" + mFileName);
        pushCandidate("sd:/powerslide/" + mFileName);
        pushCandidate("sd:/Powerslide/" + mFileName);
        pushCandidate("sd:/apps/powerslide/" + mFileName);
        pushCandidate("sd:/apps/Powerslide/" + mFileName);
        pushCandidate("sd:/apps/powerslide-remake/" + mFileName);
        pushCandidate("sd:/apps/PowerslideRemake/" + mFileName);

        WiiDebugLog("[PF] init begin file='%s' dataDir='%s' candidates=%u\n",
            mFileName.c_str(),
            mDataDir.c_str(),
            static_cast<unsigned int>(candidates.size()));

        if(!mDataDir.empty())
        {
            pushCandidate(mDataDir + "/powerslide/" + mFileName);
            pushCandidate(mDataDir + "/Powerslide/" + mFileName);
            pushCandidate(mDataDir + "/apps/powerslide/" + mFileName);
            pushCandidate(mDataDir + "/apps/Powerslide/" + mFileName);
            pushCandidate(mDataDir + "/apps/powerslide-remake/" + mFileName);
            pushCandidate(mDataDir + "/apps/PowerslideRemake/" + mFileName);
        }
#endif

        for(size_t i = 0; i < candidates.size() && !stream.get(); ++i)
        {
            const std::string& fullPath = candidates[i];
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[PFLoader::init]: Trying file path " + Ogre::String(fullPath.c_str()));

#if defined(WII) || defined(__wii__)
            WiiDebugLog("[PF] init try[%u/%u] '%s'\n",
                static_cast<unsigned int>(i + 1),
                static_cast<unsigned int>(candidates.size()),
                fullPath.c_str());
            appendWiiPFTrace("try", fullPath);
#endif

            Ogre::DataStreamPtr streamtmp = openRawFile(fullPath);
            if(streamtmp.get() && streamtmp->isReadable())
            {
#if defined(WII) || defined(__wii__)
                appendWiiPFTrace("open_ok", fullPath);
                WiiDebugLog("[PF] init open_ok '%s'\n", fullPath.c_str());
#endif
                mResolvedFilePath = makeAbsolutePath(fullPath);
#if defined(WII) || defined(__wii__)
                WiiDebugLog("[PF] init resolved '%s'\n", mResolvedFilePath.c_str());
#endif
                stream = streamtmp;
            }
            else
            {
#if defined(WII) || defined(__wii__)
                appendWiiPFTrace("open_fail", fullPath);
                WiiDebugLog("[PF] init open_fail '%s'\n", fullPath.c_str());
#endif
            }
        }

        if(!stream.get())
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[PFLoader::init]: Failed to open any path for " + Ogre::String(mFileName.c_str()));
        }
    }

    if(stream.get() && stream->isReadable())
    {
#if defined(WII) || defined(__wii__)
        if(mFileName == "data.pf")
        {
            Ogre::gAssetsLoaded = true;
        }
#endif

        fileSystem.clear();

        //read file system
        {
            res = false;

            DWORD From;
            DWORD ElemCount;
            size_t Size;

            Size = stream->size();
            if(Size < 8)
            {
                stream->close();
                return false;
            }

            stream->seek(Size - 4);
            {
                std::uint32_t fromLE;
                if(!readLE32(stream, fromLE))
                {
                    stream->close();
                    return false;
                }
                From = static_cast<DWORD>(fromLE);
            }
            if(From >= Size - 4)
            {
                stream->close();
                return false;
            }

            stream->seek(From);
            {
                std::uint32_t elemCountLE;
                if(!readLE32(stream, elemCountLE))
                {
                    stream->close();
                    return false;
                }
                ElemCount = static_cast<DWORD>(elemCountLE);
            }

            DWORD FilePos=From+4;

            DWORD parsedItems = 0;
            while(FilePos < Size - 4)
            {
                if(parsedItems > ElemCount + 4096)
                    break;

                DWORD Next,F_F,Offset,Length;

                std::string itemName = readString(stream, FilePos);
                if(itemName.empty() && FilePos >= Size - 4)
                    break;

                {
                    std::uint32_t nextLE;
                    std::uint32_t folderLE;
                    if(!readLE32(stream, nextLE) || !readLE32(stream, folderLE))
                        break;
                    Next = static_cast<DWORD>(nextLE);
                    F_F = static_cast<DWORD>(folderLE);
                }
                FilePos+=8;
                if(F_F==0xffffffff)//file
                {
                    {
                        std::uint32_t offsetLE;
                        std::uint32_t lengthLE;
                        std::uint16_t smthingLE;
                        if(!readLE32(stream, offsetLE) || !readLE32(stream, lengthLE) || !readLE16(stream, smthingLE))
                            break;
                        Offset = static_cast<DWORD>(offsetLE);
                        Length = static_cast<DWORD>(lengthLE);
                        (void)smthingLE;
                    }
                    FilePos+=10;
                    fileSystem.push_back(PF::PackedFileItem(itemName,Next,Offset,Length));
                }
                else                // folder
                {
                    fileSystem.push_back(PF::PackedFileItem(itemName,Next,F_F));
                }

                ++parsedItems;
            }

            res = !fileSystem.empty();

        }

        stream->close();
    }

    return res;
}

Ogre::DataStreamPtr PFLoader::getFile(const std::string& relativeDir, const std::string& file) const
{
    Ogre::DataStreamPtr ret;

#if defined(WII) || defined(__wii__)
#if defined(WII_NATIVE_ASSET_PIPELINE)
    return Wii_SafeOpen(relativeDir, file);
#endif
#endif

    if(!fileSystem.empty())
    {
#if defined(WII) || defined(__wii__)
        if(file == "laptrack.lpf" || file == "LAPTRACK.LPF" || file == "laptrack.LPF" || file == "powerslide.str")
        {
            WiiDebugLog("[PF] getFile req dir='%s' file='%s' fs=%u\n", relativeDir.c_str(), file.c_str(), static_cast<unsigned int>(fileSystem.size()));
        }
#endif
        ret = openPackedStream();

        if(ret.get() && ret->isReadable())
        {
            //find file offset
            size_t fileSize;
            size_t offset = findFile(relativeDir, file, fileSize);
            if(offset != 0)
            {
                ret->seek(offset);
            }
            else
            {
#if defined(WII) || defined(__wii__)
                if(file == "laptrack.lpf" || file == "LAPTRACK.LPF" || file == "laptrack.LPF" || file == "powerslide.str")
                {
                    WiiDebugLog("[PF] getFile unresolved dir='%s' file='%s'\n", relativeDir.c_str(), file.c_str());
                }
#endif
                ret->close();
                ret = Ogre::DataStreamPtr();
            }
        }
#if defined(WII) || defined(__wii__)
        else if(file == "laptrack.lpf" || file == "LAPTRACK.LPF" || file == "laptrack.LPF" || file == "powerslide.str")
        {
            WiiDebugLog("[PF] getFile no stream dir='%s' file='%s'\n", relativeDir.c_str(), file.c_str());
        }
#endif
    }
    return ret;
}

size_t PFLoader::getFileSize(const std::string& relativeDir, const std::string& file) const
{
    size_t ret = 0;

#if defined(WII) || defined(__wii__)
#if defined(WII_NATIVE_ASSET_PIPELINE)
    Ogre::DataStreamPtr s = Wii_SafeOpen(relativeDir, file);
    if(s.get() && s->isReadable())
    {
        ret = s->size();
        s->close();
    }
    return ret;
#endif
#endif

    if(!fileSystem.empty())
    {
        //find file size
        size_t fileSize;
        size_t offset = findFile(relativeDir, file, fileSize);
        if(offset != 0)
        {
            ret = fileSize;
        }
    }

    return ret;
}

size_t PFLoader::findFile(const std::string& relativeDir, const std::string& file, size_t& fileSize) const
{
    size_t res = 0;
    fileSize = 0;

    if(fileSystem.empty())
        return 0;

    std::set<char> delims;
    delims.insert('\\');
    delims.insert('/');

    std::vector<std::string> vPath = Tools::splitpath(relativeDir, delims);

    if(relativeDir == "") vPath.clear();

    while(!vPath.empty() && vPath.front() == ".")
        vPath.erase(vPath.begin());

#if defined(WII) || defined(__wii__)
    const bool traceLLT =
        (file == "laptrack.lpf" || file == "LAPTRACK.LPF" || file == "laptrack.LPF" || file == "powerslide.str") &&
        (relativeDir.find("tracks") != std::string::npos || relativeDir.empty());
    if(traceLLT)
    {
        WiiDebugLog("[PF] findFile dir='%s' file='%s' parts=%u root='%s' next=%u\n",
            relativeDir.c_str(),
            file.c_str(),
            static_cast<unsigned int>(vPath.size()),
            fileSystem.empty() ? "" : fileSystem[0].Name.c_str(),
            fileSystem.empty() ? 0u : static_cast<unsigned int>(fileSystem[0].Next));
    }
#endif

    const auto nameEquals = [](const std::string& a, const std::string& b) -> bool
    {
        if(a.size() != b.size())
            return false;
        for(size_t i = 0; i < a.size(); ++i)
        {
            char ca = a[i];
            char cb = b[i];
            if(ca >= 'A' && ca <= 'Z') ca = static_cast<char>(ca - 'A' + 'a');
            if(cb >= 'A' && cb <= 'Z') cb = static_cast<char>(cb - 'A' + 'a');
            if(ca != cb)
                return false;
        }
        return true;
    };

    DWORD next = 0;
    if(fileSystem[0].Name == ".")
        next = fileSystem[0].Next;

    for(size_t q = 0; q < vPath.size(); ++q)
    {
        bool foundDir = false;
        DWORD probe = next;
        while(probe < fileSystem.size())
        {
            if(fileSystem[probe].FileFolder != 0xFFFFFFFF && nameEquals(fileSystem[probe].Name, vPath[q]))
            {
                next = fileSystem[probe].FileFolder;
                foundDir = true;
                break;
            }
            probe = fileSystem[probe].Next;
        }
        if(!foundDir)
        {
#if defined(WII) || defined(__wii__)
            if(traceLLT)
            {
                WiiDebugLog("[PF] missing path part '%s'\n", vPath[q].c_str());
            }
#endif
            return 0;
        }
    }

    DWORD probe = next;
    while(probe < fileSystem.size())
    {
        if(fileSystem[probe].FileFolder == 0xFFFFFFFF && nameEquals(fileSystem[probe].Name, file))
        {
            res = fileSystem[probe].Offset;
            fileSize = fileSystem[probe].Length;
#if defined(WII) || defined(__wii__)
            if(traceLLT)
            {
                WiiDebugLog("[PF] found file '%s' offset=%u size=%u\n",
                    fileSystem[probe].Name.c_str(),
                    static_cast<unsigned int>(res),
                    static_cast<unsigned int>(fileSize));
            }
#endif
            break;
        }
        probe = fileSystem[probe].Next;
    }

#if defined(WII) || defined(__wii__)
    if(traceLLT && res == 0)
    {
        WiiDebugLog("[PF] file not found in final chain for '%s'\n", file.c_str());
    }
#endif

    return res;
}

std::string PFLoader::readString(const Ogre::DataStreamPtr& stream, DWORD& FilePos)
{
    std::string ret;
    char ch = 0;
    while(true)
    {
        if(!stream.get() || stream->read(&ch,1) != 1)
            break;
        FilePos++;
        if(ch == 0)
            break;
        ret.push_back(ch);
    }

    return ret;
}
