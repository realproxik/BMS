// audio.cc - BMS Audio System Implementation (C++)
#include "audio.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <memory>
#include <functional>
#include <algorithm>
#include <chrono>

namespace BMS {
namespace Audio {

// ============================================================================
// Audio Namespace Implementation
// ============================================================================

class AudioSystem {
public:
    static AudioSystem* GetInstance() {
        static AudioSystem instance;
        return &instance;
    }

    bool Initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (initialized_) return true;

        std::cout << "[BMS Audio] Initializing C++ audio system..." << std::endl;

        // Initialize master mixer
        masterMixer_ = std::make_unique<AudioMixer>();
        masterMixer_->name = "Master Mixer";
        masterMixer_->masterVolume = 1.0f;

        // Initialize volume control
        volumeControl_ = std::make_unique<AudioVolumeControl>();
        volumeControl_->masterVolume = 1.0f;

        initialized_ = true;
        std::cout << "[BMS Audio] C++ audio system initialized" << std::endl;

        return true;
    }

    void Shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!initialized_) return;

        std::cout << "[BMS Audio] Shutting down C++ audio system..." << std::endl;

        // Clear all streams
        streams_.clear();

        // Clear all devices
        devices_.clear();

        // Clear all effects
        effects_.clear();

        masterMixer_.reset();
        volumeControl_.reset();

