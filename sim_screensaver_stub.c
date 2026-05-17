/**
 * sim_screensaver_stub.c — replaces ui_screensaver.c in the simulator build.
 *
 * The real ui_screensaver_init() calls lv_draw_buf_init with an ARGB8888
 * format but a buffer sized for RGB565 — this asserts on desktop LVGL.
 * For the simulator we just provide no-op stubs so the rest of the UI works.
 */

#include "ui_screensaver.h"

void ui_screensaver_init(lv_obj_t *parent)  { (void)parent; }
void ui_screensaver_show(const char *name, bool playing) { (void)name; (void)playing; }
void ui_screensaver_hide(void)              {}
bool ui_screensaver_is_visible(void)        { return false; }
void ui_screensaver_reset_idle(void)        {}
void ui_screensaver_tick(void)              {}
