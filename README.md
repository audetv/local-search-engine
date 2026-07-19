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