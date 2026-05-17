# Rabbitrack R1 — Simulator

SDL2 симулятор прошивки Rabbitrack R1 для разработки UI на macOS/Linux без реального железа.

Симулятор воспроизводит интерфейс LVGL, проигрывает настоящие MP3 файлы через
[miniaudio](https://miniaud.io) и пишет сентинел-файл `/tmp/mp3player_sim`,
по которому [Rabbitrack Manager](https://github.com/oliviaisntcringe/rabbitrack-r1-companion)
обнаруживает симулятор и подключается автоматически.

---

## Возможности

- Полный рендер LVGL интерфейса в SDL2 окне (480×320)
- Воспроизведение MP3 через miniaudio (header-only)
- Dot-grid waveform, обложки треков, кролик-маскот
- Эмуляция кнопок клавиатурой
- Автоматическое обнаружение Rabbitrack Manager

---

## Требования

### macOS

```bash
# Xcode Command Line Tools
xcode-select --install

# SDL2 и CMake через Homebrew
brew install sdl2 cmake
```

### Linux (Ubuntu / Debian)

```bash
sudo apt update
sudo apt install build-essential cmake libsdl2-dev
```

---

## Сборка и запуск

```bash
git clone https://github.com/oliviaisntcringe/rabbitrack-r1-simulator.git
cd rabbitrack-r1-simulator

mkdir build && cd build
cmake ..
make -j$(nproc)

# Запустить с папкой Music:
./mp3_sim ~/Music

# Или указать другую папку с MP3:
./mp3_sim /path/to/your/music
```

Симулятор сканирует указанную папку рекурсивно на MP3 файлы.

---

## Управление (клавиатура)

| Клавиша | Действие |
|---------|----------|
| `←` | Предыдущий трек |
| `Space` | Play / Pause |
| `→` | Следующий трек |
| `↑` | Громче |
| `↓` | Тише |
| `R` | Пересканировать папку (после добавления треков) |
| `Q` / `Esc` | Выйти |

---

## Интеграция с Rabbitrack Manager

При запуске симулятор создаёт файл `/tmp/mp3player_sim` с путём к папке Music.
[Rabbitrack Manager](https://github.com/oliviaisntcringe/rabbitrack-r1-companion)
опрашивает этот файл каждые 500ms и автоматически подключается.

При выходе из симулятора файл удаляется — Manager автоматически отключается.

**Workflow:**
1. Запустить симулятор: `./mp3_sim ~/Music`
2. Запустить Rabbitrack Manager — он подключится автоматически
3. Добавлять/удалять треки через Manager, нажать `R` в симуляторе для обновления

---

## Структура проекта

```
rabbitrack-r1-simulator/
├── CMakeLists.txt          — сборочный скрипт
├── sim_main.c              — точка входа, SDL2 окно, LVGL рендер, IPC сентинел
├── sim_audio.c/h           — miniaudio + minimp3 воспроизведение
├── sim_albumart.c/h        — загрузка обложек (stb_image)
├── sim_screensaver_stub.c  — заглушка скринсейвера для симулятора
├── lv_conf.h               — конфигурация LVGL для симулятора
├── miniaudio.h             — аудио backend (header-only)
├── stb_image.h             — загрузчик изображений (header-only)
└── stubs/
    ├── esp_log.h           — заглушка ESP-IDF логгера
    ├── esp_err.h           — заглушка ESP-IDF ошибок
    └── esp_attr.h          — заглушка ESP-IDF атрибутов
```

---

## Зависимости (все embedded)

| Библиотека | Версия | Лицензия |
|------------|--------|----------|
| LVGL | v9.x | MIT |
| miniaudio | 0.11.x | MIT-0 / Unlicense |
| minimp3 | — | CC0 |
| stb_image | 2.x | MIT / Public Domain |
| SDL2 | системная | zlib |

LVGL подключается как git submodule или через системный пакет — см. `CMakeLists.txt`.

---

## Лицензия

Проект является приватным. Все права защищены.  
**Автор:** tuerleprince
