// audioVol.c - BMS Audio Volume Control Implementation
#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// Volume Control Context
// ============================================================================

typedef struct AudioVolContext {
    float masterVolume;
    float appVolume;
    float systemVolume;
    float commVolume;
    bool muteAll;
    bool muteApp;
    bool muteSystem;
    bool muteComm;
    uint32_t deviceCount;
    float* deviceVolumes;
    char** deviceNames;
    void* userData;
} AudioVolContext;

static AudioVolContext* g_volContext = NULL;

// ============================================================================
// Internal Functions
// ============================================================================

static AudioVolContext* audio_vol_get_context(void) {
    if (!g_volContext) {
        g_volContext = (AudioVolContext*)calloc(1, sizeof(AudioVolContext));
        if (g_volContext) {
            g_volContext->masterVolume = 1.0f;
            g_volContext->appVolume = 1.0f;
            g_volContext->systemVolume = 1.0f;
            g_volContext->commVolume = 1.0f;
            g_volContext->deviceCount = 0;
        }
    }
    return g_volContext;
}

static void audio_vol_cleanup(void) {
    if (g_volContext) {
        if (g_volContext->deviceVolumes) {
            free(g_volContext->deviceVolumes);
        }
        if (g_volContext->deviceNames) {
            for (uint32_t i = 0; i < g_volContext->deviceCount; i++) {
                free(g_volContext->deviceNames[i]);
            }
            free(g_volContext->deviceNames);
        }
        free(g_volContext);
        g_volContext = NULL;
    }
}

// ============================================================================
// Public Volume Control Functions
// ============================================================================

int audio_vol_init(void) {
    if (g_volContext) {
        return 0;
    }

    g_volContext = audio_vol_get_context();
    if (!g_volContext) {
        return -1;
    }

    printf("[BMS AudioVol] Volume control initialized\n");
    return 0;
}

void audio_vol_shutdown(void) {
    audio_vol_cleanup();
    printf("[BMS AudioVol] Volume control shut down\n");
}

float audio_vol_get_master(void) {
    AudioVolContext* ctx = audio_vol_get_context();
    return ctx ? ctx->masterVolume : 1.0f;
}

void audio_vol_set_master(float volume) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx) return;

    ctx->masterVolume = volume > 0.0f ? (volume < 1.0f ? volume : 1.0f) : 0.0f;
    printf("[BMS AudioVol] Master volume set to %.3f\n", ctx->masterVolume);
}

float audio_vol_get_application(void) {
    AudioVolContext* ctx = audio_vol_get_context();
    return ctx ? ctx->appVolume : 1.0f;
}

void audio_vol_set_application(float volume) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx) return;

    ctx->appVolume = volume > 0.0f ? (volume < 1.0f ? volume : 1.0f) : 0.0f;
    printf("[BMS AudioVol] Application volume set to %.3f\n", ctx->appVolume);
}

float audio_vol_get_system(void) {
    AudioVolContext* ctx = audio_vol_get_context();
    return ctx ? ctx->systemVolume : 1.0f;
}

void audio_vol_set_system(float volume) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx) return;

    ctx->systemVolume = volume > 0.0f ? (volume < 1.0f ? volume : 1.0f) : 0.0f;
    printf("[BMS AudioVol] System volume set to %.3f\n", ctx->systemVolume);
}

float audio_vol_get_communication(void) {
    AudioVolContext* ctx = audio_vol_get_context();
    return ctx ? ctx->commVolume : 1.0f;
}

void audio_vol_set_communication(float volume) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx) return;

    ctx->commVolume = volume > 0.0f ? (volume < 1.0f ? volume : 1.0f) : 0.0f;
    printf("[BMS AudioVol] Communication volume set to %.3f\n", ctx->commVolume);
}

bool audio_vol_get_mute_all(void) {
    AudioVolContext* ctx = audio_vol_get_context();
    return ctx ? ctx->muteAll : false;
}

void audio_vol_set_mute_all(bool mute) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx) return;

    ctx->muteAll = mute;
    printf("[BMS AudioVol] Mute all: %s\n", mute ? "ON" : "OFF");
}

bool audio_vol_get_mute_application(void) {
    AudioVolContext* ctx = audio_vol_get_context();
    return ctx ? ctx->muteApp : false;
}

