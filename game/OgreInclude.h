#ifndef OGREINCLUDE_H
#define OGREINCLUDE_H

#if defined(WII) || defined(__wii__) || defined(GEKKO)
    #include "wii_stubs/OGRE/Ogre.h"
    #include "wii_stubs/OGRE/OgreSceneManager.h"
#else
    #include "Ogre.h"
    #include "OgreSceneManager.h"

    #if defined(__ANDROID__)
        #include "OgreGLES2Plugin.h"
    #else
        #include "OgreGLPlugin.h"
    #endif
    #include "Plugins/ParticleFX/OgreParticleFXPlugin.h"
#endif

#endif
