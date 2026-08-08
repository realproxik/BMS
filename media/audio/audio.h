// audio.h - BMS Audio System Header
#ifndef BMS_AUDIO_H
#define BMS_AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Audio Enumerations
// ============================================================================

typedef enum {
    AUDIO_STATUS_STOPPED = 0,
    AUDIO_STATUS_PLAYING = 1,
    AUDIO_STATUS_PAUSED = 2,
    AUDIO_STATUS_BUFFERING = 3,
    AUDIO_STATUS_LOADING = 4,
    AUDIO_STATUS_ERROR = 5,
    AUDIO_STATUS_READY = 6
} AudioStatus;

typedef enum {
    AUDIO_FORMAT_UNKNOWN = 0,
    AUDIO_FORMAT_PCM_S16LE = 1,
    AUDIO_FORMAT_PCM_S24LE = 2,
    AUDIO_FORMAT_PCM_S32LE = 3,
    AUDIO_FORMAT_PCM_F32LE = 4,
    AUDIO_FORMAT_MP3 = 5,
    AUDIO_FORMAT_AAC = 6,
    AUDIO_FORMAT_OPUS = 7,
    AUDIO_FORMAT_VORBIS = 8,
    AUDIO_FORMAT_FLAC = 9,
    AUDIO_FORMAT_WAV = 10,
    AUDIO_FORMAT_OGG = 11,
    AUDIO_FORMAT_WEBM = 12
} AudioFormat;

typedef enum {
    AUDIO_DEVICE_DEFAULT = 0,
    AUDIO_DEVICE_SPEAKERS = 1,
    AUDIO_DEVICE_HEADPHONES = 2,
    AUDIO_DEVICE_MICROPHONE = 3,
    AUDIO_DEVICE_LINE_IN = 4,
    AUDIO_DEVICE_LINE_OUT = 5,
    AUDIO_DEVICE_HDMI = 6,
    AUDIO_DEVICE_USB = 7,
    AUDIO_DEVICE_BLUETOOTH = 8
} AudioDeviceType;

typedef enum {
    AUDIO_EFFECT_NONE = 0,
    AUDIO_EFFECT_REVERB = 1,
    AUDIO_EFFECT_CHORUS = 2,
    AUDIO_EFFECT_FLANGER = 3,
    AUDIO_EFFECT_PHASER = 4,
    AUDIO_EFFECT_WAH = 5,
    AUDIO_EFFECT_DISTORTION = 6,
    AUDIO_EFFECT_COMPRESSOR = 7,
    AUDIO_EFFECT_EQUALIZER = 8,
    AUDIO_EFFECT_DELAY = 9,
    AUDIO_EFFECT_ECHO = 10
} AudioEffect;

// ============================================================================
// Audio Structures
// ============================================================================

typedef struct AudioBuffer {
    uint8_t* data;
    size_t size;
    size_t sampleRate;
    int channels;
    int bitsPerSample;
    AudioFormat format;
    uint64_t durationMs;
    uint64_t timestamp;
    void* userData;
} AudioBuffer;

typedef struct AudioStream {
    uint32_t id;
    char* url;
    char* filePath;
    AudioStatus status;
    AudioFormat format;
    uint32_t sampleRate;
    uint8_t channels;
    uint32_t bitrate;
    uint64_t durationMs;
    uint64_t positionMs;
    float volume;
    float balance;
    bool loop;
    bool mute;
    bool playing;
    void* handle;
    void* decoder;
    void* device;
    void* userData;
} AudioStream;

typedef struct AudioDevice {
    uint32_t id;
    char* name;
    char* description;
    AudioDeviceType type;
    bool isDefault;
    bool isActive;
    uint32_t sampleRate;
    uint8_t channels;
    uint32_t bufferSize;
    uint32_t latencyMs;
    void* handle;
    void* userData;
} AudioDevice;

typedef struct AudioEffect {
    uint32_t id;
    AudioEffect type;
    char* name;
    bool enabled;
    float parameters[16];
    void* handle;
    void* userData;
} AudioEffect;

typedef struct AudioMixer {
    uint32_t id;
    char* name;
    float masterVolume;
    float masterBalance;
    bool muteAll;
    AudioStream** streams;
    uint32_t streamCount;
    AudioEffect** effects;
    uint32_t effectCount;
    void* handle;
    void* userData;
} AudioMixer;