void audio_vol_set_mute_application(bool mute) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx) return;

    ctx->muteApp = mute;
    printf("[BMS AudioVol] Mute application: %s\n", mute ? "ON" : "OFF");
}

bool audio_vol_get_mute_system(void) {
    AudioVolContext* ctx = audio_vol_get_context();
    return ctx ? ctx->muteSystem : false;
}

void audio_vol_set_mute_system(bool mute) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx) return;

    ctx->muteSystem = mute;
    printf("[BMS AudioVol] Mute system: %s\n", mute ? "ON" : "OFF");
}

bool audio_vol_get_mute_communication(void) {
    AudioVolContext* ctx = audio_vol_get_context();
    return ctx ? ctx->muteComm : false;
}

void audio_vol_set_mute_communication(bool mute) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx) return;

    ctx->muteComm = mute;
    printf("[BMS AudioVol] Mute communication: %s\n", mute ? "ON" : "OFF");
}

// ============================================================================
// Device Volume Functions
// ============================================================================

uint32_t audio_vol_get_device_count(void) {
    AudioVolContext* ctx = audio_vol_get_context();
    return ctx ? ctx->deviceCount : 0;
}

const char* audio_vol_get_device_name(uint32_t deviceIndex) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx || deviceIndex >= ctx->deviceCount) return NULL;
    return ctx->deviceNames[deviceIndex];
}

float audio_vol_get_device_volume(uint32_t deviceIndex) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx || deviceIndex >= ctx->deviceCount) return 1.0f;
    return ctx->deviceVolumes[deviceIndex];
}

void audio_vol_set_device_volume(uint32_t deviceIndex, float volume) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx || deviceIndex >= ctx->deviceCount) return;

    ctx->deviceVolumes[deviceIndex] = volume > 0.0f ? (volume < 1.0f ? volume : 1.0f) : 0.0f;
    printf("[BMS AudioVol] Device %u volume set to %.3f\n", deviceIndex, ctx->deviceVolumes[deviceIndex]);
}

int audio_vol_add_device(const char* name) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx) return -1;

    uint32_t index = ctx->deviceCount;

    // Reallocate device volumes
    ctx->deviceVolumes = (float*)realloc(ctx->deviceVolumes,
                                         (ctx->deviceCount + 1) * sizeof(float));
    if (!ctx->deviceVolumes) return -1;

    // Reallocate device names
    ctx->deviceNames = (char**)realloc(ctx->deviceNames,
                                       (ctx->deviceCount + 1) * sizeof(char*));
    if (!ctx->deviceNames) {
        // Rollback
        ctx->deviceCount--;
        return -1;
    }

    ctx->deviceNames[index] = audio_strdup(name);
    ctx->deviceVolumes[index] = 1.0f;
    ctx->deviceCount++;

    printf("[BMS AudioVol] Added device: %s (index %u)\n", name, index);
    return index;
}

void audio_vol_remove_device(uint32_t deviceIndex) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx || deviceIndex >= ctx->deviceCount) return;

    free(ctx->deviceNames[deviceIndex]);

    // Shift devices
    for (uint32_t i = deviceIndex; i < ctx->deviceCount - 1; i++) {
        ctx->deviceNames[i] = ctx->deviceNames[i + 1];
        ctx->deviceVolumes[i] = ctx->deviceVolumes[i + 1];
    }

    ctx->deviceCount--;

    printf("[BMS AudioVol] Removed device at index %u\n", deviceIndex);
}

// ============================================================================
// Volume Calculation Functions
// ============================================================================

float audio_vol_calculate_final_volume(void) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx) return 1.0f;

    float volume = 1.0f;

    // Apply master volume
    volume *= ctx->masterVolume;

    // Apply application volume if not muted
    if (!ctx->muteApp) {
        volume *= ctx->appVolume;
    }

    // Apply system volume if not muted
    if (!ctx->muteSystem) {
        volume *= ctx->systemVolume;
    }

    // Apply communication volume if not muted
    if (!ctx->muteComm) {
        volume *= ctx->commVolume;
    }

    // Mute all overrides everything
    if (ctx->muteAll) {
        volume = 0.0f;
    }

    return volume;
}

float audio_vol_calculate_device_volume(uint32_t deviceIndex) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx || deviceIndex >= ctx->deviceCount) return 1.0f;

    float baseVolume = audio_vol_calculate_final_volume();
    return baseVolume * ctx->deviceVolumes[deviceIndex];
}

