#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring> // для std::memcpy
#include <string>
#include <string_view>
#include <filesystem>

namespace lse
{

    class MemoryMappedFile
    {
    public:
        // Режим открытия
        enum class Mode
        {
            ReadOnly,  // только чтение существующего файла
            ReadWrite, // чтение и запись (growable)
            CreateNew  // создание нового файла
        };

        MemoryMappedFile() = default;
        ~MemoryMappedFile();

        // Некопируемый
        MemoryMappedFile(const MemoryMappedFile &) = delete;
        MemoryMappedFile &operator=(const MemoryMappedFile &) = delete;

        // Перемещаемый
        MemoryMappedFile(MemoryMappedFile &&other) noexcept;
        MemoryMappedFile &operator=(MemoryMappedFile &&other) noexcept;

        // Открыть файл
        auto open(const std::filesystem::path &path, Mode mode, size_t initial_size = 0) -> bool;

        // Закрыть файл
        void close();

        // Доступ к данным
        auto data() const -> const uint8_t * { return static_cast<const uint8_t *>(data_); }
        auto data() -> uint8_t * { return static_cast<uint8_t *>(data_); }
        auto size() const -> size_t { return file_size_; }

        // Проверка на открытость
        auto isOpen() const -> bool { return data_ != nullptr; }

        // Увеличить размер файла (для инкрементального добавления)
        auto grow(size_t new_size) -> bool;

        // Получить путь к файлу
        auto path() const -> std::filesystem::path { return file_path_; }

        // Синхронизировать с диском
        void flush();

        // Асинхронная синхронизация (совет ОС)
        void adviseWillNeed();
        void adviseSequential();

    private:
        void *data_ = nullptr;
        size_t file_size_ = 0;
        size_t mapped_size_ = 0;
        std::filesystem::path file_path_;
        Mode mode_ = Mode::ReadOnly;

#ifdef _WIN32
        void *file_handle_ = nullptr;
        void *mapping_handle_ = nullptr;
#else
        int fd_ = -1;
#endif
    };

    // Вспомогательная функция: чтение/запись примитивных типов через mmap
    template <typename T>
    inline T read_le(const uint8_t *data, size_t offset)
    {
        T value = 0;
        std::memcpy(&value, data + offset, sizeof(T));
        return value;
    }

    template <typename T>
    inline void write_le(uint8_t *data, size_t offset, T value)
    {
        std::memcpy(data + offset, &value, sizeof(T));
    }

    // Varint кодирование/декодирование
    auto encode_varint(uint64_t value, uint8_t *output) -> size_t;
    auto decode_varint(const uint8_t *data, size_t &offset) -> uint64_t;

} // namespace lse