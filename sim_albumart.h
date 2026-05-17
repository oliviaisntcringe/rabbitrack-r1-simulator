#pragma once
/**
 * sim_albumart.h — extract embedded cover art from MP3 ID3v2 APIC frame
 */
#include <stdbool.h>
#include <stdint.h>

/**
 * Load album art from an MP3 file's ID3v2 APIC tag.
 * On success returns true and sets *rgba_out to a malloc'd RGBA buffer
 * (w_out * h_out * 4 bytes). Caller must free(*rgba_out).
 */
bool sim_albumart_load(const char *mp3_path,
                       uint8_t   **rgba_out,
                       int        *w_out,
                       int        *h_out);
