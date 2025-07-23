# Siemens filesystem extractor

#### Щито это
Тулза, позволяет извлекать содержимое файловой системы из FULLFLASH

##### Поддерживаемые платформы:
- SGOLD
- SGOLD2
- SGOLD2 ELKA \
Требует тестирования
- EGOLD с Card-Explorer \
Только с простым поиском блоков (работает только на фуллах без патча на диск)

## Как это использовать
```./ffnightman <options> <fullflash path>```

### Параметры:

```
Siemens filesystem extractor
  Version:           0.0.2-0b90a0c-unstable
  libffshit version: 0.0.2-6ca41c6-unstable

Usage:
  ffnightman [OPTION...] positional parameters

  -d, --debug           Enable debugging
  -p, --path arg        Destination path. './<FF_file_name>_data' by 
                        default
  -m, --platform arg    Specify platform (disable autodetect).
                        [ SGOLD2_ELKA SGOLD2 SGOLD EGOLD_CE ]
  -l, --log             Save log to file '<dst_path>/extracting.log'
      --dump            Dump data to debug output
      --start-addr arg  Partition search start address (hex)
      --part arg        Partition to extract (may be several)
      --old             Old search algorithm
  -f, --partitions      partitions search for debugging purposes only
  -s, --scan            filesystem scanning for debugging purposes only
  -o, --overwrite       Always delete data directory if exists
      --skip            Skip broken file/directory
      --skip-dup        Skip duplicate id
      --skip-all        Enable all skip
  -h, --help            Help
  ```

## Бинарные и не очень сборки
- Ubuntu 24.04: см. Releases
- Windows X64: см. Releases
- Arch Linux (AUR): (Спасибо <a href="mailto:kirill.zhumarin@gmail.com">Kirill Zhumarin</a>)
  - https://aur.archlinux.org/packages/libffshit-git
  - https://aur.archlinux.org/packages/ffnightman-git

## Сборка из исходников
### *nix 
  - #### Установка зависимостей
    - ##### Arch Linux
     ```
     > sudo pacman -S gcc make cmake git fmt spdlog
     ```

    - ##### Ubuntu 24.04
    ```
    > sudo apt update
    > sudo apt install build-essential git cmake libfmt9 libfmt-dev libspdlog1.12 libspdlog-dev
    ```

  - #### Сборка
    Переходим в удобный нам каталог.
    Клонируем репозиторй libffshit

    ```
    ~ > git clone https://github.com/siemens-mobile-hacks/libffshit.git
    ```
    Собираем
    ```
    ~ > cd libffshit
    ~/libffshit > cmake -DCMAKE_BUILD_TYPE="Release" -B build .
    ~/libffshit > cmake --build build --config Release
    ~/libffshit > cmake --install build --config Release --prefix target
    ```

    Переходим на уровень выше и клонируем репозиторий ffnightman

    ```
    ~/libffshit > cd ...
    ~ > git clone https://github.com/siemens-mobile-hacks/ffnightman.git
    ```
    Собираем
    ```
    ~ > cd ffnightman
    ~/ffnightman > cmake -DCMAKE_PREFIX_PATH="~/libffshit/target" -DCMAKE_BUILD_TYPE="Release" -B build
    ~/ffnightman > cmake --build build --config Release
    ```
### Windows
Coming soon

## Спасибы
- Описание таблицы разметки дисков/паттерны:\
  Azq2 \
  marry_on_me \
  Feyman
- Паттерн поиска начала таблицы разметки дисков SGOLD/SGOLD2/ELKA: \
  Feyman
- FAT Timestamp: \
  perk11
- Патчи на увеличение диска EGOLD
 (Помогли с анализом таблицы разметки диска): \
  kay \
  AlexSid \
  SiNgle \
  Chaos \
  avkiev \
  Baloo
- Smelter: \
  avkiev
- Тестирование: \
  perk11 \
  Feyman \
  FIL \
  maximuservice \
  marry_on_me
