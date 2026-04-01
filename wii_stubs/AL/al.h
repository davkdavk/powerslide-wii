#ifndef AL_AL_H
#define AL_AL_H

#include <stdlib.h>

#define AL_API
#define AL_APIENTRY

typedef char ALchar;
typedef unsigned char ALboolean;
typedef int ALint;
typedef unsigned int ALuint;
typedef int ALsizei;
typedef unsigned int ALenum;
typedef float ALfloat;
typedef double ALdouble;
typedef void ALvoid;

#define AL_FALSE 0
#define AL_TRUE 1

#define AL_NO_ERROR 0
#define AL_INVALID_NAME 0xA001
#define AL_INVALID_ENUM 0xA002
#define AL_INVALID_VALUE 0xA003
#define AL_INVALID_OPERATION 0xA004
#define AL_OUT_OF_MEMORY 0xA005

#define AL_VERSION_1_0 1
#define AL_VERSION_1_1 1

#define AL_CINEMATIC 0x1F87
#define AL_MUSIC 0x1F82
#define AL_AMBIENT 0x1F00

#define AL_SOURCE_STATE 0x1010
#define AL_INITIAL 0x1011
#define AL_PLAYING 0x1012
#define AL_PAUSED 0x1013
#define AL_STOPPED 0x1014

#define AL_BUFFERS_QUEUED 0x1015
#define AL_BUFFERS_PROCESSED 0x1016

#define AL_SOURCE_TYPE 0x1027
#define AL_STATIC 0x1028
#define AL_STREAMING 0x1029
#define AL_UNDETERMINED 0x1030

#define AL_FORMAT_MONO8 0x1100
#define AL_FORMAT_MONO16 0x1101
#define AL_FORMAT_STEREO8 0x1102
#define AL_FORMAT_STEREO16 0x1103

#define AL_REFERENCE_DISTANCE 0x1020
#define AL_ROLLOFF_FACTOR 0x1021
#define AL_CONE_INNER_ANGLE 0x1001
#define AL_CONE_OUTER_ANGLE 0x1002
#define AL_CONE_OUTER_GAIN 0x1003

#define AL_PITCH 0x1003
#define AL_POSITION 0x1004
#define AL_DIRECTION 0x1005
#define AL_VELOCITY 0x1006
#define AL_GAIN 0x100A
#define AL_LOOPING 0x1007

#define AL_BUFFER 0x1009
#define AL_SOURCE_RELATIVE 0x202

#define AL_SEC_OFFSET 0x1024
#define AL_SAMPLE_OFFSET 0x1025
#define AL_BYTE_OFFSET 0x1026

#define AL_FREQUENCY 0x2001
#define AL_BITS 0x2002
#define AL_CHANNELS 0x2003
#define AL_SIZE 0x2004

#define AL_PENDING 0x2011
#define AL_PROCESSED 0x2012

#define AL_NO_ERROR (0)
#define AL_INVALID_NAME (0xA001)
#define AL_INVALID_ENUM (0xA002)
#define AL_INVALID_VALUE (0xA003)
#define AL_INVALID_OPERATION (0xA004)
#define AL_OUT_OF_MEMORY (0xA005)

typedef struct ALCdevice ALCdevice;
typedef struct ALCcontext ALCcontext;

#define ALC_FALSE 0
#define ALC_TRUE 1

#define ALC_MAJOR_VERSION 0x1000
#define ALC_MINOR_VERSION 0x1001
#define ALC_ATTRIBUTES_SIZE 0x1002
#define ALC_ALL_ATTRIBUTES 0x1003

#define ALC_DEFAULT_DEVICE_SPECIFIER 0x1004
#define ALC_DEVICE_SPECIFIER 0x1005
#define ALC_EXTENSIONS 0x1006
#define ALC_FREQUENCY 0x1007
#define ALC_REFRESH 0x1008
#define ALC_SYNC 0x1009

#define ALC_NO_ERROR 0
#define ALC_INVALID_DEVICE 0xA001
#define ALC_INVALID_CONTEXT 0xA002
#define ALC_INVALID_ENUM 0xA003
#define ALC_INVALID_VALUE 0xA004
#define ALC_OUT_OF_MEMORY 0xA005

#define ALC_FREQUENCY 0x1007
#define ALC_MONO_SOURCES 0x1010
#define ALC_STEREO_SOURCES 0x1011

AL_API ALvoid AL_APIENTRY alEnable(ALenum cap);
AL_API ALvoid AL_APIENTRY alDisable(ALenum cap);
AL_API ALboolean AL_APIENTRY alIsEnabled(ALenum cap);

