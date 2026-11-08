# Сборка под Windows x86 (32 бита)

Эта инструкция описывает кросс-сборку Castle Escape под 32-битный Windows
(`i686`, PE32) с помощью MinGW-w64 и проверку полученного `.exe` под Wine.
Сборка полностью локальная: скрипт скачивает и собирает Lua и SDL2 внутри
`build-windows-x86/`, ничего не устанавливает в систему и не требует `sudo`.

## Что нужно

На хосте Linux (проверено на Debian/Ubuntu) должны быть:

- CMake 3.16+;
- GNU Make;
- `curl` и `tar`;
- MinGW-w64 для 32-битной цели:
  `sudo apt install gcc-mingw-w64-i686 g++-mingw-w64-i686`;
- Wine для запуска 32-битных PE-файлов (в обычном WoW64-префиксе):
  `sudo apt install wine` (на Debian/Ubuntu может понадобиться
  `wine32:i386` и `dpkg --add-architecture i386`).

Пакеты SDL2 и Lua для Windows в системе не нужны: скрипт собирает их из
исходников сам.

## Сборка одной командой

Из корня репозитория:

```sh
./scripts/build_windows_x86.sh
```

Скрипт идемпотентен: повторный запуск использует уже скачанные архивы и
инкрементально пересобирает зависимости. Каталог сборки по умолчанию —
`build-windows-x86/`, его можно переопределить:

```sh
BUILD_DIR=/tmp/castle-windows ./scripts/build_windows_x86.sh
```

Внутри `build-windows-x86/` размещаются:

- `downloads/` — архивы Lua 5.4.8 и SDL2 2.32.10;
- `src/` — распакованные исходники;
- `deps/i686/` — заголовки и статические библиотеки `liblua.a`, `libSDL2.a`;
- `sdl2-build/`, `project/` — каталоги CMake;
- `wineprefix/` — отдельный Wine-префикс для проверок.

## Где готовый файл

```text
dist/windows-x86/castle_escape.exe
```

Это PE32-файл для Intel 80386 (консольная подсистема). Скрипт копирует его
из `build-windows-x86/project/` и проверяет тип командой `file`.

## Запуск и тесты под Wine

Скрипт сам создаёт свежий `WINEPREFIX` внутри `build-windows-x86/wineprefix`,
выставляет `WINEDEBUG=-all` и запускает шесть тестов через CTest, а затем
явный smoke-тест. Вручную это можно повторить так:

```sh
export WINEPREFIX="$PWD/build-windows-x86/wineprefix"
export WINEDEBUG=-all
unset WINEARCH
export SDL_VIDEODRIVER=dummy
export SDL_AUDIODRIVER=dummy

wine dist/windows-x86/castle_escape.exe --smoke-test
```

Ожидаемый результат smoke-теста:

```text
PASS: SDL window, 120 fixed updates and rendered frames; player x=304; Space fires once, repeat ignored, Ammo 5 and ALARM in title
```

Полный набор тестов (пути указывают на собранные `.exe` в каталоге проекта):

```sh
ctest --test-dir build-windows-x86/project --output-on-failure
```

Если CTest/эмулятор по какой-то причине недоступен, те же шесть программ
можно запустить напрямую:

```sh
wine build-windows-x86/project/game_logic_test.exe
wine build-windows-x86/project/guards_rooms_test.exe
wine build-windows-x86/project/combat_alarm_test.exe
wine build-windows-x86/project/objectives_test.exe
wine build-windows-x86/project/application_test.exe
wine build-windows-x86/project/castle_escape.exe --smoke-test
```

## Что слинковано статически

Исполняемый файл собирается с
`-static -static-libgcc -static-libstdc++`, а SDL2 и Lua — как статические
библиотеки (`libSDL2.a`, `liblua.a`). Поэтому рядом с `.exe` не нужны:

- `libstdc++-6.dll`;
- `libgcc_s_*.dll`;
- `libwinpthread-1.dll`;
- `SDL2.dll`;
- `lua*.dll`.

Скрипт проверяет это через `i686-w64-mingw32-objdump -p` и завершается с
ошибкой, если такая зависимость появилась. Системные библиотеки Windows
(`KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `WINMM.dll`,
`api-ms-win-crt-*.dll` из Universal CRT и т. п.) остаются динамическими — они
являются частью ОС.

## Почему не используется `WINEARCH=win32`

Wine 11.20 работает в режиме WoW64 и отклоняет `WINEARCH=win32` с сообщением
«not supported in wow64 mode». Для 32-битного PE нужен обычный (по умолчанию
создаваемый) WoW64-префикс, поэтому скрипт явно сбрасывает `WINEARCH` через
`unset WINEARCH` и не задаёт его.

## CI

Workflow `.github/workflows/windows-x86.yml` для `ubuntu-latest`:

1. включает архитектуру `i386` и ставит `g++-mingw-w64-i686`,
   `gcc-mingw-w64-i686`, `wine`, `wine32:i386`, `cmake`, `curl`, `make`;
2. запускает `./scripts/build_windows_x86.sh`;
3. выгружает `dist/windows-x86/castle_escape.exe` как артефакт
   `castle_escape-windows-x86`.

Workflow не требует секретов и не предполагает, что репозиторий публичный.

## Ограничения

Сборка под 32-битный Windows выполняется кросс-компиляцией на Linux и
проверяется только под Wine. Она не запускалась на физической машине с
Windows, и поведение реального драйвера/оконного менеджера Windows может
отличаться от Wine. Тесты логики платформонезависимы, а интеграционный
`sdl_smoke` использует заглушки `SDL_VIDEODRIVER=dummy` и
`SDL_AUDIODRIVER=dummy`, то есть проверяет создание окна и кадры без
настоящего дисплея и звука.
