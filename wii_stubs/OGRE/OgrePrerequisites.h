#ifndef OGREPREREQUISITES_H
#define OGREPREREQUISITES_H

#include <string>
#include <vector>

#include "OgreSceneManager.h"

namespace Ogre
{
    typedef std::string String;
    typedef std::vector<String> StringVector;

    class Matrix4;
    class Quaternion;
    class Image;
    class Ray;
    class Font;
    class ResourceManager;
    class ManualResourceLoader;
    typedef unsigned long ResourceHandle;
}

#endif