// ============================================================================
// Audio Callback Types
// ============================================================================

typedef void (*AudioCallback)(void* userData, AudioBuffer* buffer);
typedef void (*AudioStreamCallback)(AudioStream* stream, void* userData);
typedef void (*AudioDeviceCallback)(AudioDevice* device, void* userData);
typedef void (*AudioEffectCallback)(AudioEffect* effect, void* userData);
typedef void (*AudioMixerCallback)(AudioMixer* mixer, void* userData);

// ============================================================================
// Audio System Functions
// ============================================================================

// System initialization and shutdown
int audio_system_init(void);
void audio_system_shutdown(void);
bool audio_system_is_initialized(void);
const char* audio_get_version(void);

// Device management
AudioDevice* audio_device_get_default(void);
AudioDevice* audio_device_get_by_id(uint32_t id);
AudioDevice* audio_device_get_by_type(AudioDeviceType type);
AudioDevice* audio_device_create(const char* name, AudioDeviceType type);
void audio_device_destroy(AudioDevice* device);
bool audio_device_is_available(uint32_t id);
void audio_device_set_volume(AudioDevice* device, float volume);
float audio_device_get_volume(AudioDevice* device);

// Stream management
AudioStream* audio_stream_create(const char* url);
AudioStream* audio_stream_create_from_file(const char* filePath);
void audio_stream_destroy(AudioStream* stream);
int audio_stream_load(AudioStream* stream);
int audio_stream_play(AudioStream* stream);
int audio_stream_pause(AudioStream* stream);
int audio_stream_stop(AudioStream* stream);
int audio_stream_resume(AudioStream* stream);
bool audio_stream_is_playing(AudioStream* stream);
void audio_stream_set_volume(AudioStream* stream, float volume);
float audio_stream_get_volume(AudioStream* stream);
void audio_stream_set_balance(AudioStream* stream, float balance);
float audio_stream_get_balance(AudioStream* stream);
void audio_stream_set_loop(AudioStream* stream, bool loop);
bool audio_stream_get_loop(AudioStream* stream);
void audio_stream_set_position(AudioStream* stream, uint64_t positionMs);
uint64_t audio_stream_get_position(AudioStream* stream);
uint64_t audio_stream_get_duration(AudioStream* stream);
AudioStatus audio_stream_get_status(AudioStream* stream);
void audio_stream_set_callback(AudioStream* stream, AudioStreamCallback callback, void* userData);
void audio_stream_attach_device(AudioStream* stream, AudioDevice* device);
void audio_stream_detach_device(AudioStream* stream);

// Buffer management
AudioBuffer* audio_buffer_create(size_t size, uint32_t sampleRate, int channels);
void audio_buffer_destroy(AudioBuffer* buffer);
int audio_buffer_fill(AudioBuffer* buffer, const uint8_t* data, size_t size);
int audio_buffer_clear(AudioBuffer* buffer);
size_t audio_buffer_get_size(AudioBuffer* buffer);
uint64_t audio_buffer_get_duration(AudioBuffer* buffer);

// Effect management
AudioEffect* audio_effect_create(AudioEffect type, const char* name);
void audio_effect_destroy(AudioEffect* effect);
void audio_effect_enable(AudioEffect* effect, bool enable);
bool audio_effect_is_enabled(AudioEffect* effect);
void audio_effect_set_parameter(AudioEffect* effect, int index, float value);
float audio_effect_get_parameter(AudioEffect* effect, int index);
void audio_effect_apply(AudioEffect* effect, AudioBuffer* buffer);
AudioEffect* audio_effect_chain_create(void);
void audio_effect_chain_destroy(AudioEffect* chain);
void audio_effect_chain_add(AudioEffect* chain, AudioEffect* effect);
void audio_effect_chain_remove(AudioEffect* chain, AudioEffect* effect);

