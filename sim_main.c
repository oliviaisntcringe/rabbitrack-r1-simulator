/**
 * sim_main.c — macOS SDL2 simulator for MP3 player UI
 *
 * Scans ~/Documents/Music/ for .mp3 / .wav files.
 * Plays audio via miniaudio (CoreAudio).
 * Loads album art from ID3v2 APIC tags.
 *
 * Keys: TAB=toggle screens  P=play/pause  N=next  B=prev  S=screensaver  Q/Esc=quit
 */

#define LV_CONF_INCLUDE_SIMPLE
#define LV_LVGL_H_INCLUDE_SIMPLE

#include "lvgl.h"
#include "src/drivers/sdl/lv_sdl_window.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "src/drivers/sdl/lv_sdl_keyboard.h"

#include "ui_player.h"
#include "ui_library.h"
#include "ui_screensaver.h"
#include "ui_theme.h"

#include "sim_audio.h"
#include "sim_albumart.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>

#define SCREEN_W      480
#define SCREEN_H      320
#define MAX_TRACKS    256
#define NAME_MAX_LEN  128
#define MUSIC_DIR     "Documents/Music"

/* ── Companion-app IPC sentinel ─────────────────────────────────────────── */
/* The companion app polls this file to know the simulator is running.       */
/* Content: the absolute path to the music directory (one line, no newline). */
#define SIM_SENTINEL_PATH "/tmp/mp3player_sim"

static char g_music_dir[512] = {0};   /* absolute path, set in main() */

static void sim_sentinel_write(void)
{
    FILE *f = fopen(SIM_SENTINEL_PATH, "w");
    if (!f) { perror("sentinel write"); return; }
    fputs(g_music_dir, f);
    fclose(f);
    printf("[sim] sentinel written: %s\n", SIM_SENTINEL_PATH);
}

static void sim_sentinel_remove(void)
{
    remove(SIM_SENTINEL_PATH);
    printf("[sim] sentinel removed\n");
}

/* Called by atexit so the sentinel is cleaned up even on abnormal exit */
static void sim_atexit(void) { sim_sentinel_remove(); }

/* ── Track list ────────────────────────────────────────────────────────── */
typedef struct {
    char name[NAME_MAX_LEN];
    char path[512];
} track_t;

static track_t       g_tracks[MAX_TRACKS];
static const char   *g_name_ptrs[MAX_TRACKS]; /* for ui_library_update */
static int           g_track_count = 0;

static bool ends_ci(const char *s, const char *suf)
{
    size_t sl = strlen(s), xl = strlen(suf);
    if (sl < xl) return false;
    for (size_t i = 0; i < xl; i++) {
        char a = s[sl - xl + i], b = suf[i];
        if (a >= 'A' && a <= 'Z') a += 32;
        if (b >= 'A' && b <= 'Z') b += 32;
        if (a != b) return false;
    }
    return true;
}

static void strip_ext(const char *src, char *dst, size_t max)
{
    strncpy(dst, src, max - 1);
    dst[max - 1] = '\0';
    char *dot = strrchr(dst, '.');
    if (dot) *dot = '\0';
}

static int track_cmp(const void *a, const void *b)
{
    return strcmp(((const track_t *)a)->name, ((const track_t *)b)->name);
}

static int scan_music(void)
{
    const char *home = getenv("HOME");
    if (!home) home = getpwuid(getuid())->pw_dir;

    char dir[512];
    snprintf(dir, sizeof(dir), "%s/%s", home, MUSIC_DIR);

    DIR *d = opendir(dir);
    if (!d) { fprintf(stderr, "Cannot open %s\n", dir); return 0; }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && g_track_count < MAX_TRACKS) {
        if (ent->d_name[0] == '.') continue;
        if (!ends_ci(ent->d_name, ".mp3") &&
            !ends_ci(ent->d_name, ".wav")) continue;

        snprintf(g_tracks[g_track_count].path,
                 sizeof(g_tracks[0].path),
                 "%s/%s", dir, ent->d_name);
        strip_ext(ent->d_name,
                  g_tracks[g_track_count].name,
                  NAME_MAX_LEN);
        g_track_count++;
    }
    closedir(d);

    if (g_track_count > 1)
        qsort(g_tracks, g_track_count, sizeof(track_t), track_cmp);

    for (int i = 0; i < g_track_count; i++)
        g_name_ptrs[i] = g_tracks[i].name;

    printf("Found %d tracks in %s\n", g_track_count, dir);
    return g_track_count;
}

/* ── Playback state ────────────────────────────────────────────────────── */
static int      g_track_idx   = 0;
static int      g_scroll_off  = 0;
static bool     g_playing     = true;
static uint32_t g_pos_sec     = 0;
static bool     g_show_player = true;
static bool     g_screensaver = false;
static uint32_t g_last_tick   = 0;
static uint32_t g_idle_ms     = 0;   /* ms since last key press */

#define SCREENSAVER_TIMEOUT_MS 20000  /* 20 s idle → screensaver */

