# Siemens filesystem extractor

#### Щито это
Тулза, позволяет извлекать содержимое файловой системы из FULLFLASH и записывать в неё

##### Поддерживаемые платформы:
- SGOLD
- SGOLD2
- SGOLD2 ELKA \
Требует тестирования
- EGOLD с Card-Explorer \
Требует тестирования. Только извлечение

## Как это использовать
```./ffnightman <options> <fullflash path>```

### Параметры:

```
Siemens filesystem extractor
  Version:           0.0.2-5bccd9b-unstable
  libffshit version: 0.0.2-afe9d15-unstable

Usage:
  ./ffnightman [OPTION...] <ffpath>

 Extraction options:
  -p, --path arg    Destination path. './<FF_file_name>_data' by default
  -o, --overwrite   Always delete data directory if exists
      --skip        Skip broken file/directory
      --skip-dup    Skip duplicate id
      --skip-all    Enable all skip
                    
      --part arg    Partition to extract (may be several)
      --regexp arg  Extract content whose path matches the regexp. C++ 
                    regex compatible (default: "")

 Partitions search options:
  -m, --platform arg    Specify platform (disable autodetect).
                        [ SGOLD2_ELKA SGOLD2 SGOLD EGOLD_CE ]
      --start-addr arg  Partition search start address (hex)
      --old             Old search algorithm
      --part-scan       partitions search for debugging purposes only

 Filesystem options:
  -f, --fs-platform arg  Specify filesystem type (for fullflash from 
                         prototype by example).
                         [ SGOLD2_ELKA SGOLD2 SGOLD EGOLD_CE ]
      --fs-scan          filesystem scanning for debugging purposes only

 Listing options:
      --ls arg  List content whose path matches the regexp. C++ regex 
                compatible (default: "")

 Writing options:
      --put arg    Write a local file into the filesystem, replacing a file 
                   of that name: <local path>:<path>, e.g. 
                   photo.jpg:FFS_0/Pictures/photo.jpg
      --mkdir arg  Create a directory, e.g. FFS_0/Pictures/Holidays
      --rm arg     Remove a file or an empty directory
      --out arg    Save the changed fullflash to this file. Needed by 
                   --put, --mkdir and --rm, which may be given several 
                   times and run in the order given

 Logging options:
  -l, --log      Save log to file '<dst_path>/extracting.log'
  -v, --verbose  Verbose level
                 v   - Verbose processing
                 vv  - Verbose headers
                 vvv - Verbose data
  -d, --debug    Verbose level = vvv
  -h, --help     Help
  ```

### Запись
`--put` записывает файл в файловую систему, заменяя файл с тем же именем, `--mkdir` создаёт каталог, `--rm` удаляет файл или пустой каталог. Пути начинаются с имени раздела, как в выводе `--ls`, регистр букв в именах не различается — как и в телефоне. Каждый параметр можно указать несколько раз, они выполняются в том порядке, в каком указаны, а изменённый фуллфлеш сохраняется в файл из `--out`:

```
./ffnightman --mkdir FFS_0/Pictures/Holidays --put photo.jpg:FFS_0/Pictures/Holidays/photo.jpg --out new.bin ff.bin
```

Запись поддерживается для SGOLD, SGOLD2 и SGOLD2 ELKA. На SGOLD имена могут содержать только символы ASCII. Если что-то записать не удалось — нет места, нет каталога, — ffnightman ничего не сохраняет.

## Тесты
[tests/e2e](tests/e2e/README.md): записывают файлы в фуллфлеши, загружают телефоны с ними в эмуляторе pmb887x-emu и скачивают файлы по OBEX.

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