        initialized_ = false;
        std::cout << "[BMS Audio] C++ audio system shut down" << std::endl;
    }

    bool IsInitialized() const {
        return initialized_;
    }

    std::string GetVersion() const {
        return "BMS Audio C++ v1.0.0";
    }

    // Stream management
    std::shared_ptr<AudioStream> CreateStream(const std::string& url) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto stream = std::make_shared<AudioStream>();
        stream->id = GenerateId();
        stream->url = url;
        stream->status = AudioStatus::LOADING;
        stream->volume = 1.0f;
        stream->balance = 0.0f;
        stream->playing = false;

        streams_[stream->id] = stream;
        std::cout << "[BMS Audio] Created stream: " << url << std::endl;

        return stream;
    }

    std::shared_ptr<AudioStream> CreateStreamFromFile(const std::string& filePath) {
        auto stream = CreateStream(filePath);
        if (stream) {
            stream->filePath = filePath;
            stream->format = GuessFormatFromFile(filePath);
        }
        return stream;
    }

    bool DestroyStream(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = streams_.find(id);
        if (it == streams_.end()) return false;

        streams_.erase(it);
        std::cout << "[BMS Audio] Destroyed stream: " << id << std::endl;

        return true;
    }

    std::shared_ptr<AudioStream> GetStream(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = streams_.find(id);
        if (it == streams_.end()) return nullptr;
        return it->second;
    }

    std::vector<std::shared_ptr<AudioStream>> GetAllStreams() {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<std::shared_ptr<AudioStream>> result;
        for (const auto& [id, stream] : streams_) {
            result.push_back(stream);
        }
        return result;
    }

    // Stream operations
    bool PlayStream(uint32_t id) {
        auto stream = GetStream(id);
        if (!stream) return false;

        stream->status = AudioStatus::PLAYING;
        stream->playing = true;
        std::cout << "[BMS Audio] Playing stream: " << stream->url << std::endl;

        return true;
    }

    bool PauseStream(uint32_t id) {
        auto stream = GetStream(id);
        if (!stream) return false;

        stream->status = AudioStatus::PAUSED;
        stream->playing = false;
        std::cout << "[BMS Audio] Paused stream: " << stream->url << std::endl;

        return true;
    }

    bool StopStream(uint32_t id) {
        auto stream = GetStream(id);
        if (!stream) return false;

        stream->status = AudioStatus::STOPPED;
        stream->playing = false;
        stream->positionMs = 0;
        std::cout << "[BMS Audio] Stopped stream: " << stream->url << std::endl;

        return true;
    }

    bool SetStreamVolume(uint32_t id, float volume) {
        auto stream = GetStream(id);
        if (!stream) return false;

        stream->volume = std::clamp(volume, 0.0f, 1.0f);
        return true;
    }

    float GetStreamVolume(uint32_t id) {
        auto stream = GetStream(id);
        return stream ? stream->volume : 0.0f;
    }

    // Volume control
    float GetMasterVolume() const {
        return volumeControl_ ? volumeControl_->masterVolume : 1.0f;
    }

    void SetMasterVolume(float volume) {
        if (!volumeControl_) return;
        volumeControl_->masterVolume = std::clamp(volume, 0.0f, 1.0f);
        std::cout << "[BMS Audio] Master volume: " << volumeControl_->masterVolume << std::endl;
    }

    float GetApplicationVolume() const {
        return volumeControl_ ? volumeControl_->applicationVolume : 1.0f;
    }

    void SetApplicationVolume(float volume) {
        if (!volumeControl_) return;
        volumeControl_->applicationVolume = std::clamp(volume, 0.0f, 1.0f);
    }

    float GetSystemVolume() const {
        return volumeControl_ ? volumeControl_->systemVolume : 1.0f;
    }

    void SetSystemVolume(float volume) {
        if (!volumeControl_) return;
        volumeControl_->systemVolume = std::clamp(volume, 0.0f, 1.0f);
    }

    float GetCommunicationVolume() const {
        return volumeControl_ ? volumeControl_->communicationVolume : 1.0f;
    }

    void SetCommunicationVolume(float volume) {
        if (!volumeControl_) return;
        volumeControl_->communicationVolume = std::clamp(volume, 0.0f, 1.0f);
    }

    bool GetMuteAll() const {
        return volumeControl_ ? volumeControl_->muteAll : false;
    }

    void SetMuteAll(bool mute) {
        if (!volumeControl_) return;
        volumeControl_->muteAll = mute;
        std::cout << "[BMS Audio] Mute all: " << (mute ? "ON" : "OFF") << std::endl;
    }

    // Device management
    bool AddDevice(const std::string& name, AudioDeviceType type) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto device = std::make_shared<AudioDevice>();
        device->id = GenerateId();
        device->name = name;
        device->type = type;
        device->isActive = true;
        device->sampleRate = 44100;
        device->channels = 2;

        devices_[device->id] = device;
        std::cout << "[BMS Audio] Added device: " << name << std::endl;

        return true;
    }

    bool RemoveDevice(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = devices_.find(id);
        if (it == devices_.end()) return false;

        devices_.erase(it);
        std::cout << "[BMS Audio] Removed device: " << id << std::endl;

        return true;
    }

    std::shared_ptr<AudioDevice> GetDevice(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = devices_.find(id);
        if (it == devices_.end()) return nullptr;
        return it->second;
    }

    std::vector<std::shared_ptr<AudioDevice>> GetAllDevices() {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<std::shared_ptr<AudioDevice>> result;
        for (const auto& [id, device] : devices_) {
            result.push_back(device);
        }
        return result;
    }

    // Effect management
    std::shared_ptr<AudioEffect> CreateEffect(AudioEffectType type, const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto effect = std::make_shared<AudioEffect>();
        effect->id = GenerateId();
        effect->type = type;
        effect->name = name;
        effect->enabled = true;

        // Initialize default parameters
        for (int i = 0; i < 16; i++) {
            effect->parameters[i] = 0.5f;
        }

        effects_[effect->id] = effect;
        std::cout << "[BMS Audio] Created effect: " << name << std::endl;

        return effect;
    }

    bool DestroyEffect(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = effects_.find(id);
        if (it == effects_.end()) return false;

        effects_.erase(it);
        std::cout << "[BMS Audio] Destroyed effect: " << id << std::endl;

        return true;
    }

    std::shared_ptr<AudioEffect> GetEffect(uint32_t id) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = effects_.find(id);
        if (it == effects_.end()) return nullptr;
        return it->second;
    }

    // Mixer management
    std::shared_ptr<AudioMixer> GetMasterMixer() {
        return masterMixer_;
    }

    // Utility functions
    std::string FormatToString(AudioFormat format) const {
        switch (format) {
            case AudioFormat::UNKNOWN: return "Unknown";
            case AudioFormat::PCM_S16LE: return "PCM S16LE";
            case AudioFormat::PCM_S24LE: return "PCM S24LE";
            case AudioFormat::PCM_S32LE: return "PCM S32LE";
            case AudioFormat::PCM_F32LE: return "PCM F32LE";
            case AudioFormat::MP3: return "MP3";
            case AudioFormat::AAC: return "AAC";
            case AudioFormat::OPUS: return "Opus";
            case AudioFormat::VORBIS: return "Vorbis";
            case AudioFormat::FLAC: return "FLAC";
            case AudioFormat::WAV: return "WAV";
            case AudioFormat::OGG: return "OGG";
            case AudioFormat::WEBM: return "WebM";
            default: return "Invalid";
        }
    }

    std::string StatusToString(AudioStatus status) const {
        switch (status) {
            case AudioStatus::STOPPED: return "Stopped";
            case AudioStatus::PLAYING: return "Playing";
            case AudioStatus::PAUSED: return "Paused";
            case AudioStatus::BUFFERING: return "Buffering";
            case AudioStatus::LOADING: return "Loading";
            case AudioStatus::ERROR: return "Error";
            case AudioStatus::READY: return "Ready";
            default: return "Unknown";
        }
    }

