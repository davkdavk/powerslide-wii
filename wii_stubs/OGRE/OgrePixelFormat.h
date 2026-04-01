#ifndef OGREPIXELFORMAT_H
#define OGREPIXELFORMAT_H

#include "OgrePrerequisites.h"

namespace Ogre
{
    enum PixelFormat
    {
        PF_UNKNOWN = 0,
        PF_L8,
        PF_LA8,
        PF_R8G8B8,
        PF_B8G8R8,
        PF_R8G8B8A8,
        PF_B8G8R8A8,
        PF_BYTE_RGB,
        PF_BYTE_RGBA,
        PF_BYTE_BGR,
        PF_BYTE_BGRA
    };
    
    struct PixelUtil
    {
        static size_t getMemorySize(size_t width, size_t height, size_t depth, PixelFormat format)
        {
            return width * height * depth * 4;
        }
    };
}

#endif
