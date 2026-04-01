/*
 * Powerslide Wii - Full Game Implementation
 * Racing game with track, cars, physics, and gameplay
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fat.h>
#include <sys/stat.h>
#include <gccore.h>
#include <ogc/lwp_watchdog.h>
#include <ogc/gx.h>
#include <stdint.h>
#include <math.h>

#include "OGRE/WiiGXRenderer.h"
#include "WiiInput.h"
#include "PFLoader.h"
#include "Physics.h"

#define PF_DIR "sd:/powerslide"
#define MAX_FILENAME 256
#define TRACK_SEGMENTS 50
#define MAX_CARS 12
#define LAPS_TO_WIN 3

using namespace WiiGX;
using namespace PF;
using namespace Physics;

static bool fileExists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

static void* loadFile(const char *filename, unsigned int *outSize) {
    char filepath[MAX_FILENAME];
    snprintf(filepath, MAX_FILENAME, "%s/%s", PF_DIR, filename);
    
    if (!fileExists(filepath)) return NULL;
    
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return NULL;
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (size <= 0) { fclose(fp); return NULL; }
    
    void *buffer = malloc(size);
    if (!buffer) { fclose(fp); return NULL; }
    
    size_t read = fread(buffer, 1, size, fp);
    fclose(fp);
    
    if (read != (size_t)size) { free(buffer); return NULL; }
    
    *outSize = (unsigned int)size;
    return buffer;
}

static void freeFile(void *data) {
    if (data) free(data);
}

struct TrackPoint {
    float x, z;
    float width;
    float curvature;
};

struct Car {
    float x, y, z;
    float speed;
    float angle;
    float acceleration;
    float steering;
    float velocityX;
    float velocityZ;
    int lap;
    int checkpoint;
    float lapTime;
    float bestLap;
    bool finished;
    int colorR, colorG, colorB;
};

class PowerslideGame {
public:
    PowerslideGame() : running(false), world(NULL) {
        playerCar = NULL;
        cameraAngle = 0;
        cameraHeight = 5.0f;
        cameraDistance = 12.0f;
        raceState = 0;
        countdownTimer = 3.0f;
        currentTrack = 0;
        gameMode = 0;
    }
    
    ~PowerslideGame() {
        if (world) delete world;
    }
    
    bool init() {
        fatInitDefault();
        
        Renderer& gx = Renderer::getInstance();
        gx.init();
        gx.setClearColor(32, 32, 64, 255);
        gx.enableDepthTest(true);
        gx.enableDepthWrite(true);
        gx.enableBlend(false);
        
        WiiInput::Init();
        
        generateTrack();
        
        initCars();
        
        running = true;
        return true;
    }
    
    void generateTrack() {
        trackPoints.resize(TRACK_SEGMENTS);
        for (int i = 0; i < TRACK_SEGMENTS; i++) {
            float angle = (float)i / TRACK_SEGMENTS * 6.28318f;
            float radius = 40.0f + sinf(angle * 2.0f) * 15.0f;
            trackPoints[i].x = cosf(angle) * radius;
            trackPoints[i].z = sinf(angle) * radius;
            trackPoints[i].width = 8.0f + sinf(angle * 3.0f) * 2.0f;
            trackPoints[i].curvature = cosf(angle * 2.0f) * 0.3f;
        }
    }
    
    void initCars() {
        playerCar = &cars[0];
        playerCar->x = trackPoints[0].x;
        playerCar->z = trackPoints[0].z;
        playerCar->y = 0.5f;
        playerCar->speed = 0;
        playerCar->angle = 0;
        playerCar->acceleration = 0;
        playerCar->steering = 0;
        playerCar->velocityX = 0;
        playerCar->velocityZ = 0;
        playerCar->lap = 1;
        playerCar->checkpoint = 0;
        playerCar->lapTime = 0;
        playerCar->bestLap = 999.0f;
        playerCar->finished = false;
        playerCar->colorR = 255;
        playerCar->colorG = 0;
        playerCar->colorB = 0;
        
        for (int i = 1; i < MAX_CARS; i++) {
            cars[i].x = trackPoints[0].x + (i % 4) * 3.0f - 6.0f;
            cars[i].z = trackPoints[0].z + (i / 4) * 3.0f - 3.0f;
            cars[i].y = 0.5f;
            cars[i].speed = 0;
            cars[i].angle = 0;
            cars[i].lap = 1;
            cars[i].checkpoint = 0;
            cars[i].lapTime = 0;
            cars[i].bestLap = 999.0f;
            cars[i].finished = false;
            cars[i].colorR = (i * 37) % 256;
            cars[i].colorG = (i * 73) % 256;
            cars[i].colorB = (i * 131) % 256;
        }
    }
    
    void handleInput() {
        WiiInput::Update();
        WiiInput::InputManager& input = WiiInput::InputManager::getInstance();
        
        float throttle = 0;
        float steer = 0;
        
        if (input.isButtonPressed(0, WiiInput::InputManager::BTN_UP) || input.getStickY(0) < -0.3f) {
            throttle = 1.0f;
        }
        if (input.isButtonPressed(0, WiiInput::InputManager::BTN_DOWN) || input.getStickY(0) > 0.3f) {
            throttle = -0.5f;
        }
        if (input.isButtonPressed(0, WiiInput::InputManager::BTN_LEFT) || input.getStickX(0) < -0.3f) {
            steer = -1.0f;
        }
        if (input.isButtonPressed(0, WiiInput::InputManager::BTN_RIGHT) || input.getStickX(0) > 0.3f) {
            steer = 1.0f;
        }
        
        if (input.isButtonJustPressed(0, WiiInput::InputManager::BTN_A)) {
            gameMode = (gameMode + 1) % 2;
        }
        
        if (input.isButtonJustPressed(0, WiiInput::InputManager::BTN_HOME)) {
            running = false;
        }
        
        playerCar->acceleration = throttle * 80.0f;
        playerCar->steering = steer;
        
        if (input.isButtonPressed(0, WiiInput::InputManager::BTN_B)) {
            playerCar->acceleration = -30.0f;
        }
    }
    
    void updateAI(Car* car, float dt) {
        int targetCheckpoint = (car->checkpoint + 1) % TRACK_SEGMENTS;
        TrackPoint& target = trackPoints[targetCheckpoint];
        
        float dx = target.x - car->x;
        float dz = target.z - car->z;
        float targetAngle = atan2f(dz, dx);
        
        float angleDiff = targetAngle - car->angle;
        while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
        while (angleDiff < -3.14159f) angleDiff += 6.28318f;
        
        car->steering = angleDiff * 2.0f;
        if (car->steering > 1.0f) car->steering = 1.0f;
        if (car->steering < -1.0f) car->steering = -1.0f;
        
        float speedTarget = 40.0f + (car->lap - 1) * 5.0f;
        if (fabsf(angleDiff) > 0.5f) speedTarget *= 0.5f;
        
        if (car->speed < speedTarget) {
            car->acceleration = 60.0f;
        } else {
            car->acceleration = 0;
        }
    }
    
    void updateCar(Car* car, float dt) {
        if (car->finished) {
            car->speed *= 0.98f;
            car->x += sinf(car->angle) * car->speed * dt;
            car->z += cosf(car->angle) * car->speed * dt;
            return;
        }
        
        car->speed += car->acceleration * dt;
        
        if (car->speed > 80.0f) car->speed = 80.0f;
        if (car->speed < -20.0f) car->speed = -20.0f;
        
        if (car->speed > 0.1f) {
            car->angle += car->steering * 2.0f * dt * (car->speed / 30.0f);
        } else if (car->speed < -0.1f) {
            car->angle -= car->steering * 2.0f * dt * (car->speed / 30.0f);
        }
        
        car->x += sinf(car->angle) * car->speed * dt;
        car->z += cosf(car->angle) * car->speed * dt;
        
        int nearCheckpoint = findNearestCheckpoint(car->x, car->z);
        if (nearCheckpoint != car->checkpoint) {
            if ((car->checkpoint + 1) % TRACK_SEGMENTS == nearCheckpoint) {
                car->checkpoint = nearCheckpoint;
                if (nearCheckpoint == 0 && car->lap > 0) {
                    if (car->lapTime < car->bestLap && car->lap > 0) {
                        car->bestLap = car->lapTime;
                    }
                    car->lapTime = 0;
                    car->lap++;
                    if (car->lap > LAPS_TO_WIN) {
                        car->finished = true;
                    }
                }
            }
        }
        
        if (!car->finished) {
            car->lapTime += dt;
        }
    }
    
    int findNearestCheckpoint(float x, float z) {
        int nearest = 0;
        float minDist = 99999.0f;
        for (int i = 0; i < TRACK_SEGMENTS; i++) {
            float dx = trackPoints[i].x - x;
            float dz = trackPoints[i].z - z;
            float dist = dx*dx + dz*dz;
            if (dist < minDist) {
                minDist = dist;
                nearest = i;
            }
        }
        return nearest;
    }
    
    void update(float dt) {
        handleInput();
        
        if (raceState == 0) {
            countdownTimer -= dt;
            if (countdownTimer <= 0) {
                raceState = 1;
                countdownTimer = 0;
            }
            return;
        }
        
        if (raceState == 1) {
            updateCar(playerCar, dt);
            
            for (int i = 1; i < MAX_CARS; i++) {
                updateAI(&cars[i], dt);
                updateCar(&cars[i], dt);
            }
        }
    }
    
    void renderTrack() {
        Renderer& gx = Renderer::getInstance();
        
        Material trackMat;
        trackMat.diffuse = Color(80, 80, 80);
        trackMat.ambient = Color(30, 30, 30);
        gx.setMaterial(trackMat);
        
        for (int i = 0; i < TRACK_SEGMENTS; i++) {
            int next = (i + 1) % TRACK_SEGMENTS;
            
            TrackPoint& p1 = trackPoints[i];
            TrackPoint& p2 = trackPoints[next];
            
            float dx = p2.x - p1.x;
            float dz = p2.z - p1.z;
            float len = sqrtf(dx*dx + dz*dz);
            float nx = -dz / len * p1.width;
            float nz = dx / len * p1.width;
            
            Vertex quad[] = {
                {Vec3(p1.x - nx, 0, p1.z - nz), Vec3(0, 1, 0), Color(100, 100, 100), 0, 0},
                {Vec3(p1.x + nx, 0, p1.z + nz), Vec3(0, 1, 0), Color(100, 100, 100), 1, 0},
                {Vec3(p2.x + nx, 0, p2.z + nz), Vec3(0, 1, 0), Color(100, 100, 100), 1, 1},
                {Vec3(p2.x - nx, 0, p2.z - nz), Vec3(0, 1, 0), Color(100, 100, 100), 0, 1},
            };
            
            u16 indices[] = {0, 1, 2, 0, 2, 3};
            
            Matrix4x4 world = Matrix4x4::identity();
            gx.setWorld(world);
            gx.drawIndexedPrimitives(GX_TRIANGLES, quad, 4, indices, 6);
            
            Material lineMat;
            lineMat.diffuse = Color(255, 255, 0);
            lineMat.ambient = Color(100, 100, 0);
            gx.setMaterial(lineMat);
            
            Vertex line[] = {
                {Vec3(p1.x - nx*0.9f, 0.05f, p1.z - nz*0.9f), Vec3(0, 1, 0), Color(255, 255, 0), 0, 0},
                {Vec3(p1.x + nx*0.9f, 0.05f, p1.z + nz*0.9f), Vec3(0, 1, 0), Color(255, 255, 0), 1, 0},
            };
            u16 lineIdx[] = {0, 1};
            
            gx.setWorld(world);
            gx.drawIndexedPrimitives(GX_LINES, line, 2, lineIdx, 2);
        }
        
        Material grassMat;
        grassMat.diffuse = Color(0, 100, 0);
        grassMat.ambient = Color(0, 50, 0);
        gx.setMaterial(grassMat);
        
        float grassSize = 200.0f;
        Vertex grass[] = {
            {Vec3(-grassSize, -0.1f, -grassSize), Vec3(0, 1, 0), Color(0, 120, 0), 0, 0},
            {Vec3(grassSize, -0.1f, -grassSize), Vec3(0, 1, 0), Color(0, 120, 0), 1, 0},
            {Vec3(grassSize, -0.1f, grassSize), Vec3(0, 1, 0), Color(0, 120, 0), 1, 1},
            {Vec3(-grassSize, -0.1f, grassSize), Vec3(0, 1, 0), Color(0, 120, 0), 0, 1},
        };
        u16 gIdx[] = {0, 1, 2, 0, 2, 3};
        
        Matrix4x4 world = Matrix4x4::identity();
        gx.setWorld(world);
        gx.drawIndexedPrimitives(GX_TRIANGLES, grass, 4, gIdx, 6);
    }
    
    void renderCar(Car* car) {
        Renderer& gx = Renderer::getInstance();
        
        Material carMat;
        carMat.diffuse = Color(car->colorR, car->colorG, car->colorB);
        carMat.ambient = Color(car->colorR/3, car->colorG/3, car->colorB/3);
        gx.setMaterial(carMat);
        
        float carLength = 2.0f;
        float carWidth = 1.0f;
        float carHeight = 0.5f;
        
        float c = cosf(car->angle);
        float s = sinf(car->angle);
        
        Vertex carVerts[] = {
            {Vec3(-carLength, car->y, -carWidth), Vec3(0, 1, 0), Color(car->colorR, car->colorG, car->colorB), 0, 0},
            {Vec3(carLength, car->y, -carWidth), Vec3(0, 1, 0), Color(car->colorR, car->colorG, car->colorB), 1, 0},
            {Vec3(carLength, car->y + carHeight, -carWidth), Vec3(0, 1, 0), Color(car->colorR, car->colorG, car->colorB), 1, 1},
            {Vec3(-carLength, car->y + carHeight, -carWidth), Vec3(0, 1, 0), Color(car->colorR, car->colorG, car->colorB), 0, 1},
            
            {Vec3(-carLength, car->y, carWidth), Vec3(0, 1, 0), Color(car->colorR/2, car->colorG/2, car->colorB/2), 0, 0},
            {Vec3(-carLength, car->y + carHeight, carWidth), Vec3(0, 1, 0), Color(car->colorR/2, car->colorG/2, car->colorB/2), 1, 0},
            {Vec3(carLength, car->y + carHeight, carWidth), Vec3(0, 1, 0), Color(car->colorR/2, car->colorG/2, car->colorB/2), 1, 1},
            {Vec3(carLength, car->y, carWidth), Vec3(0, 1, 0), Color(car->colorR/2, car->colorG/2, car->colorB/2), 0, 1},
        };
        
        u16 carIdx[] = {
            0, 1, 2, 0, 2, 3,
            4, 5, 6, 4, 6, 7,
            0, 3, 5, 0, 5, 4,
            1, 7, 6, 1, 6, 2,
            3, 2, 6, 3, 6, 5,
            0, 4, 7, 0, 7, 1
        };
        
        Matrix4x4 world = Matrix4x4::identity();
        world.m[0] = c; world.m[1] = s;
        world.m[4] = -s; world.m[5] = c;
        world.m[12] = car->x;
        world.m[14] = car->z;
        
        gx.setWorld(world);
        gx.drawIndexedPrimitives(GX_TRIANGLES, carVerts, 8, carIdx, 36);
    }
    
    void render() {
        Renderer& gx = Renderer::getInstance();
        gx.beginFrame();
        
        gx.setWorld(Matrix4x4::identity());
        
        renderTrack();
        
        for (int i = 0; i < MAX_CARS; i++) {
            renderCar(&cars[i]);
        }
        
        gx.endFrame();
    }
    
    void run() {
        if (!init()) return;
        
        u64 lastTime = gettime();
        
        while (running) {
            u64 currentTime = gettime();
            float dt = (float)ticks_to_millisecs(currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;
            
            if (dt > 0.1f) dt = 0.1f;
            
            update(dt);
            render();
        }
    }
    
private:
    bool running;
    World* world;
    VehicleController* vehicle;
    Car cars[MAX_CARS];
    Car* playerCar;
    std::vector<TrackPoint> trackPoints;
    float cameraAngle;
    float cameraHeight;
    float cameraDistance;
    int raceState;
    float countdownTimer;
    int currentTrack;
    int gameMode;
};

#if !defined(WII_FULL_BUILD)
int gamemain_unused() {
    PowerslideGame game;
    game.run();
    return 0;
}
#endif
