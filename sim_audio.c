/**
 * sim_audio.c — MP3/WAV playback via miniaudio (CoreAudio backend on macOS)
 */

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "sim_audio.h"
#include <stdio.h>
#include <string.h>

static ma_engine g_engine;
static ma_sound  g_sound;
static bool      g_engine_ok    = false;
static bool      g_sound_valid  = false;

void sim_audio_init(void)
{
    ma_result r = ma_engine_init(NULL, &g_engine);
    if (r != MA_SUCCESS) {
        fprintf(stderr, "[audio] engine init failed: %d\n", r);
        return;
    }
    g_engine_ok = true;
    printf("[audio] miniaudio engine ready\n");
}

void sim_audio_play(const char *path)
{
    if (!g_engine_ok) return;

    /* Uninit previous sound */
    if (g_sound_valid) {
        ma_sound_stop(&g_sound);
        ma_sound_uninit(&g_sound);
        g_sound_valid = false;
    }

    /* No STREAM flag: load fully into memory so get_length works reliably */
    ma_result r = ma_sound_init_from_file(&g_engine, path, 0,
                                          NULL, NULL, &g_sound);
    if (r != MA_SUCCESS) {
        fprintf(stderr, "[audio] cannot open: %s  (err %d)\n", path, r);
        return;
    }
    g_sound_valid = true;
    ma_sound_start(&g_sound);
    printf("[audio] playing: %s\n", path);
}

void sim_audio_pause(bool paused)
{
    if (!g_engine_ok || !g_sound_valid) return;
    if (paused) ma_sound_stop(&g_sound);
    else        ma_sound_start(&g_sound);
}

void sim_audio_stop(void)
{
    if (!g_engine_ok || !g_sound_valid) return;
    ma_sound_stop(&g_sound);
    ma_sound_uninit(&g_sound);
    g_sound_valid = false;
}

float sim_audio_get_length_sec(void)
{
    if (!g_engine_ok || !g_sound_valid) return 0.0f;
    float len = 0.0f;
    ma_sound_get_length_in_seconds(&g_sound, &len);
    return len;
}

void sim_audio_cleanup(void)
{
    sim_audio_stop();
    if (g_engine_ok) {
        ma_engine_uninit(&g_engine);
        g_engine_ok = false;
    }
}
