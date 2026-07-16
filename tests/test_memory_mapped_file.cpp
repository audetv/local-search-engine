#include <catch2/catch_test_macros.hpp>
#include "index/memory_mapped_file.hpp"
#include <filesystem>
#include <cstring>

using namespace lse;
namespace fs = std::filesystem;

TEST_CASE("MemoryMappedFile: create and write", "[mmap]")
{
    auto path = fs::temp_directory_path() / "test_mmap.bin";

    {
        MemoryMappedFile mmf;
        REQUIRE(mmf.open(path, MemoryMappedFile::Mode::CreateNew, 128));
        REQUIRE(mmf.data() != nullptr);
        REQUIRE(mmf.size() == 128);

        // Записать магическое число
        write_le<uint32_t>(mmf.data(), 0, 0xDEADBEEF);
    }

    // Файл должен существовать после закрытия
    REQUIRE(fs::exists(path));
    REQUIRE(fs::file_size(path) == 128);

    // Проверить чтение
    {
        MemoryMappedFile mmf;
        REQUIRE(mmf.open(path, MemoryMappedFile::Mode::ReadOnly));
        REQUIRE(read_le<uint32_t>(mmf.data(), 0) == 0xDEADBEEF);
    }

    fs::remove(path);
}

TEST_CASE("MemoryMappedFile: grow", "[mmap]")
{
    auto path = fs::temp_directory_path() / "test_grow.bin";

    {
        MemoryMappedFile mmf;
        REQUIRE(mmf.open(path, MemoryMappedFile::Mode::CreateNew, 64));
        REQUIRE(mmf.size() == 64);

        // Заполняем первые 64 байта
        std::memset(mmf.data(), 0xAA, 64);

        // Расширяем
        REQUIRE(mmf.grow(256));
        REQUIRE(mmf.size() == 256);

        // Первые 64 байта должны сохраниться
        for (size_t i = 0; i < 64; ++i)
        {
            CHECK(mmf.data()[i] == 0xAA);
        }
    }

    REQUIRE(fs::file_size(path) == 256);
    fs::remove(path);
}

TEST_CASE("MemoryMappedFile: varint encoding", "[mmap]")
{
    uint8_t buf[10];

    // 0 → 1 байт
    size_t len = encode_varint(0, buf);
    REQUIRE(len == 1);
    REQUIRE(buf[0] == 0);

    // 127 → 1 байт
    len = encode_varint(127, buf);
    REQUIRE(len == 1);
    REQUIRE(buf[0] == 127);

    // 128 → 2 байта
    len = encode_varint(128, buf);
    REQUIRE(len == 2);
    REQUIRE(buf[0] == 0x80);
    REQUIRE(buf[1] == 0x01);

    size_t offset = 0;
    REQUIRE(decode_varint(buf, offset) == 128);
    REQUIRE(offset == 2);
}

TEST_CASE("MemoryMappedFile: move semantics", "[mmap]")
{
    auto path = fs::temp_directory_path() / "test_move.bin";

    MemoryMappedFile mmf1;
    REQUIRE(mmf1.open(path, MemoryMappedFile::Mode::CreateNew, 32));
    write_le<uint64_t>(mmf1.data(), 0, 0x1234567890ABCDEF);

    // Перемещаем
    MemoryMappedFile mmf2 = std::move(mmf1);
    REQUIRE(!mmf1.isOpen());
    REQUIRE(mmf2.isOpen());
    REQUIRE(read_le<uint64_t>(mmf2.data(), 0) == 0x1234567890ABCDEF);

    mmf2.close();
    fs::remove(path);
}