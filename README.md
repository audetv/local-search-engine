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
cmake --preset=debug
cmake --build --preset=debug
```

## Архитектура
См. [docs/architecture.md](docs/architecture.md)

## План разработки
См. [PLAN.md](PLAN.md)

## Лицензия
MIT