#ifndef WII_PHYSICS_H
#define WII_PHYSICS_H

#include <math.h>
#include <vector>
#include <map>

namespace Physics
{
    struct Vector3 {
        float x, y, z;
        Vector3() : x(0), y(0), z(0) {}
        Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
        Vector3 operator+(const Vector3& v) const { return Vector3(x+v.x, y+v.y, z+v.z); }
        Vector3 operator-(const Vector3& v) const { return Vector3(x-v.x, y-v.y, z-v.z); }
        Vector3 operator*(float s) const { return Vector3(x*s, y*s, z*s); }
        Vector3 operator/(float s) const { return Vector3(x/s, y/s, z/s); }
        float length() const { return sqrtf(x*x + y*y + z*z); }
        float dot(const Vector3& v) const { return x*v.x + y*v.y + z*v.z; }
        Vector3 normalize() const { float l = length(); return l > 0 ? *this/l : *this; }
        Vector3 cross(const Vector3& v) const { return Vector3(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x); }
    };
    
    struct Quaternion {
        float x, y, z, w;
        Quaternion() : x(0), y(0), z(0), w(1) {}
        Quaternion(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
        void fromAxisAngle(const Vector3& axis, float angle) {
            float half = angle * 0.5f;
            float s = sinf(half);
            w = cosf(half);
            x = axis.x * s; y = axis.y * s; z = axis.z * s;
        }
    };
    
    struct Transform {
        Vector3 position;
        Quaternion rotation;
        Vector3 scale;
        Transform() : scale(1,1,1) {}
    };
    
    struct AABB {
        Vector3 min;
        Vector3 max;
        AABB() : min(0,0,0), max(0,0,0) {}
        AABB(const Vector3& _min, const Vector3& _max) : min(_min), max(_max) {}
    };
    
    class RigidBody {
    public:
        RigidBody() : mass(1.0f), friction(0.5f), restitution(0.5f) {}
        
        void setMass(float m) { mass = m; }
        float getMass() const { return mass; }
        
        void setPosition(const Vector3& pos) { transform.position = pos; }
        Vector3 getPosition() const { return transform.position; }
        
        void setRotation(const Quaternion& q) { transform.rotation = q; }
        Quaternion getRotation() const { return transform.rotation; }
        
        void setLinearVelocity(const Vector3& v) { linearVelocity = v; }
        Vector3 getLinearVelocity() const { return linearVelocity; }
        
        void setAngularVelocity(const Vector3& v) { angularVelocity = v; }
        Vector3 getAngularVelocity() const { return angularVelocity; }
        
        void applyForce(const Vector3& force) { forces = forces + force; }
        void applyImpulse(const Vector3& impulse) { linearVelocity = linearVelocity + impulse / mass; }
        
        void integrate(float dt);
        
        Transform transform;
        Vector3 linearVelocity;
        Vector3 angularVelocity;
        Vector3 forces;
        float mass;
        float friction;
        float restitution;
        AABB boundingBox;
    };
    
    class WheelInfo {
    public:
        Vector3 connectionPoint;
        Vector3 direction;
        Vector3 axle;
        float suspensionRestLength;
        float suspensionCompression;
        float suspensionStiffness;
        float wheelRadius;
        float frictionSlip;
        bool isFrontWheel;
        float steering;
        float suspensionForce;
        float slip;
    };
    
    class VehicleController {
    public:
        VehicleController();
        ~VehicleController();
        
        void setPosition(const Vector3& pos);
        void setRotation(const Quaternion& rot);
        
        void applyEngineForce(float force, int wheel);
        void applySteering(float steering, int wheel);
        void applyBraking(float brake, int wheel);
        
        void update(float dt);
        
        RigidBody chassis;
        std::vector<WheelInfo> wheels;
        
        float engineForce;
        float brakeForce;
        float steeringAngle;
        float maxSteerAngle;
        float maxEngineForce;
        float maxBrakeForce;
    };
    
    class World {
    public:
        World();
        ~World();
        
        void addRigidBody(RigidBody* body);
        void removeRigidBody(RigidBody* body);
        
        void addVehicle(VehicleController* vehicle);
        void removeVehicle(VehicleController* vehicle);
        
        void setGravity(const Vector3& g);
        Vector3 getGravity() const { return gravity; }
        
        void stepSimulation(float dt);
        
    protected:
        std::vector<RigidBody*> bodies;
        std::vector<VehicleController*> vehicles;
        Vector3 gravity;
    };
    
