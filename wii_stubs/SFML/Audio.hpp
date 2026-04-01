#ifndef SFML_AUDIO_HPP
#define SFML_AUDIO_HPP

#include <string>
#include <vector>
#include <stdint.h>

namespace sf
{
    typedef int16_t int16;
    
    class timeSpan
    {
    public:
        timeSpan() : microseconds(0) {}
        timeSpan(int64_t us) : microseconds(us) {}
        int64_t microseconds;
    };
    
    typedef int64_t int64;
    
    class SoundBuffer
    {
    public:
        SoundBuffer() : mSampleCount(0), mSampleRate(48000), mChannelCount(1) {}
        ~SoundBuffer() {}
        
        bool loadFromFile(const std::string& filename) { return false; }
        bool loadFromMemory(const void* data, std::size_t size) { return false; }
        bool loadFromSamples(const int16_t* samples, std::size_t sampleCount, unsigned int channelCount, unsigned int sampleRate) { return false; }
        
        std::size_t getSampleCount() const { return mSampleCount; }
        unsigned int getSampleRate() const { return mSampleRate; }
        unsigned int getChannelCount() const { return mChannelCount; }
        
    private:
        std::size_t mSampleCount;
        unsigned int mSampleRate;
        unsigned int mChannelCount;
    };
    
    class Sound
    {
    public:
        Sound() : mBuffer(nullptr), mVoice(-1), mStatus(Stopped) {}
        Sound(const SoundBuffer& buffer) : mBuffer(&buffer), mVoice(-1), mStatus(Stopped) {}
        ~Sound() {}
        
        void setBuffer(const SoundBuffer& buffer) { mBuffer = &buffer; }
        
        void play() {}
        void pause() {}
        void stop() {}
        
        enum Status { Stopped, Playing, Paused };
        Status getStatus() const { return mStatus; }
        
        void setPitch(float pitch) {}
        void setVolume(float volume) { mVolume = volume; }
        float getVolume() const { return mVolume; }
        void setPosition(float x, float y, float z) {}
        void setRelativeToListener(bool relative) {}
        void setLoop(bool loop) {}
        void setMinDistance(float distance) {}
        void setAttenuation(float attenuation) {}
        
    private:
        const SoundBuffer* mBuffer;
        int mVoice;
        Status mStatus;
        float mVolume = 100.0f;
    };
    
    class Music
    {
    public:
        Music() : mVoice(-1), mVolume(1.0f), mLoop(false), mStatus(Stopped), mSampleRate(48000), mChannelCount(1), mSampleCount(0) {}
        ~Music() {}
        
        bool openFromFile(const std::string& filename) { return false; }
        bool openFromMemory(const void* data, std::size_t size) { return false; }
        
        void play() {}
        void pause() {}
        void stop() {}
        
        enum Status { Stopped, Playing, Paused };
        Status getStatus() const { return mStatus; }
        
        void setVolume(float volume) { mVolume = volume; }
        void setLoop(bool loop) { mLoop = loop; }
        
        void setPlayingOffset(timeSpan timeOffset) {}
        timeSpan getPlayingOffset() const { return timeSpan(); }
        
        std::size_t getChannelCount() const { return mChannelCount; }
        unsigned int getSampleRate() const { return mSampleRate; }
        timeSpan getDuration() const { return timeSpan(); }
        
    private:
        unsigned int mSampleRate;
        unsigned int mChannelCount;
        std::size_t mSampleCount;
        timeSpan mDuration;
        int mVoice;
        float mVolume;
        bool mLoop;
        Status mStatus;
    };
    
    class SoundSource
    {
    public:
        enum Status { Stopped, Playing, Paused };
        
        void setPitch(float pitch) {}
        void setVolume(float volume) {}
        void setPosition(float x, float y, float z) {}
        void setRelativeToListener(bool relative) {}
        void setMinDistance(float distance) {}
        void setAttenuation(float attenuation) {}
        
        Status getStatus() const { return Stopped; }
        
    protected:
        unsigned int m_source;
    };
    
    class Listener
    {
    public:
        static void setPosition(float x, float y, float z) {}
        static void setDirection(float x, float y, float z) {}
        static void setUpVector(float x, float y, float z) {}
        static void setGlobalVolume(float volume) { sGlobalVolume = volume; }
        static float getGlobalVolume() { return sGlobalVolume; }
    private:
        inline static float sGlobalVolume = 100.0f;
    };
    
    inline void InitAudio() {}
    inline void UpdateAudio() {}
}

#endif
