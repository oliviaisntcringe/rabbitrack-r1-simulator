#pragma once
/**
 * sim_audio.h — thin wrapper around miniaudio for MP3 playback in simulator
 */
#include <stdbool.h>

void  sim_audio_init(void);
void  sim_audio_play(const char *path);   /* stop current, start new file */
void  sim_audio_pause(bool paused);
void  sim_audio_stop(void);
void  sim_audio_cleanup(void);
float sim_audio_get_length_sec(void);    /* actual track duration, 0 if unknown */