float audio_vol_linear_to_db(float linear) {
    if (linear <= 0.0f) return -120.0f;
    return 20.0f * log10f(linear);
}

float audio_vol_db_to_linear(float db) {
    if (db <= -120.0f) return 0.0f;
    return powf(10.0f, db / 20.0f);
}

float audio_vol_linear_to_percent(float linear) {
    return linear * 100.0f;
}

float audio_vol_percent_to_linear(float percent) {
    return percent / 100.0f;
}

float audio_vol_smooth_ramp(float current, float target, float speed) {
    float diff = target - current;
    float step = diff * speed;

    if (fabsf(step) < 0.0001f) {
        return target;
    }

    return current + step;
}

// ============================================================================
// Preset Management
// ============================================================================

typedef struct AudioPreset {
    char* name;
    float masterVolume;
    float appVolume;
    float systemVolume;
    float commVolume;
    bool muteAll;
    bool muteApp;
    bool muteSystem;
    bool muteComm;
    float* deviceVolumes;
    uint32_t deviceCount;
} AudioPreset;

static AudioPreset** g_presets = NULL;
static uint32_t g_presetCount = 0;

int audio_vol_preset_create(const char* name) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx) return -1;

    AudioPreset* preset = (AudioPreset*)calloc(1, sizeof(AudioPreset));
    if (!preset) return -1;

    preset->name = audio_strdup(name);
    preset->masterVolume = ctx->masterVolume;
    preset->appVolume = ctx->appVolume;
    preset->systemVolume = ctx->systemVolume;
    preset->commVolume = ctx->commVolume;
    preset->muteAll = ctx->muteAll;
    preset->muteApp = ctx->muteApp;
    preset->muteSystem = ctx->muteSystem;
    preset->muteComm = ctx->muteComm;
    preset->deviceCount = ctx->deviceCount;

    if (ctx->deviceCount > 0) {
        preset->deviceVolumes = (float*)malloc(ctx->deviceCount * sizeof(float));
        if (!preset->deviceVolumes) {
            free(preset->name);
            free(preset);
            return -1;
        }
        memcpy(preset->deviceVolumes, ctx->deviceVolumes, ctx->deviceCount * sizeof(float));
    }

    // Add to preset list
    g_presets = (AudioPreset**)realloc(g_presets, (g_presetCount + 1) * sizeof(AudioPreset*));
    if (!g_presets) {
        free(preset->deviceVolumes);
        free(preset->name);
        free(preset);
        return -1;
    }

    g_presets[g_presetCount++] = preset;
    printf("[BMS AudioVol] Preset created: %s\n", name);

    return g_presetCount - 1;
}

void audio_vol_preset_apply(uint32_t presetIndex) {
    if (presetIndex >= g_presetCount) return;

    AudioPreset* preset = g_presets[presetIndex];
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx) return;

    ctx->masterVolume = preset->masterVolume;
    ctx->appVolume = preset->appVolume;
    ctx->systemVolume = preset->systemVolume;
    ctx->commVolume = preset->commVolume;
    ctx->muteAll = preset->muteAll;
    ctx->muteApp = preset->muteApp;
    ctx->muteSystem = preset->muteSystem;
    ctx->muteComm = preset->muteComm;

    if (preset->deviceCount <= ctx->deviceCount) {
        for (uint32_t i = 0; i < preset->deviceCount; i++) {
            ctx->deviceVolumes[i] = preset->deviceVolumes[i];
        }
    }

    printf("[BMS AudioVol] Preset applied: %s\n", preset->name);
}

void audio_vol_preset_delete(uint32_t presetIndex) {
    if (presetIndex >= g_presetCount) return;

    AudioPreset* preset = g_presets[presetIndex];
    free(preset->name);
    if (preset->deviceVolumes) {
        free(preset->deviceVolumes);
    }
    free(preset);

    for (uint32_t i = presetIndex; i < g_presetCount - 1; i++) {
        g_presets[i] = g_presets[i + 1];
    }

    g_presetCount--;
    printf("[BMS AudioVol] Preset deleted\n");
}

uint32_t audio_vol_preset_get_count(void) {
    return g_presetCount;
}

const char* audio_vol_preset_get_name(uint32_t presetIndex) {
    if (presetIndex >= g_presetCount) return NULL;
    return g_presets[presetIndex]->name;
}

