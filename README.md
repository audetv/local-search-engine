# Local Search Engine

Десктопное приложение для полнотекстового поиска по русскоязычным книгам.

## Возможности (планируется)
- Локальная индексация книг (FB2, EPUB, TXT, PDF)
- Полнотекстовый поиск с BM25 ранжированием
- Подключение к серверу Manticore Search для поиска по общему корпусу (176k+ книг)
- Мгновенная установка, не требует Docker/MySQL

## Требования
- C++20
- CMake 3.27+
- vcpkg


## Установка

## 1. Установите необходимые пакеты

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build git pkg-config curl unzip tar
```

## 2. Установите vcpkg

```bash
cd ~
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
```

## 3. Настройте переменную окружения VCPKG_ROOT

Добавьте в ваш ~/.bashrc:
```bash
echo 'export VCPKG_ROOT=~/vcpkg' >> ~/.bashrc
echo 'export PATH=$VCPKG_ROOT:$PATH' >> ~/.bashrc
source ~/.bashrc
```

## 4. Поправьте путь в CMakePresets.json

В корне проекта есть файл `CMakePresets.json`. Проверьте, какой там путь к vcpkg:
Создайте файл CMakeUserPresets.json
```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "default",
            "inherits": "debug",
            "environment": {
                "VCPKG_ROOT": "/home/{user}/vcpkg"
            }
        }
    ]
}
```

## 5. Установите зависимости через vcpkg

```bash
cd ~/projects/local-search-engine
vcpkg install
```

## 6. Соберите проект

```bash
cmake --preset=debug
cmake --build build/debug
```

## Альтернативный вариант (если не хотите править CMakePresets.json)

```bash
cd ~/projects/local-search-engine
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=~/projects/vcpkg/scripts/buildsystems/vcpkg.cmake -G Ninja
ninja
```

## Если всё ещё есть проблемы с компилятором

Убедитесь, что установлены компиляторы:

```bash
sudo apt install -y g++ gcc
which g++  # должен показать /usr/bin/g++
which gcc  # должен показать /usr/bin/gcc
```

Проверьте версии:
```bash
g++ --version
cmake --version
ninja --version
```

## Сборка

```bash
git clone https://github.com/audetv/local-search-engine
cd local-search-engine
rm -rf build/debug
cmake --preset=debug
cmake --build --preset=debug
ctest --preset=debug
```

## Проверка сборки
```bash
# Запуск приложения
./build/debug/src/local-search-engine
```
# Создание индекса с маппингом жанров
```bash
./build/debug/src/local-search-engine index test_books ./user_index --genres ./config/genres_map.csv
```

# Подключение к Manticore Search
```bash
export LSE_API_KEY="1234-user-key"
export LSE_SERVER_URL="http://localhost:18081"
./build/debug/src/local-search-engine
> remote бои на танках
```

# Запуск тестов
```bash
ctest --preset=debug
```

## Архитектура
См. [docs/architecture.md](docs/architecture.md)

## План разработки
См. [PLAN.md](PLAN.md)

## Лицензия
MIT