/* Actual duration of current track fetched from miniaudio after loading */
static uint32_t g_track_dur_s = 225;

static lv_obj_t *g_player_screen  = NULL;
static lv_obj_t *g_library_screen = NULL;

/* ── Load album art for current track ─────────────────────────────────── */
static void load_album_art(void)
{
    if (g_track_count == 0) { printf("[sim] load_album_art: no tracks\n"); return; }
    printf("[sim] load_album_art: idx=%d path=%s\n",
           g_track_idx, g_tracks[g_track_idx].path);
    uint8_t *rgba = NULL;
    int w = 0, h = 0;
    bool ok = sim_albumart_load(g_tracks[g_track_idx].path, &rgba, &w, &h);
    printf("[sim] sim_albumart_load returned %s\n", ok ? "OK" : "FAIL");
    if (ok) {
        ui_player_set_album_art(rgba, w, h);
        ui_screensaver_set_art(rgba, w, h);
        free(rgba);
    } else {
        ui_player_set_album_art(NULL, 0, 0);
        ui_screensaver_set_art(NULL, 0, 0);
    }
}

/* ── Clock / refresh helpers ───────────────────────────────────────────── */
static void update_clock_label(void)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    ui_player_update_clock(buf);
}

static void refresh_player(void)
{
    if (g_track_count == 0) return;
    ui_player_update_track(g_name_ptrs[g_track_idx],
                           g_track_idx + 1, g_track_count);
    ui_player_update_progress(g_pos_sec, g_track_dur_s);
    ui_player_update_state(g_playing);
    ui_player_update_volume(72);
    update_clock_label();
}

static void refresh_library(void)
{
    if (g_track_count == 0) return;
    ui_library_update(g_name_ptrs, g_track_count, g_track_idx, g_scroll_off);
}

/* ── Rescan music folder (R key / called after companion uploads) ─────── */
static void rescan_tracks(void)
{
    bool was_playing = g_playing;
    if (was_playing) { sim_audio_pause(true); g_playing = false; }

    int old_count = g_track_count;
    g_track_count = 0;
    scan_music();

    printf("[sim] rescan: %d tracks (was %d)\n", g_track_count, old_count);
    ui_player_add_log("> Rescan complete");

    if (g_track_count == 0) {
        ui_player_update_track("No tracks", 0, 0);
        return;
    }

    /* Keep current index in bounds */
    if (g_track_idx >= g_track_count) g_track_idx = 0;

    refresh_player();
    refresh_library();
    load_album_art();

    if (was_playing) {
        g_playing = true;
        sim_audio_play(g_tracks[g_track_idx].path);
        float len = sim_audio_get_length_sec();
        g_track_dur_s = (len > 1.0f) ? (uint32_t)len : 225;
    }
}

/* ── Start playing the current track ──────────────────────────────────── */
static void play_current(void)
{
    if (g_track_count == 0) return;
    printf("\n[sim] ==== play_current: idx=%d name=%s ====\n",
           g_track_idx, g_name_ptrs[g_track_idx]);
    sim_audio_play(g_tracks[g_track_idx].path);

    /* Fetch real duration (available after sound is loaded) */
    float len = sim_audio_get_length_sec();
    g_track_dur_s = (len > 1.0f) ? (uint32_t)len : 225;
    printf("[sim] duration: %.1f s -> g_track_dur_s=%u\n", len, g_track_dur_s);

    load_album_art();
    refresh_player();
    refresh_library();
    char buf[80];
    snprintf(buf, sizeof(buf), "> >> %s", g_name_ptrs[g_track_idx]);
    ui_player_add_log(buf);
}

