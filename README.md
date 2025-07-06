# Siemens filesystem extractor

#### Щито это
Тулза, позволяет извлекать содержимое файловой системы из FULLFLASH

##### Поддерживаемые платформы:
- SGOLD
- SGOLD2
- EGOLD с Card-Explorer (эксперементально в main ветке)

#####  В разработке/планах на 2035:
- SGOLD2 ELKA

## Как это использовать
```./ffnightman <options> <fullflash path>```

### Параметры:

```
Siemens filesystem extractor
  Version:           0.0.1-cc633d5-unstable
  libffshit version: 0.0.1-962324b-unstable

Usage:
  build/ffnightman [OPTION...] positional parameters

  -d, --debug           Enable debugging
  -p, --path arg        Destination path. Data_<Model>_<IMEI> by default
  -m, --platform arg    Specify platform (disable autodetect).
                        [ SGOLD2_ELKA SGOLD2 SGOLD EGOLD_CE ]
      --start-addr arg  Partition search start address (hex)
      --old             Old search algorithm
  -f, --partitions      partitions search for debugging purposes only
  -s, --scan            filesystem scanning for debugging purposes only
  -o, --overwrite       Always delete data directory if exists
      --skip            Skip broken file/directory
      --skip-dup        Skip duplicate id
      --proto           For fullflash from prototypes. Enable all skip
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