// ============================================================================
// Save/Load Functions
// ============================================================================

int audio_vol_save(const char* path) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx) return -1;

    FILE* file = fopen(path, "w");
    if (!file) return -1;

    fprintf(file, "# BMS Audio Volume Control Configuration\n");
    fprintf(file, "# Generated by BMS Browser Audio System\n\n");

    fprintf(file, "[General]\n");
    fprintf(file, "master_volume=%.6f\n", ctx->masterVolume);
    fprintf(file, "application_volume=%.6f\n", ctx->appVolume);
    fprintf(file, "system_volume=%.6f\n", ctx->systemVolume);
    fprintf(file, "communication_volume=%.6f\n", ctx->commVolume);
    fprintf(file, "mute_all=%d\n", ctx->muteAll ? 1 : 0);
    fprintf(file, "mute_application=%d\n", ctx->muteApp ? 1 : 0);
    fprintf(file, "mute_system=%d\n", ctx->muteSystem ? 1 : 0);
    fprintf(file, "mute_communication=%d\n", ctx->muteComm ? 1 : 0);
    fprintf(file, "device_count=%u\n\n", ctx->deviceCount);

    fprintf(file, "[Devices]\n");
    for (uint32_t i = 0; i < ctx->deviceCount; i++) {
        fprintf(file, "device_%u_name=%s\n", i, ctx->deviceNames[i] ? ctx->deviceNames[i] : "");
        fprintf(file, "device_%u_volume=%.6f\n", i, ctx->deviceVolumes[i]);
    }

    fclose(file);
    printf("[BMS AudioVol] Configuration saved to: %s\n", path);

    return 0;
}

int audio_vol_load(const char* path) {
    AudioVolContext* ctx = audio_vol_get_context();
    if (!ctx) return -1;

    FILE* file = fopen(path, "r");
    if (!file) return -1;

    char line[512];
    char key[128];
    char value[384];

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        // Skip section headers
        if (line[0] == '[') {
            continue;
        }

        if (sscanf(line, "%127[^=]=%383[^\n]", key, value) == 2) {
            // Remove trailing whitespace
            char* end = value + strlen(value) - 1;
            while (end > value && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
                *end-- = '\0';
            }

            if (strcmp(key, "master_volume") == 0) {
                ctx->masterVolume = atof(value);
            } else if (strcmp(key, "application_volume") == 0) {
                ctx->appVolume = atof(value);
            } else if (strcmp(key, "system_volume") == 0) {
                ctx->systemVolume = atof(value);
            } else if (strcmp(key, "communication_volume") == 0) {
                ctx->commVolume = atof(value);
            } else if (strcmp(key, "mute_all") == 0) {
                ctx->muteAll = atoi(value) != 0;
            } else if (strcmp(key, "mute_application") == 0) {
                ctx->muteApp = atoi(value) != 0;
            } else if (strcmp(key, "mute_system") == 0) {
                ctx->muteSystem = atoi(value) != 0;
            } else if (strcmp(key, "mute_communication") == 0) {
                ctx->muteComm = atoi(value) != 0;
            }
        }
    }

    fclose(file);
    printf("[BMS AudioVol] Configuration loaded from: %s\n", path);

    return 0;
}

// ============================================================================
// Cleanup
// ============================================================================

void audio_vol_cleanup_presets(void) {
    for (uint32_t i = 0; i < g_presetCount; i++) {
        AudioPreset* preset = g_presets[i];
        free(preset->name);
        if (preset->deviceVolumes) {
            free(preset->deviceVolumes);
        }
        free(preset);
    }
    free(g_presets);
    g_presets = NULL;
    g_presetCount = 0;
}

// ============================================================================
// BMS Browser Integration
// ============================================================================

int audio_vol_bms_init(void) {
    audio_vol_init();

    // Create default presets
    audio_vol_preset_create("Default");
    audio_vol_preset_create("Headphones");
    audio_vol_preset_create("Speakers");
    audio_vol_preset_create("Quiet");
    audio_vol_preset_create("Loud");

    // Add default devices
    audio_vol_add_device("System Default");
    audio_vol_add_device("Speakers");
    audio_vol_add_device("Headphones");

    return 0;
}

void audio_vol_bms_shutdown(void) {
    audio_vol_cleanup_presets();
    audio_vol_shutdown();
}