private:
    AudioSystem() : initialized_(false), idCounter_(0) {}
    ~AudioSystem() = default;

    uint32_t GenerateId() {
        return ++idCounter_;
    }

    AudioFormat GuessFormatFromFile(const std::string& filePath) {
        size_t dotPos = filePath.rfind('.');
        if (dotPos == std::string::npos) return AudioFormat::UNKNOWN;

        std::string ext = filePath.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".mp3") return AudioFormat::MP3;
        if (ext == ".wav") return AudioFormat::WAV;
        if (ext == ".ogg") return AudioFormat::OGG;
        if (ext == ".aac") return AudioFormat::AAC;
        if (ext == ".flac") return AudioFormat::FLAC;
        if (ext == ".opus") return AudioFormat::OPUS;
        if (ext == ".webm") return AudioFormat::WEBM;

        return AudioFormat::UNKNOWN;
    }

    // Private members
    bool initialized_;
    std::mutex mutex_;
    std::atomic<uint32_t> idCounter_;

    std::map<uint32_t, std::shared_ptr<AudioStream>> streams_;
    std::map<uint32_t, std::shared_ptr<AudioDevice>> devices_;
    std::map<uint32_t, std::shared_ptr<AudioEffect>> effects_;

    std::shared_ptr<AudioMixer> masterMixer_;
    std::unique_ptr<AudioVolumeControl> volumeControl_;
};

// ============================================================================
// C++ API Functions (C++11 Standard)
// ============================================================================

// System functions
bool Initialize() {
    return AudioSystem::GetInstance()->Initialize();
}

void Shutdown() {
    AudioSystem::GetInstance()->Shutdown();
}

bool IsInitialized() {
    return AudioSystem::GetInstance()->IsInitialized();
}

std::string GetVersion() {
    return AudioSystem::GetInstance()->GetVersion();
}

// Stream functions
std::shared_ptr<AudioStream> CreateStream(const std::string& url) {
    return AudioSystem::GetInstance()->CreateStream(url);
}

std::shared_ptr<AudioStream> CreateStreamFromFile(const std::string& filePath) {
    return AudioSystem::GetInstance()->CreateStreamFromFile(filePath);
}

bool DestroyStream(uint32_t id) {
    return AudioSystem::GetInstance()->DestroyStream(id);
}

std::shared_ptr<AudioStream> GetStream(uint32_t id) {
    return AudioSystem::GetInstance()->GetStream(id);
}

std::vector<std::shared_ptr<AudioStream>> GetAllStreams() {
    return AudioSystem::GetInstance()->GetAllStreams();
}

bool PlayStream(uint32_t id) {
    return AudioSystem::GetInstance()->PlayStream(id);
}

bool PauseStream(uint32_t id) {
    return AudioSystem::GetInstance()->PauseStream(id);
}

bool StopStream(uint32_t id) {
    return AudioSystem::GetInstance()->StopStream(id);
}

bool SetStreamVolume(uint32_t id, float volume) {
    return AudioSystem::GetInstance()->SetStreamVolume(id, volume);
}

float GetStreamVolume(uint32_t id) {
    return AudioSystem::GetInstance()->GetStreamVolume(id);
}

// Volume control functions
float GetMasterVolume() {
    return AudioSystem::GetInstance()->GetMasterVolume();
}

void SetMasterVolume(float volume) {
    AudioSystem::GetInstance()->SetMasterVolume(volume);
}

float GetApplicationVolume() {
    return AudioSystem::GetInstance()->GetApplicationVolume();
}

void SetApplicationVolume(float volume) {
    AudioSystem::GetInstance()->SetApplicationVolume(volume);
}

float GetSystemVolume() {
    return AudioSystem::GetInstance()->GetSystemVolume();
}

void SetSystemVolume(float volume) {
    AudioSystem::GetInstance()->SetSystemVolume(volume);
}

float GetCommunicationVolume() {
    return AudioSystem::GetInstance()->GetCommunicationVolume();
}

void SetCommunicationVolume(float volume) {
    AudioSystem::GetInstance()->SetCommunicationVolume(volume);
}

bool GetMuteAll() {
    return AudioSystem::GetInstance()->GetMuteAll();
}

void SetMuteAll(bool mute) {
    AudioSystem::GetInstance()->SetMuteAll(mute);
}

