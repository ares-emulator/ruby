#if defined(PLATFORM_MACOSX)
  #include <OpenAL/al.h>
  #include <OpenAL/alc.h>
#else
  #include <AL/al.h>
  #include <AL/alc.h>
#endif

struct AudioOpenAL : Audio {
  ~AudioOpenAL() { term(); }

  struct {
    ALCdevice* handle = nullptr;
    ALCcontext* context = nullptr;
    ALuint source = 0;
    ALenum format = AL_FORMAT_STEREO16;
    unsigned latency = 0;
    unsigned queueLength = 0;
  } device;

  struct {
    uint32_t* data = nullptr;
    unsigned length = 0;
    unsigned size = 0;
  } buffer;

  struct {
    bool synchronize = true;
    unsigned frequency = 22050;
    unsigned latency = 40;
  } settings;

  auto cap(const string& name) -> bool {
    if(name == Audio::Synchronize) return true;
    if(name == Audio::Frequency) return true;
    if(name == Audio::Latency) return true;
    return false;
  }

  auto get(const string& name) -> any {
    if(name == Audio::Synchronize) return settings.synchronize;
    if(name == Audio::Frequency) return settings.frequency;
    if(name == Audio::Latency) return settings.latency;
    return {};
  }

  auto set(const string& name, const any& value) -> bool {
    if(name == Audio::Synchronize && value.is<bool>()) {
      settings.synchronize = value.get<bool>();
      return true;
    }

    if(name == Audio::Frequency && value.is<unsigned>()) {
      settings.frequency = value.get<unsigned>();
      return true;
    }

    if(name == Audio::Latency && value.is<unsigned>()) {
      if(settings.latency != value.get<unsigned>()) {
        settings.latency = value.get<unsigned>();
        updateLatency();
      }
      return true;
    }

    return false;
  }

  auto sample(uint16_t left, uint16_t right) -> void {
    buffer.data[buffer.length++] = left << 0 | right << 16;
    if(buffer.length < buffer.size) return;

    ALuint albuffer = 0;
    int processed = 0;
    while(true) {
      alGetSourcei(device.source, AL_BUFFERS_PROCESSED, &processed);
      while(processed--) {
        alSourceUnqueueBuffers(device.source, 1, &albuffer);
        alDeleteBuffers(1, &albuffer);
        device.queueLength--;
      }
      //wait for buffer playback to catch up to sample generation if not synchronizing
      if(settings.synchronize == false || device.queueLength < 3) break;
    }

    if(device.queueLength < 3) {
      alGenBuffers(1, &albuffer);
      alBufferData(albuffer, device.format, buffer.data, buffer.size * 4, settings.frequency);
      alSourceQueueBuffers(device.source, 1, &albuffer);
      device.queueLength++;
    }

    ALint playing;
    alGetSourcei(device.source, AL_SOURCE_STATE, &playing);
    if(playing != AL_PLAYING) alSourcePlay(device.source);
    buffer.length = 0;
  }

  auto clear() -> void {
  }

  auto init() -> bool {
    updateLatency();
    device.queueLength = 0;

    bool success = false;
    if(device.handle = alcOpenDevice(nullptr)) {
      if(device.context = alcCreateContext(device.handle, nullptr)) {
        alcMakeContextCurrent(device.context);
        alGenSources(1, &device.source);

      //alSourcef (device.source, AL_PITCH, 1.0);
      //alSourcef (device.source, AL_GAIN, 1.0);
      //alSource3f(device.source, AL_POSITION, 0.0, 0.0, 0.0);
      //alSource3f(device.source, AL_VELOCITY, 0.0, 0.0, 0.0);
      //alSource3f(device.source, AL_DIRECTION, 0.0, 0.0, 0.0);
      //alSourcef (device.source, AL_ROLLOFF_FACTOR, 0.0);
      //alSourcei (device.source, AL_SOURCE_RELATIVE, AL_TRUE);

        alListener3f(AL_POSITION, 0.0, 0.0, 0.0);
        alListener3f(AL_VELOCITY, 0.0, 0.0, 0.0);
        ALfloat listener_orientation[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        alListenerfv(AL_ORIENTATION, listener_orientation);

        success = true;
      }
    }

    if(success == false) {
      term();
      return false;
    }

    return true;
  }

  auto term() -> void {
    if(alIsSource(device.source) == AL_TRUE) {
      int playing = 0;
      alGetSourcei(device.source, AL_SOURCE_STATE, &playing);
      if(playing == AL_PLAYING) {
        alSourceStop(device.source);
        int queued = 0;
        alGetSourcei(device.source, AL_BUFFERS_QUEUED, &queued);
        while(queued--) {
          ALuint albuffer = 0;
          alSourceUnqueueBuffers(device.source, 1, &albuffer);
          alDeleteBuffers(1, &albuffer);
          device.queueLength--;
        }
      }

      alDeleteSources(1, &device.source);
      device.source = 0;
    }

    if(device.context) {
      alcMakeContextCurrent(nullptr);
      alcDestroyContext(device.context);
      device.context = 0;
    }

    if(device.handle) {
      alcCloseDevice(device.handle);
      device.handle = 0;
    }

    if(buffer.data) {
      delete[] buffer.data;
      buffer.data = 0;
    }
  }

private:
  auto queryDevices() -> lstring {
    lstring result;

    const char* buffer = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
    if(!buffer) return result;

    while(buffer[0] || buffer[1]) {
      result.append(buffer);
      while(buffer[0]) buffer++;
    }

    return result;
  }

  auto updateLatency() -> void {
    if(buffer.data) delete[] buffer.data;
    buffer.size = settings.frequency * settings.latency / 1000.0 + 0.5;
    buffer.data = new uint32_t[buffer.size]();
  }
};
