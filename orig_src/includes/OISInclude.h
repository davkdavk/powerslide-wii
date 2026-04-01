
#ifndef OISINCLUDE_H
#define OISINCLUDE_H

#if defined(WII) || defined(__wii__)
    #include "wii_stubs/OIS/OISEvents.h"
    #include "wii_stubs/OIS/OISInputManager.h"
    #include "wii_stubs/OIS/OISKeyboard.h"
    #include "wii_stubs/OIS/OISMouse.h"
#else
    #include "OISEvents.h"
    #include "OISInputManager.h"
    #include "OISKeyboard.h"
    #include "OISMouse.h"

    #if defined(__ANDROID__)
        #include "OISMultiTouch.h"
    #endif
#endif

#endif