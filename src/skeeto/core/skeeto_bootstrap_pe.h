#pragma once
#include <Windows.h>
#include <cstring>

// Startup only, <=64 MiB input and <=4096 exports. Map data, never execute an
// unknown module merely to identify it. Offsets use 64-bit bounds arithmetic.
class SktPeFile final {
    HANDLE file_ = INVALID_HANDLE_VALUE, mapping_ = nullptr;
    const BYTE* data_ = nullptr;
    DWORD size_ = 0, sections_ = 0, table_ = 0;
    bool word(DWORD offset, WORD& out) const {
        if (offset > size_ || size_ - offset < sizeof(out)) return false;
        memcpy(&out, data_ + offset, sizeof(out)); return true;
    }
    bool dword(DWORD offset, DWORD& out) const {
        if (offset > size_ || size_ - offset < sizeof(out)) return false;
        memcpy(&out, data_ + offset, sizeof(out)); return true;
    }
    bool rva(DWORD address, DWORD bytes, DWORD& out) const {
        for (DWORD i = 0; i < sections_; ++i) {
            DWORD start = 0, size = 0, raw = 0;
            const DWORD section = table_ + 40 * i;
            if (!dword(section + 12, start) || !dword(section + 16, size) || !dword(section + 20, raw)) return false;
            if (address < start || static_cast<ULONGLONG>(address) + bytes > static_cast<ULONGLONG>(start) + size) continue;
            const ULONGLONG offset = static_cast<ULONGLONG>(raw) + address - start;
            if (offset + bytes > size_) return false;
            out = static_cast<DWORD>(offset); return true;
        }
        return false;
    }
public:
    ~SktPeFile() {
        if (data_) UnmapViewOfFile(data_);
        if (mapping_) CloseHandle(mapping_);
        if (file_ != INVALID_HANDLE_VALUE) CloseHandle(file_);
    }
    bool nativeNecola(const wchar_t* path) {
        file_ = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_ == INVALID_HANDLE_VALUE) return false;
        LARGE_INTEGER length{};
        if (!GetFileSizeEx(file_, &length) || length.QuadPart < 256 || length.QuadPart > 64 * 1024 * 1024) return false;
        size_ = static_cast<DWORD>(length.QuadPart);
        mapping_ = CreateFileMappingW(file_, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!mapping_) return false;
        data_ = static_cast<const BYTE*>(MapViewOfFile(mapping_, FILE_MAP_READ, 0, 0, 0));
        if (!data_) return false;
        WORD mz = 0, machine = 0, count = 0, optionalSize = 0, flags = 0, magic = 0;
        DWORD nt = 0, signature = 0, directoryCount = 0, exportRva = 0;
        if (!word(0, mz) || mz != IMAGE_DOS_SIGNATURE || !dword(60, nt) || nt < 64 || nt > size_ - 128 ||
            !dword(nt, signature) || signature != IMAGE_NT_SIGNATURE || !word(nt + 4, machine) || machine != IMAGE_FILE_MACHINE_I386 ||
            !word(nt + 6, count) || !count || count > 96 || !word(nt + 20, optionalSize) || optionalSize < 104 ||
            !word(nt + 22, flags) || !(flags & IMAGE_FILE_DLL) || !word(nt + 24, magic) || magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
            !dword(nt + 116, directoryCount) || !directoryCount || !dword(nt + 120, exportRva)) return false;
        table_ = nt + 24 + optionalSize; sections_ = count;
        if (static_cast<ULONGLONG>(table_) + 40 * sections_ > size_) return false;
        DWORD exports = 0, namesRva = 0, names = 0, namesCount = 0;
        if (!rva(exportRva, 40, exports) || !dword(exports + 24, namesCount) || namesCount > 4096 ||
            !dword(exports + 32, namesRva) || !rva(namesRva, namesCount * 4, names)) return false;
        bool found = false;
        for (DWORD i = 0; i < namesCount; ++i) {
            DWORD nameRva = 0, offset = 0;
            if (!dword(names + 4 * i, nameRva) || !rva(nameRva, 1, offset)) return false;
            const size_t available = size_ - offset < 256 ? size_ - offset : 256;
            const char* name = reinterpret_cast<const char*>(data_ + offset);
            if (!memchr(name, 0, available)) return false;
            if (!strcmp(name, "SkeetoBootstrapQuery") || !strcmp(name, "necola_export")) return false;
            if (!strcmp(name, "?necola_export@@YAXXZ")) found = true;
        }
        return found;
    }
};