    class RaycastHit {
    public:
        RigidBody* body;
        Vector3 hitPoint;
        Vector3 hitNormal;
        float hitFraction;
    };
    
    class DynamicsWorld : public World {
    public:
        DynamicsWorld();
        ~DynamicsWorld();
        
        bool raycastClosest(const Vector3& from, const Vector3& to, RaycastHit& hit);
    };
    
    void RigidBody::integrate(float dt) {
        if (mass <= 0) return;
        
        Vector3 acceleration = forces / mass;
        linearVelocity = linearVelocity + acceleration * dt;
        transform.position = transform.position + linearVelocity * dt;
        
        angularVelocity = angularVelocity * (1.0f - friction * dt);
        
        forces = Vector3(0,0,0);
    }
    
    VehicleController::VehicleController() : 
        engineForce(0), brakeForce(0), steeringAngle(0),
        maxSteerAngle(0.5f), maxEngineForce(1000.0f), maxBrakeForce(100.0f)
    {
        wheels.resize(4);
        for (int i = 0; i < 4; i++) {
            wheels[i].suspensionRestLength = 0.3f;
            wheels[i].suspensionStiffness = 30.0f;
            wheels[i].wheelRadius = 0.3f;
            wheels[i].frictionSlip = 2.0f;
            wheels[i].isFrontWheel = (i < 2);
        }
    }
    
    VehicleController::~VehicleController() {}
    
    void VehicleController::setPosition(const Vector3& pos) {
        chassis.setPosition(pos);
    }
    
    void VehicleController::setRotation(const Quaternion& rot) {
        chassis.setRotation(rot);
    }
    
    void VehicleController::applyEngineForce(float force, int wheel) {
        if (wheel >= 0 && wheel < 4) {
            wheels[wheel].suspensionForce = force;
        }
    }
    
    void VehicleController::applySteering(float steering, int wheel) {
        if (wheel >= 0 && wheel < 4 && wheels[wheel].isFrontWheel) {
            wheels[wheel].steering = steering * maxSteerAngle;
        }
    }
    
    void VehicleController::applyBraking(float brake, int wheel) {
        brakeForce = brake * maxBrakeForce;
    }
    
    void VehicleController::update(float dt) {
        chassis.applyForce(Vector3(0, -9.8f * chassis.getMass(), 0));
        chassis.integrate(dt);
    }
    
    World::World() : gravity(0, -9.8f, 0) {}
    World::~World() {}
    
    void World::addRigidBody(RigidBody* body) {
        bodies.push_back(body);
    }
    
    void World::removeRigidBody(RigidBody* body) {
        for (auto it = bodies.begin(); it != bodies.end(); ++it) {
            if (*it == body) {
                bodies.erase(it);
                break;
            }
        }
    }
    
    void World::addVehicle(VehicleController* vehicle) {
        vehicles.push_back(vehicle);
    }
    
    void World::removeVehicle(VehicleController* vehicle) {
        for (auto it = vehicles.begin(); it != vehicles.end(); ++it) {
            if (*it == vehicle) {
                vehicles.erase(it);
                break;
            }
        }
    }
    
    void World::setGravity(const Vector3& g) {
        gravity = g;
    }
    
    void World::stepSimulation(float dt) {
        for (auto* body : bodies) {
            if (body) body->integrate(dt);
        }
        for (auto* vehicle : vehicles) {
            if (vehicle) vehicle->update(dt);
        }
    }
    
    DynamicsWorld::DynamicsWorld() : World() {}
    DynamicsWorld::~DynamicsWorld() {}
    
    bool DynamicsWorld::raycastClosest(const Vector3& from, const Vector3& to, RaycastHit& hit) {
        hit.hitFraction = 1.0f;
        hit.body = NULL;
        
        Vector3 dir = to - from;
        float maxDist = dir.length();
        if (maxDist < 0.0001f) return false;
        dir = dir.normalize();
        
        for (auto* body : bodies) {
            if (!body) continue;
            
            Vector3 diff = body->getPosition() - from;
            float t = diff.dot(dir);
            
            if (t > 0 && t < maxDist) {
                Vector3 closest = from + dir * t;
                Vector3 toCenter = closest - body->getPosition();
                float dist = toCenter.length();
                
                float boundingRadius = 1.0f;
                if (dist < boundingRadius && t < hit.hitFraction) {
                    hit.hitFraction = t;
                    hit.hitPoint = closest;
                    hit.hitNormal = toCenter.normalize();
                    hit.body = body;
                }
            }
        }
        
        return hit.body != NULL;
    }
}

#endif
