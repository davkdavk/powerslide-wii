/*
 * Texture Loader - TGA format for Wii GX
 */

#ifndef TEXTURELOADER_H
#define TEXTURELOADER_H

#include <cstdint>
#include <cstddef>

namespace Texture
{
    struct TGAHeader
    {
        uint8_t idLength;
        uint8_t colorMapType;
        uint8_t imageType;
        uint8_t colorMapSpec[5];
        uint16_t xOrigin;
        uint16_t yOrigin;
        uint16_t width;
        uint16_t height;
        uint8_t bitsPerPixel;
        uint8_t imageDesc;
    };
    
    struct LoadedTexture
    {
        uint16_t width;
        uint16_t height;
        uint8_t* data;
        uint32_t size;
        bool isSwizzled;
        
        LoadedTexture() : width(0), height(0), data(NULL), size(0), isSwizzled(false) {}
        ~LoadedTexture() { if (data) delete[] data; }
    };
    
    bool loadTGA(const uint8_t* tgaData, uint32_t size, LoadedTexture& texture);
    void convertToGXFormat(LoadedTexture& texture);
    void swizzleTexture(uint8_t* dest, const uint8_t* src, uint16_t width, uint16_t height);
}

#endif