AL_API const ALchar* AL_APIENTRY alGetString(ALenum param);
AL_API ALvoid AL_APIENTRY alGetBooleanv(ALenum param, ALboolean* data);
AL_API ALvoid AL_APIENTRY alGetIntegerv(ALenum param, ALint* data);
AL_API ALvoid AL_APIENTRY alGetFloatv(ALenum param, ALfloat* data);
AL_API ALvoid AL_APIENTRY alGetDoublev(ALenum param, ALdouble* data);
AL_API ALboolean AL_APIENTRY alGetBoolean(ALenum param);
AL_API ALint AL_APIENTRY alGetInteger(ALenum param);
AL_API ALfloat AL_APIENTRY alGetFloat(ALenum param);
AL_API ALdouble AL_APIENTRY alGetDouble(ALenum param);

AL_API ALenum AL_APIENTRY alGetError(void);

AL_API ALboolean AL_APIENTRY alIsExtensionPresent(const ALchar* extname);

AL_API ALvoid AL_APIENTRY alListenerf(ALenum param, ALfloat value);
AL_API ALvoid AL_APIENTRY alListener3f(ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
AL_API ALvoid AL_APIENTRY alListenerfv(ALenum param, const ALfloat* values);
AL_API ALvoid AL_APIENTRY alListeneri(ALenum param, ALint value);
AL_API ALvoid AL_APIENTRY alListener3i(ALenum param, ALint value1, ALint value2, ALint value3);
AL_API ALvoid AL_APIENTRY alListeneriv(ALenum param, const ALint* values);

AL_API ALvoid AL_APIENTRY alGetListenerf(ALenum param, ALfloat* value);
AL_API ALvoid AL_APIENTRY alGetListener3f(ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3);
AL_API ALvoid AL_APIENTRY alGetListenerfv(ALenum param, ALfloat* values);
AL_API ALvoid AL_APIENTRY alGetListeneri(ALenum param, ALint* value);
AL_API ALvoid AL_APIENTRY alGetListener3i(ALenum param, ALint* value1, ALint* value2, ALint* value3);
AL_API ALvoid AL_APIENTRY alGetListeneriv(ALenum param, ALint* values);

AL_API ALvoid AL_APIENTRY alGenSources(ALsizei n, ALuint* sources);
AL_API ALvoid AL_APIENTRY alDeleteSources(ALsizei n, const ALuint* sources);
AL_API ALboolean AL_APIENTRY alIsSource(ALuint sid);

AL_API ALvoid AL_APIENTRY alSourcef(ALuint sid, ALenum param, ALfloat value);
AL_API ALvoid AL_APIENTRY alSource3f(ALuint sid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
AL_API ALvoid AL_APIENTRY alSourcefv(ALuint sid, ALenum param, const ALfloat* values);
AL_API ALvoid AL_APIENTRY alSourcei(ALuint sid, ALenum param, ALint value);
AL_API ALvoid AL_APIENTRY alSource3i(ALuint sid, ALenum param, ALint value1, ALint value2, ALint value3);
AL_API ALvoid AL_APIENTRY alSourceiv(ALuint sid, ALenum param, const ALint* values);

AL_API ALvoid AL_APIENTRY alGetSourcef(ALuint sid, ALenum param, ALfloat* value);
AL_API ALvoid AL_APIENTRY alGetSource3f(ALuint sid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3);
AL_API ALvoid AL_APIENTRY alGetSourcefv(ALuint sid, ALenum param, ALfloat* values);
AL_API ALvoid AL_APIENTRY alGetSourcei(ALuint sid, ALenum param, ALint* value);
AL_API ALvoid AL_APIENTRY alGetSource3i(ALuint sid, ALenum param, ALint* value1, ALint* value2, ALint* value3);
AL_API ALvoid AL_APIENTRY alGetSourceiv(ALuint sid, ALenum param, ALint* values);

AL_API ALvoid AL_APIENTRY alSourcePlayv(ALsizei n, const ALuint *sources);
AL_API ALvoid AL_APIENTRY alSourceStopv(ALsizei n, const ALuint *sources);
AL_API ALvoid AL_APIENTRY alSourceRewindv(ALsizei n, const ALuint *sources);
AL_API ALvoid AL_APIENTRY alSourcePausev(ALsizei n, const ALuint *sources);
AL_API ALvoid AL_APIENTRY alSourcePlay(ALuint source);
AL_API ALvoid AL_APIENTRY alSourceStop(ALuint source);
AL_API ALvoid AL_APIENTRY alSourceRewind(ALuint source);
AL_API ALvoid AL_APIENTRY alSourcePause(ALuint source);

AL_API ALvoid AL_APIENTRY alSourceQueueBuffers(ALuint source, ALsizei nb, const ALuint* buffers);
AL_API ALvoid AL_APIENTRY alSourceUnqueueBuffers(ALuint source, ALsizei nb, ALuint* buffers);

AL_API ALvoid AL_APIENTRY alGenBuffers(ALsizei n, ALuint* buffers);
AL_API ALvoid AL_APIENTRY alDeleteBuffers(ALsizei n, const ALuint* buffers);
AL_API ALboolean AL_APIENTRY alIsBuffer(ALuint buffer);

AL_API ALvoid AL_APIENTRY alBufferData(ALuint buffer, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq);
AL_API ALvoid AL_APIENTRY alBufferf(ALuint buffer, ALenum param, ALfloat value);
AL_API ALvoid AL_APIENTRY alBuffer3f(ALuint buffer, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3);
AL_API ALvoid AL_APIENTRY alBufferfv(ALuint buffer, ALenum param, const ALfloat* values);
AL_API ALvoid AL_APIENTRY alBufferi(ALuint buffer, ALenum param, ALint value);
AL_API ALvoid AL_APIENTRY alBuffer3i(ALuint buffer, ALenum param, ALint value1, ALint value2, ALint value3);
AL_API ALvoid AL_APIENTRY alBufferiv(ALuint buffer, ALenum param, const ALint* values);

AL_API ALvoid AL_APIENTRY alGetBufferf(ALuint buffer, ALenum param, ALfloat* value);
AL_API ALvoid AL_APIENTRY alGetBuffer3f(ALuint buffer, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3);
AL_API ALvoid AL_APIENTRY alGetBufferfv(ALuint buffer, ALenum param, ALfloat* values);
AL_API ALvoid AL_APIENTRY alGetBufferi(ALuint buffer, ALenum param, ALint* value);
AL_API ALvoid AL_APIENTRY alGetBuffer3i(ALuint buffer, ALenum param, ALint* value1, ALint* value2, ALint* value3);
AL_API ALvoid AL_APIENTRY alGetBufferiv(ALuint buffer, ALenum param, ALint* values);

AL_API void* AL_APIENTRY alcGetProcAddress(ALChenum device, const ALchar* procName);

AL_API ALCboolean AL_APIENTRY alcIsExtensionPresent(ALCdevice* device, const ALchar* extname);
AL_API ALCenum AL_APIENTRY alcGetEnumValue(ALCdevice* device, const ALchar* enumname);

AL_API const ALCchar* AL_APIENTRY alcGetString(ALCdevice* device, ALCenum param);
AL_API ALvoid AL_APIENTRY alcGetIntegerv(ALCdevice* device, ALCenum param, ALsizei size, ALint* dest);

AL_API ALCdevice* AL_APIENTRY alcOpenDevice(const ALCchar* devicename);
AL_API ALCboolean AL_APIENTRY alcCloseDevice(ALCdevice* device);

AL_API ALCcontext* AL_APIENTRY alcCreateContext(ALCdevice* device, const ALCint* attrlist);
AL_API ALCboolean AL_APIENTRY alcMakeContextCurrent(ALCcontext* context);
AL_API ALvoid AL_APIENTRY alcProcessContext(ALCcontext* context);
AL_API ALvoid AL_APIENTRY alcSuspendContext(ALCcontext* context);
AL_API ALvoid AL_APIENTRY alcDestroyContext(ALCcontext* context);

AL_API ALCdevice* AL_APIENTRY alcGetContextsDevice(ALCcontext* context);

inline void alEnable(ALenum) {}
inline void alDisable(ALenum) {}
inline ALboolean alIsEnabled(ALenum) { return 0; }
inline const ALchar* alGetString(ALenum) { return ""; }
inline void alGetBooleanv(ALenum, ALboolean*) {}
inline void alGetIntegerv(ALenum, ALint*) {}
inline void alGetFloatv(ALenum, ALfloat*) {}
inline void alGetDoublev(ALenum, ALdouble*) {}
inline ALboolean alGetBoolean(ALenum) { return 0; }
inline ALint alGetInteger(ALenum) { return 0; }
inline ALfloat alGetFloat(ALenum) { return 0.0f; }
inline ALdouble alGetDouble(ALenum) { return 0.0; }
inline ALenum alGetError(void) { return 0; }
inline ALboolean alIsExtensionPresent(const ALchar*) { return 0; }
inline void alListenerf(ALenum, ALfloat) {}
inline void alListener3f(ALenum, ALfloat, ALfloat, ALfloat) {}
inline void alListenerfv(ALenum, const ALfloat*) {}
inline void alListeneri(ALenum, ALint) {}
inline void alListener3i(ALenum, ALint, ALint, ALint) {}
inline void alListeneriv(ALenum, const ALint*) {}
inline void alGetListenerf(ALenum, ALfloat*) {}
inline void alGetListener3f(ALenum, ALfloat*, ALfloat*, ALfloat*) {}
inline void alGetListenerfv(ALenum, ALfloat*) {}
inline void alGetListeneri(ALenum, ALint*) {}
inline void alGetListener3i(ALenum, ALint*, ALint*, ALint*) {}
inline void alGetListeneriv(ALenum, ALint*) {}
inline void alGenSources(ALsizei, ALuint*) {}
inline void alDeleteSources(ALsizei, const ALuint*) {}
inline ALboolean alIsSource(ALuint) { return 0; }
inline void alSourcef(ALuint, ALenum, ALfloat) {}
inline void alSource3f(ALuint, ALenum, ALfloat, ALfloat, ALfloat) {}
inline void alSourcefv(ALuint, ALenum, const ALfloat*) {}
inline void alSourcei(ALuint, ALenum, ALint) {}
inline void alSource3i(ALuint, ALenum, ALint, ALint, ALint) {}
inline void alSourceiv(ALuint, ALenum, const ALint*) {}
inline void alGetSourcef(ALuint, ALenum, ALfloat*) {}
inline void alGetSource3f(ALuint, ALenum, ALfloat*, ALfloat*, ALfloat*) {}
inline void alGetSourcefv(ALuint, ALenum, ALfloat*) {}
inline void alGetSourcei(ALuint, ALenum, ALint*) {}
inline void alGetSource3i(ALuint, ALenum, ALint*, ALint*, ALint*) {}
inline void alGetSourceiv(ALuint, ALenum, ALint*) {}
inline void alSourcePlayv(ALsizei, const ALuint*) {}
inline void alSourceStopv(ALsizei, const ALuint*) {}
inline void alSourceRewindv(ALsizei, const ALuint*) {}
inline void alSourcePausev(ALsizei, const ALuint*) {}
inline void alSourcePlay(ALuint) {}
inline void alSourceStop(ALuint) {}
inline void alSourceRewind(ALuint) {}
inline void alSourcePause(ALuint) {}
inline void alSourceQueueBuffers(ALuint, ALsizei, const ALuint*) {}
inline void alSourceUnqueueBuffers(ALuint, ALsizei, ALuint*) {}
inline void alGenBuffers(ALsizei, ALuint*) {}
inline void alDeleteBuffers(ALsizei, const ALuint*) {}
inline ALboolean alIsBuffer(ALuint) { return 0; }
inline void alBufferData(ALuint, ALenum, const ALvoid*, ALsizei, ALsizei) {}
inline void alBufferf(ALuint, ALenum, ALfloat) {}
inline void alBuffer3f(ALuint, ALenum, ALfloat, ALfloat, ALfloat) {}
inline void alBufferfv(ALuint, ALenum, const ALfloat*) {}
inline void alBufferi(ALuint, ALenum, ALint) {}
inline void alBuffer3i(ALuint, ALenum, ALint, ALint, ALint) {}
inline void alBufferiv(ALuint, ALenum, const ALint*) {}
inline void alGetBufferf(ALuint, ALenum, ALfloat*) {}
inline void alGetBuffer3f(ALuint, ALenum, ALfloat*, ALfloat*, ALfloat*) {}
inline void alGetBufferfv(ALuint, ALenum, ALfloat*) {}
inline void alGetBufferi(ALuint, ALenum, ALint*) {}
inline void alGetBuffer3i(ALuint, ALenum, ALint*, ALint*, ALint*) {}
inline void alGetBufferiv(ALuint, ALenum, ALint*) {}

inline ALCcontext* alcCreateContext(ALCdevice*, const ALCint*) { return nullptr; }
inline ALCboolean alcMakeContextCurrent(ALCcontext*) { return 0; }
inline void alcProcessContext(ALCcontext*) {}
inline void alcSuspendContext(ALCcontext*) {}
inline void alcDestroyContext(ALCcontext*) {}
inline ALCdevice* alcGetContextsDevice(ALCcontext*) { return nullptr; }
inline ALCboolean alcCloseDevice(ALCdevice*) { return 0; }
inline ALCdevice* alcOpenDevice(const ALCchar*) { return nullptr; }
inline const ALCchar* alcGetString(ALCdevice*, ALCenum) { return ""; }
inline void alcGetIntegerv(ALCdevice*, ALCenum, ALsizei, ALint*) {}
inline ALCboolean alcIsExtensionPresent(ALCdevice*, const ALchar*) { return 0; }
inline ALCenum alcGetEnumValue(ALCdevice*, const ALchar*) { return 0; }

#endif
