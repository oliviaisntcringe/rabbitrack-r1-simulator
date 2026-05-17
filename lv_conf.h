/**
 * lv_conf.h — LVGL v9 config for macOS SDL2 simulator
 */

#if 1

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0   /* SDL2 is little-endian, no swap needed */

/* Memory — LVGL v9 uses LV_USE_STDLIB_MALLOC, not LV_MEM_CUSTOM */
#define LV_USE_STDLIB_MALLOC    1   /* 1 = LV_STDLIB_CLIB = system malloc/free */
#define LV_USE_STDLIB_STRING    1
#define LV_USE_STDLIB_SPRINTF   1

/* Tick via SDL_GetTicks */
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE <SDL2/SDL.h>
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR SDL_GetTicks()
#endif

/* Log */
#define LV_USE_LOG 0
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

/* Asserts */
#define LV_USE_ASSERT_NULL    1
#define LV_USE_ASSERT_MALLOC  1
#define LV_USE_ASSERT_STYLE   0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ     0

/* Draw */
#define LV_DRAW_BUF_STRIDE_ALIGN    1
#define LV_DRAW_BUF_ALIGN           4
#define LV_USE_DRAW_SW              1
#define LV_DRAW_SW_COMPLEX          1
#define LV_USE_DRAW_SW_ASM_NEON     0

/* Display */
#define LV_DISPLAY_DEF_REFR_PERIOD  16
#define LV_DPI_DEF                  130

/* SDL driver */
#define LV_USE_SDL 1

/* Widgets */
#define LV_USE_ANIMIMG     1
#define LV_USE_ARC         1
#define LV_USE_BAR         1
#define LV_USE_BUTTON      1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CALENDAR    0
#define LV_USE_CANVAS      1
#define LV_USE_CHART       0
#define LV_USE_CHECKBOX    0
#define LV_USE_DROPDOWN    0
#define LV_USE_IMAGE       1
#define LV_USE_IMAGEBUTTON 0
#define LV_USE_KEYBOARD    0
#define LV_USE_LABEL       1
#define LV_USE_LED         0
#define LV_USE_LINE        1
#define LV_USE_LIST        1
#define LV_USE_MENU        0
#define LV_USE_MSGBOX      0
#define LV_USE_ROLLER      0
#define LV_USE_SCALE       0
#define LV_USE_SLIDER      1
#define LV_USE_SPAN        0
#define LV_USE_SPINBOX     0
#define LV_USE_SPINNER     1
#define LV_USE_SWITCH      0
#define LV_USE_TABLE       0
#define LV_USE_TABVIEW     0
#define LV_USE_TEXTAREA    0
#define LV_USE_TILEVIEW    0
#define LV_USE_WIN         0

/* Fonts */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 0
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0
#define LV_FONT_UNSCII_8  0
#define LV_FONT_UNSCII_16 0
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Misc */
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR  0
#define LV_USE_SYSMON       0
#define LV_SPRINTF_CUSTOM   0
#define LV_SPRINTF_BUF_SIZE 256

/* Input */
#define LV_INDEV_DEF_READ_PERIOD         10
#define LV_INDEV_DEF_SCROLL_LIMIT        10
#define LV_INDEV_DEF_SCROLL_THROW        10
#define LV_INDEV_DEF_LONG_PRESS_TIME     400
#define LV_INDEV_DEF_LONG_PRESS_REP_TIME 100
#define LV_INDEV_DEF_GESTURE_LIMIT       50
#define LV_INDEV_DEF_GESTURE_MIN_VELOCITY 3

/* Animation / Group */
#define LV_USE_ANIM  1
#define LV_USE_GROUP 1

/* Image formats */
#define LV_USE_BMP  0
#define LV_USE_GIF  0
#define LV_USE_PNG  0

/* OS — none for simulator (single-threaded) */
#define LV_USE_OS LV_OS_NONE

#define LV_USE_PROFILER 0

#endif /* LV_CONF_H */
#endif
