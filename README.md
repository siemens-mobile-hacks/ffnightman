# Siemens filesystem extractor

#### Щито это
Тулза, позволяет извлекать содержимое файловой системы из FULLFLASH

Поддерживаемые платформы:
* X65
* X75

В разработке/планах на 2035:
* X85
* X55

## Как это использовать
```./ffnightman <options> <fullflash path>```

### Параметры:

```
Siemens filesystem extractor
Usage:
  ./ffnightman [OPTION...] positional parameters

  -d, --debug           Enable debugging
  -p, --path arg        Destination path. Data_<Model>_<IMEI> by default
  -m, --platform arg    Specify platform (disable autodetect).
                        [ X85 X75 X65 ]
      --start-addr arg  Partition search start address (hex)

      --old             Old search algorithm
  -f, --partitions      partitions search for debugging purposes only
  -s, --scan            filesystem scanning for debugging purposes only
  -o, --overwrite       Always delete data directory if exists
  -h, --help            Help
  ```

## Бинарные сборки
Coming soon
## Сборка из исходников
- ### *nix
  - #### Установка зависимостей
    - ##### Arch Linux
     ```
     > sudo pacman -S gcc make cmake git fmt spdlog
     ```

    - ##### Ubuntu 24.40
    ```
    > sudo apt update
    > sudo apt install build-essential git cmake libfmt9 libfmt-dev libspdlog1.12 libspdlog-dev
    ```

  - #### Сборка
    Переходим в удобный нам каталог.
    Клонируем git репозиторй libffshit

    ```
    > git clone https://github.com/siemens-mobile-hacks/libffshit.git
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

- ### Windows
    Coming soon