/*
 * PF File Loader - Implementation
 */

#include "PFLoader.h"
#include <stdio.h>
#include <string.h>

namespace PF
{
    Loader::Loader()
        : mFileHandle(NULL)
    {
    }
    
    Loader::~Loader()
    {
        if (mFileHandle)
        {
            fclose(mFileHandle);
            mFileHandle = NULL;
        }
    }
    
    bool Loader::init(const std::string& filename)
    {
        mFileName = filename;
        
        mFileHandle = fopen(filename.c_str(), "rb");
        if (!mFileHandle)
            return false;
        
        fseek(mFileHandle, 0, SEEK_END);
        uint32_t fileSize = ftell(mFileHandle);
        
        // Read the index offset at end of file
        fseek(mFileHandle, fileSize - 4, SEEK_SET);
        uint32_t from;
        fread(&from, 4, 1, mFileHandle);
        
        // Read number of entries
        fseek(mFileHandle, from, SEEK_SET);
        uint32_t elemCount;
        fread(&elemCount, 4, 1, mFileHandle);
        
        uint32_t filePos = from + 4;
        
        // Read all entries
        while (filePos < fileSize - 4)
        {
            PackedFileItem item;
            
            // Read name
            char name[256];
            int nameIdx = 0;
            char ch;
            while (1)
            {
                fread(&ch, 1, 1, mFileHandle);
                filePos++;
                if (ch == 0) break;
                name[nameIdx++] = ch;
            }
            name[nameIdx] = 0;
            item.Name = name;
            
            fread(&item.Next, 4, 1, mFileHandle);
            fread(&item.FileFolder, 4, 1, mFileHandle);
            filePos += 8;
            
            if (item.FileFolder == 0xFFFFFFFF)
            {
                // File
                filePos += 10;
                fseek(mFileHandle, filePos, SEEK_SET);
                fread(&item.Offset, 4, 1, mFileHandle);
                fread(&item.Length, 4, 1, mFileHandle);
                fread(&item.FileFolder, 2, 1, mFileHandle); // Actually "Smthing"
                filePos += 16;
            }
            else
            {
                // Folder
                item.Offset = 0;
                item.Length = 0;
                filePos += 2;
            }
            
            fileSystem.push_back(item);
        }
        
        return !fileSystem.empty();
    }
    
    bool Loader::loadFromMemory(void* data, uint32_t size)
    {
        if (!data || size < 4) return false;
        
        fileSystem.clear();
        
        uint8_t* bytes = (uint8_t*)data;
        
        uint32_t from = bytes[size - 4] | (bytes[size - 3] << 8) | 
                        (bytes[size - 2] << 16) | (bytes[size - 3] << 24);
        
        if (from >= size) from = 0;
        
        uint32_t elemCount = bytes[from] | (bytes[from + 1] << 8) | 
                            (bytes[from + 2] << 16) | (bytes[from + 3] << 24);
        
        uint32_t filePos = from + 4;
        
        for (uint32_t i = 0; i < elemCount && filePos < size - 4; i++)
        {
            PackedFileItem item;
            
            std::string name;
            while (filePos < size && bytes[filePos] != 0)
            {
                name += (char)bytes[filePos++];
            }
            filePos++;
            item.Name = name;
            
            if (filePos + 12 > size) break;
            
            item.Next = bytes[filePos] | (bytes[filePos + 1] << 8) | 
                        (bytes[filePos + 2] << 16) | (bytes[filePos + 3] << 24);
            filePos += 4;
            
            item.FileFolder = bytes[filePos] | (bytes[filePos + 1] << 8) | 
                              (bytes[filePos + 2] << 16) | (bytes[filePos + 3] << 24);
            filePos += 4;
            
            if (item.FileFolder == 0xFFFFFFFF)
            {
                filePos += 4;
                item.Offset = bytes[filePos] | (bytes[filePos + 1] << 8) | 
                              (bytes[filePos + 2] << 16) | (bytes[filePos + 3] << 24);
                filePos += 4;
                item.Length = bytes[filePos] | (bytes[filePos + 1] << 8) | 
                              (bytes[filePos + 2] << 16) | (bytes[filePos + 3] << 24);
                filePos += 4;
            }
            else
            {
                item.Offset = 0;
                item.Length = 0;
            }
            
            fileSystem.push_back(item);
        }
        
        return true;
    }
    
    bool Loader::getFile(const std::string& relativeDir, const std::string& file,
                         void* buffer, uint32_t* outSize)
    {
        uint32_t fileSize;
        uint32_t offset = findFile(relativeDir, file, fileSize);
        
        if (offset == 0 || !mFileHandle)
            return false;
        
        fseek(mFileHandle, offset, SEEK_SET);
        
        uint32_t toRead = outSize ? *outSize : fileSize;
        if (buffer)
        {
            fread(buffer, 1, toRead, mFileHandle);
        }
        
        if (outSize)
            *outSize = fileSize;
        
        return true;
    }
    
