/*
 * Wii Entry Point - Powerslide Remake
 * Wii GX Renderer - Gouraud shading, 60fps
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fat.h>
#include <sys/stat.h>
#include <gccore.h>
#include <ogc/system.h>
#include <sdcard/wiisd_io.h>
#include <sdcard/gcsd.h>
#include <stdarg.h>

#include "OGRE/WiiGXRenderer.h"
#include "PFLoader.h"

#define PF_DIR "sd:/apps/powerslide"
#define WII_BUILD_TAG "fix164-appspath"
#define MAX_FILENAME 256

using namespace WiiGX;
using namespace PF;

static void WiiDebugLog(const char* fmt, ...)
{
    if(!fmt)
        return;

    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    buffer[sizeof(buffer) - 1] = '\0';

    SYS_Report("%s", buffer);

    FILE* logFile = fopen("sd:/powerslide/debug.log", "a");
    if(!logFile)
        logFile = fopen("sd:/debug.log", "a");
    if(!logFile)
        logFile = fopen("debug.log", "a");
    if(logFile)
    {
        fputs(buffer, logFile);
        fclose(logFile);
    }
}

static bool fileExists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

static void* loadFile(const char *filename, unsigned int *outSize) {
    char filepath[MAX_FILENAME];
    snprintf(filepath, MAX_FILENAME, "%s/%s", PF_DIR, filename);
    
    if (!fileExists(filepath)) {
        printf("ERROR: File not found: %s\n", filepath);
        return NULL;
    }
    
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        printf("ERROR: Cannot open file: %s\n", filepath);
        return NULL;
    }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (size <= 0) {
        printf("ERROR: Invalid file size: %s\n", filepath);
        fclose(fp);
        return NULL;
    }
    
    void *buffer = malloc(size);
    if (!buffer) {
        printf("ERROR: Cannot allocate memory for: %s\n", filepath);
        fclose(fp);
        return NULL;
    }
    
    size_t read = fread(buffer, 1, size, fp);
    fclose(fp);
    
    if (read != (size_t)size) {
        printf("ERROR: Incomplete read from: %s\n", filepath);
        free(buffer);
        return NULL;
    }
    
    *outSize = (unsigned int)size;
    printf("Loaded %s (%u bytes)\n", filepath, *outSize);
    return buffer;
}

static void freeFile(void *data) {
    if (data) {
        free(data);
    }
}

static void drawTestCube() {
    Renderer& gx = Renderer::getInstance();
    
    Material mat;
    mat.diffuse = Color(200, 100, 100);
    mat.ambient = Color(50, 25, 25);
    gx.setMaterial(mat);
    
    Vertex cube[] = {
        {Vec3(-1, -1,  1), Vec3(0, 0, 1), Color(255, 0, 0), 0, 0},
        {Vec3( 1, -1,  1), Vec3(0, 0, 1), Color(255, 0, 0), 1, 0},
        {Vec3( 1,  1,  1), Vec3(0, 0, 1), Color(255, 0, 0), 1, 1},
        {Vec3(-1,  1,  1), Vec3(0, 0, 1), Color(255, 0, 0), 0, 1},
        
        {Vec3(-1, -1, -1), Vec3(0, 0, -1), Color(0, 255, 0), 0, 0},
        {Vec3(-1,  1, -1), Vec3(0, 0, -1), Color(0, 255, 0), 1, 0},
        {Vec3( 1,  1, -1), Vec3(0, 0, -1), Color(0, 255, 0), 1, 1},
        {Vec3( 1, -1, -1), Vec3(0, 0, -1), Color(0, 255, 0), 0, 1},
        
        {Vec3(-1,  1, -1), Vec3(0, 1, 0), Color(0, 0, 255), 0, 0},
        {Vec3(-1,  1,  1), Vec3(0, 1, 0), Color(0, 0, 255), 1, 0},
        {Vec3( 1,  1,  1), Vec3(0, 1, 0), Color(0, 0, 255), 1, 1},
        {Vec3( 1,  1, -1), Vec3(0, 1, 0), Color(0, 0, 255), 0, 1},
        
        {Vec3(-1, -1, -1), Vec3(0, -1, 0), Color(255, 255, 0), 0, 0},
        {Vec3( 1, -1, -1), Vec3(0, -1, 0), Color(255, 255, 0), 1, 0},
        {Vec3( 1, -1,  1), Vec3(0, -1, 0), Color(255, 255, 0), 1, 1},
        {Vec3(-1, -1,  1), Vec3(0, -1, 0), Color(255, 255, 0), 0, 1},
        
        {Vec3( 1, -1, -1), Vec3(1, 0, 0), Color(255, 0, 255), 0, 0},
        {Vec3( 1,  1, -1), Vec3(1, 0, 0), Color(255, 0, 255), 1, 0},
        {Vec3( 1,  1,  1), Vec3(1, 0, 0), Color(255, 0, 255), 1, 1},
        {Vec3( 1, -1,  1), Vec3(1, 0, 0), Color(255, 0, 255), 0, 1},
        
        {Vec3(-1, -1, -1), Vec3(-1, 0, 0), Color(0, 255, 255), 0, 0},
        {Vec3(-1, -1,  1), Vec3(-1, 0, 0), Color(0, 255, 255), 1, 0},
        {Vec3(-1,  1,  1), Vec3(-1, 0, 0), Color(0, 255, 255), 1, 1},
        {Vec3(-1,  1, -1), Vec3(-1, 0, 0), Color(0, 255, 255), 0, 1},
    };
    
    u16 indices[] = {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };
    
    Matrix4x4 world = Matrix4x4::identity();
    gx.setWorld(world);
    
    gx.drawIndexedPrimitives(GX_TRIANGLES, cube, 24, indices, 36);
}

// Tell devkitPro CRT that no argv was provided
// so build_argv is skipped entirely
extern "C" {
    __attribute__((weak)) struct __argv __wii_no_argv = {0, NULL, 0, 0, NULL, NULL};
    __attribute__((weak)) struct __argv* __system_argv = &__wii_no_argv;
}

int main() {
    // Zero argv magic so __CheckARGV skips build_argv
    if (__system_argv) {
        __system_argv->argvMagic = 0;
    }

    printf("\n");
    printf("=========================================\n");
    printf("       POWERSLIDE REMAKE - WII GX\n");
    printf("=========================================\n");
    printf("\n");
    
    printf("Initializing libfat...\n");
    remove("sd:/powerslide/debug.log");
    remove("sd:/debug.log");
    WiiDebugLog("[DBG] debug log start\n");
    WiiDebugLog("[DBG] build=%s pf_dir=%s\n", WII_BUILD_TAG, PF_DIR);
    const bool fatOk = fatInitDefault();
    WiiDebugLog("[FAT] fatInitDefault result=%d\n", fatOk ? 1 : 0);
    if (!fatOk) {
        printf("ERROR: fatInitDefault() failed!\n");
        return 1;
    }

    const bool sdMountOk = fatMountSimple("sd", &__io_wiisd);
    WiiDebugLog("[FAT] fatMountSimple(sd,__io_wiisd) result=%d\n", sdMountOk ? 1 : 0);

    const bool gcsdMountOk = fatMountSimple("sd", &__io_gcsda);
    WiiDebugLog("[FAT] fatMountSimple(sd,__io_gcsda) result=%d\n", gcsdMountOk ? 1 : 0);

    struct stat sdStat;
    WiiDebugLog("[FAT] stat sd:/data.pf=%d\n", stat("sd:/data.pf", &sdStat) == 0 ? 1 : 0);
    WiiDebugLog("[FAT] stat sd:/powerslide/data.pf=%d\n", stat("sd:/powerslide/data.pf", &sdStat) == 0 ? 1 : 0);
    WiiDebugLog("[FAT] stat sd:/powerslide/store.pf=%d\n", stat("sd:/powerslide/store.pf", &sdStat) == 0 ? 1 : 0);
    WiiDebugLog("[FAT] stat sd:/powerslide/gameshell.pf=%d\n", stat("sd:/powerslide/gameshell.pf", &sdStat) == 0 ? 1 : 0);
    WiiDebugLog("[FAT] stat sd:/apps/powerslide/data.pf=%d\n", stat("sd:/apps/powerslide/data.pf", &sdStat) == 0 ? 1 : 0);
    WiiDebugLog("[FAT] stat sd:/apps/powerslide/store.pf=%d\n", stat("sd:/apps/powerslide/store.pf", &sdStat) == 0 ? 1 : 0);
    WiiDebugLog("[FAT] stat sd:/apps/powerslide/gameshell.pf=%d\n", stat("sd:/apps/powerslide/gameshell.pf", &sdStat) == 0 ? 1 : 0);
    WiiDebugLog("[FAT] stat sd:/apps/Powerslide/data.pf=%d\n", stat("sd:/apps/Powerslide/data.pf", &sdStat) == 0 ? 1 : 0);
    WiiDebugLog("[FAT] stat sd:/apps/Powerslide/store.pf=%d\n", stat("sd:/apps/Powerslide/store.pf", &sdStat) == 0 ? 1 : 0);
    WiiDebugLog("[FAT] stat sd:/apps/Powerslide/gameshell.pf=%d\n", stat("sd:/apps/Powerslide/gameshell.pf", &sdStat) == 0 ? 1 : 0);

    FILE* sdProbe = fopen("sd:/data.pf", "rb");
    WiiDebugLog("[FAT] direct test sd:/data.pf = %d\n", sdProbe ? 1 : 0);
    if (sdProbe) {
        fclose(sdProbe);
    }

    printf("libfat: OK\n");
    
    mkdir(PF_DIR, 0777);
    
    printf("\nChecking required files in %s/:\n", PF_DIR);
    
    bool data_pf = fileExists(PF_DIR "/data.pf");
    bool gameshell_pf = fileExists(PF_DIR "/gameshell.pf");
    bool store_pf = fileExists(PF_DIR "/store.pf");
    
    printf("  data.pf:      %s\n", data_pf ? "FOUND" : "MISSING");
    printf("  gameshell.pf: %s\n", gameshell_pf ? "FOUND" : "MISSING");
    printf("  store.pf:    %s\n", store_pf ? "FOUND" : "MISSING");
    
    if (!data_pf || !gameshell_pf || !store_pf) {
        printf("\nERROR: Required files missing!\n");
    } else {
        printf("\nLoading game files...\n");
        unsigned int size = 0;
        
        void *data = loadFile("data.pf", &size);
        if (data) freeFile(data);
        
        void *shell = loadFile("gameshell.pf", &size);
        if (shell) freeFile(shell);
        
        void *store = loadFile("store.pf", &size);
        if (store) freeFile(store);
        
        printf("\nGame files loaded!\n");
    }
    
    printf("\nInitializing Wii GX Renderer...\n");
    printf("  - Gouraud shading (vertex-based)\n");
    printf("  - No soft shadows\n");
    printf("  - No per-pixel lighting\n");
    printf("  - Target: 60fps\n");
    
    Renderer& gx = Renderer::getInstance();
    gx.init();
    gx.setClearColor(32, 32, 64, 255);
    gx.enableDepthTest(true);
    gx.enableDepthWrite(true);
    gx.enableBlend(false);
    
    printf("Wii GX: OK\n");
    
    printf("\n=========================================\n");
    printf("Rendering test...\n");
    printf("=========================================\n");
    
    for (int i = 0; i < 60; i++) {
        gx.beginFrame();
        
        drawTestCube();
        
        gx.endFrame();
        
        if (i % 10 == 0) {
            printf("Frame %d - FPS: %.1f\n", i, gx.getFPS());
        }
    }
    
    printf("\n=========================================\n");
    printf("Render test complete!\n");
    printf("Final FPS: %.1f\n", gx.getFPS());
    printf("=========================================\n");

    extern int powerslide_main();
    return powerslide_main();
}
