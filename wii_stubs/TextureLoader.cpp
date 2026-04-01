/*
 * Texture Loader - Implementation
 */

#include "TextureLoader.h"
#include <string.h>

namespace Texture
{
    bool loadTGA(const uint8_t* tgaData, uint32_t size, LoadedTexture& texture)
    {
        if (!tgaData || size < sizeof(TGAHeader))
            return false;
        
        const TGAHeader* header = (const TGAHeader*)tgaData;
        
        // Only support uncompressed RGB/RGBA
        if (header->imageType != 2 && header->imageType != 10)
            return false;
        
        texture.width = header->width;
        texture.height = header->height;
        
        uint8_t bpp = header->bitsPerPixel;
        if (bpp != 24 && bpp != 32)
            return false;
        
        uint32_t bytesPerPixel = bpp / 8;
        uint32_t dataSize = texture.width * texture.height * bytesPerPixel;
        
        const uint8_t* imageData = tgaData + sizeof(TGAHeader) + header->idLength;
        
        // Allocate and convert (flip Y and convert BGR to RGB)
        texture.data = new uint8_t[dataSize];
        
        bool flipHorizontal = (header->imageDesc & 0x10) != 0;
        bool flipVertical = (header->imageDesc & 0x20) != 0;
        
        for (uint32_t y = 0; y < texture.height; y++)
        {
            uint32_t srcY = flipVertical ? (texture.height - 1 - y) : y;
            
            for (uint32_t x = 0; x < texture.width; x++)
            {
                uint32_t srcX = flipHorizontal ? (texture.width - 1 - x) : x;
                
                uint32_t srcIdx = (srcY * texture.width + srcX) * bytesPerPixel;
                uint32_t dstIdx = (y * texture.width + x) * bytesPerPixel;
                
                // BGR(A) -> RGB(A)
                texture.data[dstIdx + 0] = imageData[srcIdx + 2];
                texture.data[dstIdx + 1] = imageData[srcIdx + 1];
                texture.data[dstIdx + 2] = imageData[srcIdx + 0];
                
                if (bytesPerPixel == 4)
                    texture.data[dstIdx + 3] = imageData[srcIdx + 3];
                else
                    texture.data[dstIdx + 3] = 255;
            }
        }
        
        texture.size = dataSize;
        texture.isSwizzled = false;
        
        return true;
    }
    
    void swizzleTexture(uint8_t* dest, const uint8_t* src, uint16_t width, uint16_t height)
    {
        uint32_t blockSize = 8;
        uint32_t blockWidth = width / blockSize;
        
        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                uint32_t blockNum = (y / blockSize) * blockWidth + (x / blockSize);
                uint32_t blockOffset = ((y % blockSize) * blockSize + (x % blockSize)) * 4;
                
                uint32_t srcIdx = (y * width + x) * 4;
                uint32_t dstIdx = blockNum * blockSize * blockSize * 4 + blockOffset;
                
                dest[dstIdx + 0] = src[srcIdx + 0];
                dest[dstIdx + 1] = src[srcIdx + 1];
                dest[dstIdx + 2] = src[srcIdx + 2];
                dest[dstIdx + 3] = src[srcIdx + 3];
            }
        }
    }
    
    void convertToGXFormat(LoadedTexture& texture)
    {
        // Wii GX prefers swizzled textures in TMEM
        // This is a simple pass-through for now
        // Real implementation would swizzle and convert to GX_TF_* format
    }
}