// Device functions
bool AddDevice(const std::string& name, AudioDeviceType type) {
    return AudioSystem::GetInstance()->AddDevice(name, type);
}

bool RemoveDevice(uint32_t id) {
    return AudioSystem::GetInstance()->RemoveDevice(id);
}

std::shared_ptr<AudioDevice> GetDevice(uint32_t id) {
    return AudioSystem::GetInstance()->GetDevice(id);
}

std::vector<std::shared_ptr<AudioDevice>> GetAllDevices() {
    return AudioSystem::GetInstance()->GetAllDevices();
}

// Effect functions
std::shared_ptr<AudioEffect> CreateEffect(AudioEffectType type, const std::string& name) {
    return AudioSystem::GetInstance()->CreateEffect(type, name);
}

bool DestroyEffect(uint32_t id) {
    return AudioSystem::GetInstance()->DestroyEffect(id);
}

std::shared_ptr<AudioEffect> GetEffect(uint32_t id) {
    return AudioSystem::GetInstance()->GetEffect(id);
}

// Mixer functions
std::shared_ptr<AudioMixer> GetMasterMixer() {
    return AudioSystem::GetInstance()->GetMasterMixer();
}

// Utility functions
std::string FormatToString(AudioFormat format) {
    return AudioSystem::GetInstance()->FormatToString(format);
}

std::string StatusToString(AudioStatus status) {
    return AudioSystem::GetInstance()->StatusToString(status);
}

// ============================================================================
// C Compatibility Wrappers
// ============================================================================

extern "C" {

int audio_system_init(void) {
    return BMS::Audio::Initialize() ? 0 : -1;
}

void audio_system_shutdown(void) {
    BMS::Audio::Shutdown();
}

bool audio_system_is_initialized(void) {
    return BMS::Audio::IsInitialized();
}

const char* audio_get_version(void) {
    static std::string version = BMS::Audio::GetVersion();
    return version.c_str();
}

// Stream wrappers
AudioStream* audio_stream_create(const char* url) {
    auto stream = BMS::Audio::CreateStream(url);
    if (!stream) return nullptr;

    // Transfer ownership to C memory
    AudioStream* cStream = (AudioStream*)malloc(sizeof(AudioStream));
    if (cStream) {
        cStream->id = stream->id;
        cStream->url = strdup(stream->url.c_str());
        cStream->status = stream->status;
        cStream->volume = stream->volume;
        cStream->playing = stream->playing;
        cStream->userData = stream.get(); // Store C++ pointer
    }
    return cStream;
}

AudioStream* audio_stream_create_from_file(const char* filePath) {
    auto stream = BMS::Audio::CreateStreamFromFile(filePath);
    if (!stream) return nullptr;

    AudioStream* cStream = (AudioStream*)malloc(sizeof(AudioStream));
    if (cStream) {
        cStream->id = stream->id;
        cStream->filePath = strdup(filePath);
        cStream->status = stream->status;
        cStream->volume = stream->volume;
        cStream->playing = stream->playing;
        cStream->userData = stream.get();
    }
    return cStream;
}

void audio_stream_destroy(AudioStream* stream) {
    if (!stream) return;
    free(stream->url);
    free(stream->filePath);
    free(stream);
}

int audio_stream_play(AudioStream* stream) {
    if (!stream) return -1;
    return BMS::Audio::PlayStream(stream->id) ? 0 : -1;
}

int audio_stream_pause(AudioStream* stream) {
    if (!stream) return -1;
    return BMS::Audio::PauseStream(stream->id) ? 0 : -1;
}

int audio_stream_stop(AudioStream* stream) {
    if (!stream) return -1;
    return BMS::Audio::StopStream(stream->id) ? 0 : -1;
}

void audio_stream_set_volume(AudioStream* stream, float volume) {
    if (!stream) return;
    BMS::Audio::SetStreamVolume(stream->id, volume);
}

float audio_stream_get_volume(AudioStream* stream) {
    if (!stream) return 0.0f;
    return BMS::Audio::GetStreamVolume(stream->id);
}

// Volume control wrappers
void audio_vol_set_master(float volume) {
    BMS::Audio::SetMasterVolume(volume);
}

float audio_vol_get_master(void) {
    return BMS::Audio::GetMasterVolume();
}

void audio_vol_set_mute_all(bool mute) {
    BMS::Audio::SetMuteAll(mute);
}

bool audio_vol_get_mute_all(void) {
    return BMS::Audio::GetMuteAll();
}

} // extern "C"

} // namespace Audio
} // namespace BMS