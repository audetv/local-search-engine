#include "memory_mapped_file.hpp"
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace lse
{

    MemoryMappedFile::~MemoryMappedFile()
    {
        close();
    }

    MemoryMappedFile::MemoryMappedFile(MemoryMappedFile &&other) noexcept
        : data_(other.data_), file_size_(other.file_size_), mapped_size_(other.mapped_size_), file_path_(std::move(other.file_path_)), mode_(other.mode_)
#ifdef _WIN32
          ,
          file_handle_(other.file_handle_), mapping_handle_(other.mapping_handle_)
#else
          ,
          fd_(other.fd_)
#endif
    {
        other.data_ = nullptr;
        other.file_size_ = 0;
#ifdef _WIN32
        other.file_handle_ = nullptr;
        other.mapping_handle_ = nullptr;
#else
        other.fd_ = -1;
#endif
    }

    MemoryMappedFile &MemoryMappedFile::operator=(MemoryMappedFile &&other) noexcept
    {
        if (this != &other)
        {
            close();
            data_ = other.data_;
            file_size_ = other.file_size_;
            mapped_size_ = other.mapped_size_;
            file_path_ = std::move(other.file_path_);
            mode_ = other.mode_;
#ifdef _WIN32
            file_handle_ = other.file_handle_;
            mapping_handle_ = other.mapping_handle_;
#else
            fd_ = other.fd_;
#endif
            other.data_ = nullptr;
        }
        return *this;
    }

    auto MemoryMappedFile::open(const std::filesystem::path &path, Mode mode, size_t initial_size) -> bool
    {
        close();
        file_path_ = path;
        mode_ = mode;

#ifdef _WIN32
        DWORD access = 0;
        DWORD disposition = 0;
        DWORD protect = 0;
        DWORD map_access = 0;

        switch (mode)
        {
        case Mode::ReadOnly:
            access = GENERIC_READ;
            disposition = OPEN_EXISTING;
            protect = PAGE_READONLY;
            map_access = FILE_MAP_READ;
            break;
        case Mode::ReadWrite:
            access = GENERIC_READ | GENERIC_WRITE;
            disposition = OPEN_ALWAYS;
            protect = PAGE_READWRITE;
            map_access = FILE_MAP_READ | FILE_MAP_WRITE;
            break;
        case Mode::CreateNew:
            access = GENERIC_READ | GENERIC_WRITE;
            disposition = CREATE_ALWAYS;
            protect = PAGE_READWRITE;
            map_access = FILE_MAP_READ | FILE_MAP_WRITE;
            break;
        }

        file_handle_ = CreateFileW(path.c_str(), access, 0, nullptr,
                                   disposition, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_handle_ == INVALID_HANDLE_VALUE)
            return false;

        LARGE_INTEGER size;
        if (mode == Mode::CreateNew && initial_size > 0)
        {
            size.QuadPart = initial_size;
            SetFilePointerEx(file_handle_, size, nullptr, FILE_BEGIN);
            SetEndOfFile(file_handle_);
        }
        else
        {
            GetFileSizeEx(file_handle_, &size);
        }
        file_size_ = size.QuadPart;
        mapped_size_ = file_size_;

        if (file_size_ > 0)
        {
            mapping_handle_ = CreateFileMappingW(file_handle_, nullptr, protect, 0, 0, nullptr);
            if (!mapping_handle_)
            {
                CloseHandle(file_handle_);
                return false;
            }
            data_ = MapViewOfFile(mapping_handle_, map_access, 0, 0, 0);
            if (!data_)
            {
                CloseHandle(mapping_handle_);
                CloseHandle(file_handle_);
                return false;
            }
        }
#else
        int flags = 0;
        mode_t perms = 0644;

        switch (mode)
        {
        case Mode::ReadOnly:
            flags = O_RDONLY;
            break;
        case Mode::ReadWrite:
            flags = O_RDWR | O_CREAT;
            break;
        case Mode::CreateNew:
            flags = O_RDWR | O_CREAT | O_TRUNC;
            break;
        }

        fd_ = ::open(path.c_str(), flags, perms);
        if (fd_ < 0)
            return false;

        struct stat st;
        if (fstat(fd_, &st) < 0)
        {
            ::close(fd_);
            return false;
        }

        if (mode == Mode::CreateNew && initial_size > 0)
        {
            if (ftruncate(fd_, initial_size) < 0)
            {
                ::close(fd_);
                return false;
            }
            file_size_ = initial_size;
        }
        else
        {
            file_size_ = st.st_size;
        }
        mapped_size_ = file_size_;

        if (file_size_ > 0)
        {
            int prot = (mode == Mode::ReadOnly) ? PROT_READ : (PROT_READ | PROT_WRITE);
            data_ = mmap(nullptr, file_size_, prot, MAP_SHARED, fd_, 0);
            if (data_ == MAP_FAILED)
            {
                data_ = nullptr;
                ::close(fd_);
                return false;
            }
        }
#endif

        return true;
    }

    void MemoryMappedFile::close()
    {
        if (data_)
        {
#ifdef _WIN32
            UnmapViewOfFile(data_);
            data_ = nullptr;
            if (mapping_handle_)
            {
                CloseHandle(mapping_handle_);
                mapping_handle_ = nullptr;
            }
            if (file_handle_)
            {
                SetFilePointerEx(file_handle_, {.QuadPart = static_cast<LONGLONG>(file_size_)}, nullptr, FILE_BEGIN);
                SetEndOfFile(file_handle_);
                CloseHandle(file_handle_);
                file_handle_ = nullptr;
            }
#else
            munmap(data_, mapped_size_);
            data_ = nullptr;
            if (fd_ >= 0)
            {
                ftruncate(fd_, file_size_);
                ::close(fd_);
                fd_ = -1;
            }
#endif
        }
        file_size_ = 0;
        mapped_size_ = 0;
    }

    auto MemoryMappedFile::grow(size_t new_size) -> bool
    {
        if (new_size <= file_size_)
            return true;

#ifdef _WIN32
        UnmapViewOfFile(data_);
        CloseHandle(mapping_handle_);

        LARGE_INTEGER size;
        size.QuadPart = new_size;
        SetFilePointerEx(file_handle_, size, nullptr, FILE_BEGIN);
        SetEndOfFile(file_handle_);

        mapping_handle_ = CreateFileMappingW(file_handle_, nullptr, PAGE_READWRITE, 0, 0, nullptr);
        if (!mapping_handle_)
            return false;

        data_ = MapViewOfFile(mapping_handle_, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
        if (!data_)
            return false;
#else
        if (ftruncate(fd_, new_size) < 0)
            return false;

        munmap(data_, mapped_size_);
        data_ = mmap(nullptr, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (data_ == MAP_FAILED)
        {
            data_ = nullptr;
            return false;
        }
#endif

        mapped_size_ = new_size;
        file_size_ = new_size;
        return true;
    }

    void MemoryMappedFile::flush()
    {
        if (data_)
        {
#ifdef _WIN32
            FlushViewOfFile(data_, file_size_);
            FlushFileBuffers(file_handle_);
#else
            msync(data_, file_size_, MS_SYNC);
#endif
        }
    }

    void MemoryMappedFile::adviseWillNeed()
    {
#ifndef _WIN32
        if (data_)
            madvise(data_, file_size_, MADV_WILLNEED);
#endif
    }

    void MemoryMappedFile::adviseSequential()
    {
#ifndef _WIN32
        if (data_)
            madvise(data_, file_size_, MADV_SEQUENTIAL);
#endif
    }

    // Varint кодирование
    auto encode_varint(uint64_t value, uint8_t *output) -> size_t
    {
        size_t i = 0;
        while (value >= 0x80)
        {
            output[i++] = (value & 0x7F) | 0x80;
            value >>= 7;
        }
        output[i++] = value & 0x7F;
        return i;
    }

    // Varint декодирование
    auto decode_varint(const uint8_t *data, size_t &offset) -> uint64_t
    {
        uint64_t result = 0;
        int shift = 0;
        while (true)
        {
            uint8_t byte = data[offset++];
            result |= (uint64_t(byte & 0x7F) << shift);
            if (!(byte & 0x80))
                break;
            shift += 7;
        }
        return result;
    }

} // namespace lse