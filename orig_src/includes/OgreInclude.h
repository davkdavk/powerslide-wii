#ifndef OGREINCLUDE_H
#define OGREINCLUDE_H

#if defined(WII) || defined(__wii__)
    #include "wii_stubs/OGRE/Ogre.h"
#else
    #include "Ogre.h"

    #if defined(__ANDROID__)
        #include "OgreGLES2Plugin.h"
    #else
        #include "OgreGLPlugin.h"
    #endif
    #include "Plugins/ParticleFX/OgreParticleFXPlugin.h"
#endif

#endif