/* ── main ──────────────────────────────────────────────────────────────── */
int main(void)
{
    lv_init();

    lv_display_t *disp = lv_sdl_window_create(SCREEN_W, SCREEN_H);
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);
    lv_sdl_mouse_create();
    lv_indev_t *kb = lv_sdl_keyboard_create();
    (void)kb;

    ui_theme_init();

    g_player_screen  = lv_obj_create(NULL);
    g_library_screen = lv_obj_create(NULL);

    lv_screen_load(g_player_screen);
    lv_timer_handler();
    ui_player_create(g_player_screen);

    lv_screen_load(g_library_screen);
    lv_timer_handler();
    ui_library_create(g_library_screen);

    lv_screen_load(g_player_screen);
    ui_screensaver_init(lv_layer_top());

    /* Build absolute music dir path and write sentinel for companion app */
    {
        const char *home = getenv("HOME");
        if (!home) home = getpwuid(getuid())->pw_dir;
        snprintf(g_music_dir, sizeof(g_music_dir), "%s/%s", home, MUSIC_DIR);
    }
    atexit(sim_atexit);
    sim_sentinel_write();

    /* Scan music */
    scan_music();
    if (g_track_count == 0) {
        printf("No tracks in ~/%s — add .mp3 or .wav\n", MUSIC_DIR);
    }

    /* Init audio */
    sim_audio_init();

    /* Initial UI population */
    refresh_player();
    refresh_library();
    load_album_art();
    if (g_playing && g_track_count > 0) {
        printf("[sim] startup: starting audio for idx=%d\n", g_track_idx);
        sim_audio_play(g_tracks[g_track_idx].path);
        float len = sim_audio_get_length_sec();
        g_track_dur_s = (len > 1.0f) ? (uint32_t)len : 225;
        printf("[sim] startup: duration %.1f s -> g_track_dur_s=%u\n", len, g_track_dur_s);
    }

    ui_player_add_log("> Simulator started");
    ui_player_add_log("> TAB: screens  P: play/pause");

    printf("=== MP3 Player Simulator ===\n");
    printf("Loaded %d tracks from ~/%s\n", g_track_count, MUSIC_DIR);
    printf("TAB=switch  P=play/pause  N=next  B=prev  S=screensaver  Q=quit\n");

    g_last_tick = SDL_GetTicks();
    uint32_t waveform_timer = 0;

    while (1) {
        uint32_t now     = SDL_GetTicks();
        uint32_t elapsed = now - g_last_tick;
        g_last_tick = now;

        lv_tick_inc(elapsed);
        lv_timer_handler();

        /* Advance progress & detect track end by simulation clock */
        if (g_playing && !g_screensaver) {
            static uint32_t frac = 0;
            frac += elapsed;
            if (frac >= 1000) {
                frac -= 1000;
                g_pos_sec++;
                if (g_track_count > 0 && g_pos_sec >= g_track_dur_s) {
                    g_track_idx = (g_track_idx + 1) % g_track_count;
                    g_pos_sec   = 0;
                    play_current(); /* also resets g_track_dur_s */
                } else {
                    refresh_player();
                }
            }
        }

        /* Waveform tick every 100 ms */
        waveform_timer += elapsed;
        if (waveform_timer >= 100) {
            waveform_timer = 0;
            if (!g_screensaver) ui_player_waveform_tick(g_playing);
        }

        /* Idle screensaver timer */
        if (!g_screensaver) {
            g_idle_ms += elapsed;
            if (g_idle_ms >= SCREENSAVER_TIMEOUT_MS && g_track_count > 0) {
                ui_screensaver_show(g_name_ptrs[g_track_idx], g_playing);
                g_screensaver = true;
                lv_screen_load(g_player_screen);
                g_show_player = true;
            }
        }

        /* SDL events */
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) goto quit;
            if (e.type == SDL_KEYDOWN) {
                SDL_Keycode key = e.key.keysym.sym;

                if (key == SDLK_q || key == SDLK_ESCAPE) goto quit;

                /* Any key resets idle timer */
                g_idle_ms = 0;

                /* Any key dismisses screensaver */
                if (g_screensaver && key != SDLK_s) {
                    ui_screensaver_hide();
                    g_screensaver = false;
                }

                if (key == SDLK_TAB) {
                    if (g_screensaver) { ui_screensaver_hide(); g_screensaver = false; }
                    g_show_player = !g_show_player;
                    lv_screen_load(g_show_player ? g_player_screen
                                                 : g_library_screen);
                }

                if (key == SDLK_p && g_track_count > 0) {
                    g_playing = !g_playing;
                    sim_audio_pause(!g_playing);
                    ui_player_update_state(g_playing);
                    if (g_screensaver)
                        ui_screensaver_show(g_name_ptrs[g_track_idx], g_playing);
                    char buf[64];
                    snprintf(buf, sizeof(buf), "> %s",
                             g_playing ? "Playing" : "Paused");
                    ui_player_add_log(buf);
                }

                if (key == SDLK_n && g_track_count > 0) {
                    g_track_idx = (g_track_idx + 1) % g_track_count;
                    g_pos_sec   = 0;
                    play_current();
                    if (g_screensaver)
                        ui_screensaver_show(g_name_ptrs[g_track_idx], g_playing);
                }

                if (key == SDLK_b && g_track_count > 0) {
                    g_track_idx = (g_track_idx - 1 + g_track_count) % g_track_count;
                    g_pos_sec   = 0;
                    play_current();
                    if (g_screensaver)
                        ui_screensaver_show(g_name_ptrs[g_track_idx], g_playing);
                }

                if (key == SDLK_s) {
                    if (!g_screensaver) {
                        if (g_track_count > 0)
                            ui_screensaver_show(g_name_ptrs[g_track_idx], g_playing);
                        g_screensaver = true;
                        lv_screen_load(g_player_screen);
                        g_show_player = true;
                    } else {
                        ui_screensaver_hide();
                        g_screensaver = false;
                    }
                }

                /* R — rescan music folder (picks up files added by companion) */
                if (key == SDLK_r) {
                    rescan_tracks();
                    if (g_screensaver && g_track_count > 0)
                        ui_screensaver_show(g_name_ptrs[g_track_idx], g_playing);
                }
            }
        }

        SDL_Delay(5);
    }

quit:
    sim_sentinel_remove(); /* belt-and-suspenders; atexit also calls this */
    sim_audio_cleanup();
    printf("Bye.\n");
    return 0;
}