    uint32_t Loader::getFileSize(const std::string& relativeDir, const std::string& file) const
    {
        uint32_t size;
        uint32_t offset = findFile(relativeDir, file, size);
        return offset ? size : 0;
    }
    
    void Loader::listFiles(const std::string& path, std::vector<std::string>& files)
    {
        files.clear();
        
        uint32_t pathDepth = 0;
        for (size_t i = 0; i < path.size(); i++)
            if (path[i] == '/' || path[i] == '\\') pathDepth++;
        
        uint32_t folderStart = 0;
        if (!path.empty())
        {
            for (size_t i = 0; i < fileSystem.size(); i++)
            {
                if (fileSystem[i].FileFolder == 0xFFFFFFFF &&
                    fileSystem[i].Name == path)
                {
                    folderStart = i;
                    break;
                }
            }
        }
        
        for (size_t i = folderStart + 1; i < fileSystem.size(); i++)
        {
            if (fileSystem[i].FileFolder != 0xFFFFFFFF)
            {
                // Count depth
                uint32_t depth = 0;
                uint32_t parent = fileSystem[i].FileFolder;
                while (parent != folderStart && parent != 0xFFFFFFFF)
                {
                    depth++;
                    if (parent >= fileSystem.size()) break;
                    parent = fileSystem[parent].FileFolder;
                }
                
                if (depth == pathDepth + 1)
                {
                    files.push_back(fileSystem[i].Name);
                }
            }
        }
    }
    
    void Loader::listFolders(const std::string& path, std::vector<std::string>& folders)
    {
        folders.clear();
        
        uint32_t folderStart = 0;
        if (!path.empty())
        {
            for (size_t i = 0; i < fileSystem.size(); i++)
            {
                if (fileSystem[i].FileFolder != 0xFFFFFFFF &&
                    fileSystem[i].Name == path)
                {
                    folderStart = i;
                    break;
                }
            }
        }
        
        for (size_t i = folderStart + 1; i < fileSystem.size(); i++)
        {
            if (fileSystem[i].FileFolder != 0xFFFFFFFF)
            {
                folders.push_back(fileSystem[i].Name);
                // Skip to next folder
                while (i < fileSystem.size() && fileSystem[i].FileFolder != 0xFFFFFFFF)
                    i++;
            }
        }
    }
    
    uint32_t Loader::findFile(const std::string& relativeDir, const std::string& file, 
                              uint32_t& fileSize) const
    {
        fileSize = 0;
        
        if (fileSystem.empty())
            return 0;
        
        uint32_t startFolder = 0;
        if (!relativeDir.empty())
        {
            for (size_t i = 0; i < fileSystem.size(); i++)
            {
                if (fileSystem[i].FileFolder != 0xFFFFFFFF &&
                    fileSystem[i].Name == relativeDir)
                {
                    startFolder = i;
                    break;
                }
            }
        }
        
        uint32_t next = fileSystem[startFolder].Next;
        
        // Find folder first
        if (!relativeDir.empty())
        {
            while (next < fileSystem.size())
            {
                if (fileSystem[next].FileFolder != 0xFFFFFFFF &&
                    fileSystem[next].Name == relativeDir)
                {
                    startFolder = next;
                    break;
                }
                next = fileSystem[next].Next;
            }
        }
        
        // Now find the file
        next = startFolder;
        while (next < fileSystem.size())
        {
            if (fileSystem[next].FileFolder == 0xFFFFFFFF &&
                fileSystem[next].Name == file)
            {
                fileSize = fileSystem[next].Length;
                return fileSystem[next].Offset;
            }
            next = fileSystem[next].Next;
        }
        
        return 0;
    }
    
    std::string Loader::readString(FILE* stream, uint32_t& filePos)
    {
        std::string ret;
        char ch;
        while (1)
        {
            fread(&ch, 1, 1, stream);
            filePos++;
            if (ch == 0) break;
            ret += ch;
        }
        return ret;
    }
    
    // Helper functions
    bool isValidTexture(const std::string& filename)
    {
        const char* ext = strrchr(filename.c_str(), '.');
        if (!ext) return false;
        
        if (strcasecmp(ext, ".png") == 0) return true;
        if (strcasecmp(ext, ".jpg") == 0) return true;
        if (strcasecmp(ext, ".jpeg") == 0) return true;
        if (strcasecmp(ext, ".bmp") == 0) return true;
        if (strcasecmp(ext, ".tga") == 0) return true;
        
        return false;
    }
    
    const char* getTextureFormat(const uint8_t* data, uint32_t size)
    {
        if (size < 4) return "unknown";
        
        // PNG
        if (data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47)
            return "PNG";
        
        // JPEG
        if (data[0] == 0xFF && data[1] == 0xD8)
            return "JPEG";
        
        // BMP
        if (data[0] == 'B' && data[1] == 'M')
            return "BMP";
        
        // TGA
        if (data[2] == 2 || data[2] == 10)
            return "TGA";
        
        return "unknown";
    }
}
