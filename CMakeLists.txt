cmake_minimum_required(VERSION 3.16)
project(mp3_sim C)

set(CMAKE_C_STANDARD 11)

# ── Paths ──────────────────────────────────────────────────────────────
set(FW_MAIN  ${CMAKE_SOURCE_DIR}/../firmware/main)
set(LVGL_DIR ${CMAKE_SOURCE_DIR}/../firmware/managed_components/lvgl__lvgl)

# ── SDL2 ───────────────────────────────────────────────────────────────
find_package(SDL2 REQUIRED)

# ── LVGL sources ───────────────────────────────────────────────────────
file(GLOB_RECURSE LVGL_SRCS "${LVGL_DIR}/src/*.c")
list(FILTER LVGL_SRCS EXCLUDE REGEX ".*/demos/.*")
list(FILTER LVGL_SRCS EXCLUDE REGEX ".*/examples/.*")
list(FILTER LVGL_SRCS EXCLUDE REGEX ".*/nuttx/.*")
list(FILTER LVGL_SRCS EXCLUDE REGEX ".*/evdev/.*")
list(FILTER LVGL_SRCS EXCLUDE REGEX ".*/libinput/.*")
list(FILTER LVGL_SRCS EXCLUDE REGEX ".*/x11/.*")
list(FILTER LVGL_SRCS EXCLUDE REGEX ".*/windows/.*")

# ── UI sources ─────────────────────────────────────────────────────────
set(UI_SRCS
    ${FW_MAIN}/ui/ui_theme.c
    ${FW_MAIN}/ui/ui_player.c
    ${FW_MAIN}/ui/ui_library.c
    ${FW_MAIN}/ui/ui_screensaver.c  # real screensaver (buf mismatch fixed)
    ${FW_MAIN}/ui/rabbit.c
    ${FW_MAIN}/fonts/font_jb_9.c
    ${FW_MAIN}/fonts/font_jb_10.c
    ${FW_MAIN}/fonts/font_jb_11.c
    ${FW_MAIN}/fonts/font_jb_13b.c
)

# ── Simulator-specific sources ─────────────────────────────────────────
set(SIM_SRCS
    sim_main.c
    sim_audio.c         # miniaudio MP3 playback
    sim_albumart.c      # ID3v2 APIC + stb_image cover art
)

# ── Executable ─────────────────────────────────────────────────────────
add_executable(mp3_sim
    ${SIM_SRCS}
    ${UI_SRCS}
    ${LVGL_SRCS}
)

# ── Include dirs ────────────────────────────────────────────────────────
target_include_directories(mp3_sim PRIVATE
    ${CMAKE_SOURCE_DIR}          # lv_conf.h, miniaudio.h, stb_image.h
    ${CMAKE_SOURCE_DIR}/stubs    # esp_attr.h, esp_log.h, esp_err.h
    ${LVGL_DIR}                  # lvgl.h
    ${FW_MAIN}                   # config.h
    ${FW_MAIN}/ui
    ${FW_MAIN}/fonts
    ${FW_MAIN}/audio
    ${SDL2_INCLUDE_DIRS}
)

# ── Compile definitions ─────────────────────────────────────────────────
target_compile_definitions(mp3_sim PRIVATE
    LV_CONF_INCLUDE_SIMPLE=1
    LV_LVGL_H_INCLUDE_SIMPLE=1
    LV_USE_SDL=1
)

# ── Link ────────────────────────────────────────────────────────────────
target_link_libraries(mp3_sim PRIVATE
    ${SDL2_LIBRARIES}
    # miniaudio needs CoreAudio on macOS
    "-framework CoreAudio"
    "-framework AudioToolbox"
    "-framework CoreFoundation"
)

# ── Compiler flags ──────────────────────────────────────────────────────
target_compile_options(mp3_sim PRIVATE
    -Wall -Wextra
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    # miniaudio and stb_image generate many warnings we don't care about
    -Wno-deprecated-declarations
    -Wno-sign-compare
    -Wno-conversion
    -Wno-pedantic
)
