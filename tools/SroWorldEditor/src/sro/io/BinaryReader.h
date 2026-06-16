#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

namespace sro {

class BinaryReader {
public:
    explicit BinaryReader(const std::wstring& path) {
        m_stream.open(path, std::ios::binary);
    }

    bool IsOpen() const { return m_stream.is_open(); }
    bool Good() const { return static_cast<bool>(m_stream); }
    std::streampos Tell() const { return m_stream.tellg(); }
    void Seek(std::streampos pos) { m_stream.seekg(pos); }

    bool ReadBytes(void* dst, size_t count) {
        m_stream.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(count));
        return static_cast<bool>(m_stream);
    }

    template<typename T>
    bool Read(T& out) {
        return ReadBytes(&out, sizeof(T));
    }

    bool ReadString(char* buf, size_t len) {
        std::memset(buf, 0, len);
        return ReadBytes(buf, len);
    }

    bool ExpectSignature(const char* expected, size_t len = 12) {
        char sig[13] = {};
        if (!ReadString(sig, len)) return false;
        return std::strncmp(sig, expected, len) == 0;
    }

    bool ReadSignature(std::string& out, size_t len = 12) {
        char sig[13] = {};
        if (!ReadString(sig, len)) return false;
        out.assign(sig, len);
        return true;
    }

    size_t Remaining() const {
        auto cur = m_stream.tellg();
        m_stream.seekg(0, std::ios::end);
        auto end = m_stream.tellg();
        const_cast<std::ifstream&>(m_stream).seekg(cur);
        if (cur < 0 || end < 0) return 0;
        return static_cast<size_t>(end - cur);
    }

private:
    mutable std::ifstream m_stream;
};

class BinaryWriter {
public:
    explicit BinaryWriter(const std::wstring& path) {
        m_stream.open(path, std::ios::binary | std::ios::trunc);
    }

    bool IsOpen() const { return m_stream.is_open(); }
    bool Good() const { return static_cast<bool>(m_stream); }

    bool WriteBytes(const void* src, size_t count) {
        m_stream.write(reinterpret_cast<const char*>(src), static_cast<std::streamsize>(count));
        return static_cast<bool>(m_stream);
    }

    template<typename T>
    bool Write(const T& val) {
        return WriteBytes(&val, sizeof(T));
    }

    bool WriteSignature(const std::string& sig, size_t len = 12) {
        char buf[12] = {};
        std::strncpy(buf, sig.c_str(), len);
        return WriteBytes(buf, len);
    }

private:
    std::ofstream m_stream;
};

} // namespace sro
