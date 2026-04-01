#ifndef SFML_SYSTEM_HPP
#define SFML_SYSTEM_HPP

#include <string>
#include <ctime>

namespace sf
{
    class String
    {
    public:
        String() {}
        String(const char* str) {}
    };
    
    class Clock
    {
    public:
        Clock() : mTime(0) {}
        
        sf::Time getElapsedTime() const;
        sf::Time restart() { return sf::Time(); }
        
    private:
        clock_t mTime;
    };
    
    class Time
    {
    public:
        Time() : microseconds(0) {}
        
        int64_t asMicroseconds() const { return microseconds; }
        float asSeconds() const { return microseconds / 1000000.0f; }
        
    private:
        int64_t microseconds;
    };
    
    inline Clock::getElapsedTime() const
    {
        return sf::Time();
    }
    
    class Thread
    {
    public:
        template<typename T>
        Thread(T* function, void* userData = nullptr) {}
        ~Thread() {}
        
        void launch() {}
        void wait() {}
        void terminate() {}
    };
    
    class Mutex
    {
    public:
        Mutex() {}
        ~Mutex() {}
        
        void lock() {}
        void unlock() {}
        bool tryLock() { return true; }
    };
    
    class Lock
    {
    public:
        explicit Lock(Mutex& mutex) : mMutex(mutex) { mMutex.lock(); }
        ~Lock() { mMutex.unlock(); }
        
    private:
        Mutex& mMutex;
    };
    
    class NonCopyable
    {
    protected:
        NonCopyable() {}
        ~NonCopyable() {}
        
    private:
        NonCopyable(const NonCopyable&);
        NonCopyable& operator=(const NonCopyable&);
    };
    
    class InputStream
    {
    public:
        virtual ~InputStream() {}
        virtual bool open(const std::string& filename) = 0;
        virtual int64_t read(void* data, int64_t size) = 0;
        virtual int64_t seek(int64_t position) = 0;
        virtual int64_t tell() = 0;
        virtual int64_t getSize() = 0;
    };
    
    typedef int64_t int64;
    typedef int64_t Uint64;
    typedef int32_t Int32;
    typedef uint32_t Uint32;
    typedef int16_t Int16;
    typedef uint16_t Uint16;
    typedef int8_t Int8;
    typedef uint8_t Uint8;
}

#endif