// Mixer management
AudioMixer* audio_mixer_create(const char* name);
void audio_mixer_destroy(AudioMixer* mixer);
void audio_mixer_add_stream(AudioMixer* mixer, AudioStream* stream);
void audio_mixer_remove_stream(AudioMixer* mixer, AudioStream* stream);
void audio_mixer_add_effect(AudioMixer* mixer, AudioEffect* effect);
void audio_mixer_remove_effect(AudioMixer* mixer, AudioEffect* effect);
void audio_mixer_set_master_volume(AudioMixer* mixer, float volume);
float audio_mixer_get_master_volume(AudioMixer* mixer);
void audio_mixer_set_master_balance(AudioMixer* mixer, float balance);
float audio_mixer_get_master_balance(AudioMixer* mixer);
void audio_mixer_mute_all(AudioMixer* mixer, bool mute);
bool audio_mixer_is_muted(AudioMixer* mixer);
void audio_mixer_process(AudioMixer* mixer, AudioBuffer* buffer);

// Volume control functions
float audio_volume_linear_to_db(float linear);
float audio_volume_db_to_linear(float db);
float audio_volume_linear_to_percent(float linear);
float audio_volume_percent_to_linear(float percent);
void audio_volume_ramp(AudioStream* stream, float targetVolume, uint32_t durationMs);

// Audio processing functions
int audio_resample(AudioBuffer* src, AudioBuffer* dst, uint32_t targetSampleRate);
int audio_convert_format(AudioBuffer* src, AudioBuffer* dst, AudioFormat targetFormat);
int audio_mix_buffers(AudioBuffer* dest, AudioBuffer* src, float volume);
int audio_normalize(AudioBuffer* buffer);
int audio_apply_gain(AudioBuffer* buffer, float gain);
int audio_fade_in(AudioBuffer* buffer, uint32_t durationMs);
int audio_fade_out(AudioBuffer* buffer, uint32_t durationMs);

// Utility functions
const char* audio_format_to_string(AudioFormat format);
const char* audio_status_to_string(AudioStatus status);
const char* audio_device_type_to_string(AudioDeviceType type);
const char* audio_effect_to_string(AudioEffect effect);
AudioFormat audio_guess_format_from_file(const char* filePath);
AudioFormat audio_guess_format_from_url(const char* url);

// BMS Browser specific audio functions
int audio_bms_browser_audio_init(void);
void audio_bms_browser_audio_shutdown(void);
int audio_bms_browser_audio_play_tab_sound(uint32_t tabId);
int audio_bms_browser_audio_play_notification(const char* soundFile);
int audio_bms_browser_audio_record_microphone(const char* outputFile);
int audio_bms_browser_audio_capture_screen_audio(void);
int audio_bms_browser_audio_stream_web_audio(const char* url);

// Volume control structures
typedef struct AudioVolumeControl {
    float masterVolume;
    float applicationVolume;
    float systemVolume;
    float communicationVolume;
    bool muteAll;
    bool muteSystem;
    bool muteApplications;
    bool muteCommunications;
    uint32_t deviceCount;
    float deviceVolumes[32];
    char* deviceNames[32];
    void* userData;
} AudioVolumeControl;

// Volume control functions
AudioVolumeControl* audio_volume_control_create(void);
void audio_volume_control_destroy(AudioVolumeControl* control);
void audio_volume_control_set_master(AudioVolumeControl* control, float volume);
float audio_volume_control_get_master(AudioVolumeControl* control);
void audio_volume_control_set_application(AudioVolumeControl* control, float volume);
float audio_volume_control_get_application(AudioVolumeControl* control);
void audio_volume_control_set_system(AudioVolumeControl* control, float volume);
float audio_volume_control_get_system(AudioVolumeControl* control);
void audio_volume_control_set_communication(AudioVolumeControl* control, float volume);
float audio_volume_control_get_communication(AudioVolumeControl* control);
void audio_volume_control_set_mute_all(AudioVolumeControl* control, bool mute);
bool audio_volume_control_get_mute_all(AudioVolumeControl* control);
void audio_volume_control_set_device_volume(AudioVolumeControl* control, uint32_t deviceIndex, float volume);
float audio_volume_control_get_device_volume(AudioVolumeControl* control, uint32_t deviceIndex);
void audio_volume_control_save(AudioVolumeControl* control, const char* path);
void audio_volume_control_load(AudioVolumeControl* control, const char* path);

#ifdef __cplusplus
}
#endif

#endif // BMS_AUDIO_H