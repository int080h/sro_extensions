#pragma once
#include <fstream>
#include <string>

template<typename T>
inline void ReadVal(std::ifstream& fs, T& val) {
    fs.read(reinterpret_cast<char*>(&val), sizeof(T));
}

inline std::string ReadString(std::ifstream& fs, uint32_t len) {
    if (len == 0) return "";
    std::string s(len, '\0');
    fs.read(&s[0], len);
    while (!s.empty() && (s.back() == '\0' || s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t')) {
        s.pop_back();
    }
    return s;
}

template<typename T>
inline void WriteVal(std::ofstream& fs, const T& val) {
    fs.write(reinterpret_cast<const char*>(&val), sizeof(T));
}

inline void WriteString(std::ofstream& fs, const std::string& s) {
    fs.write(s.c_str(), s.size());
}
