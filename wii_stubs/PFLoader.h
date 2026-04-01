/*
 * PF File Loader - Reads packed file format used by Powerslide
 */

#ifndef PFLOADER_H
#define PFLOADER_H

#include <string>
#include <vector>
#include <cstdint>

namespace PF
{
    struct PackedFileItem
    {
        std::string Name;
        uint32_t Next;
        uint32_t FileFolder;
        uint32_t Offset;
        uint32_t Length;
    };
    
    class Loader
    {
    public:
        Loader();
        ~Loader();
        
        bool init(const std::string& filename);
        
        bool loadFromMemory(void* data, uint32_t size);
        
        bool getFile(const std::string& relativeDir, const std::string& file, 
                     void* buffer, uint32_t* size);
        
        uint32_t getFileSize(const std::string& relativeDir, const std::string& file) const;
        
        void listFiles(const std::string& path, std::vector<std::string>& files);
        void listFolders(const std::string& path, std::vector<std::string>& folders);
        
        bool isLoaded() const { return !fileSystem.empty(); }
        
        uint32_t getItemCount() const { return fileSystem.size(); }
        
    private:
        uint32_t findFile(const std::string& relativeDir, const std::string& file, 
                          uint32_t& fileSize) const;
        
        std::string readString(FILE* stream, uint32_t& filePos);
        
        std::string mFileName;
        std::vector<PackedFileItem> fileSystem;
        FILE* mFileHandle;
    };
    
    typedef Loader PFLoader;
    
    bool isValidTexture(const std::string& filename);
    const char* getTextureFormat(const uint8_t* data, uint32_t size);
}

#endif
