#include "TextDecode.h"
#include <windows.h>
#include <fstream>
#include <codecvt>
#include <locale>

namespace TextDecode {

std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return {};
    const int utf8Count = WideCharToMultiByte(CP_UTF8, 0, wide.data(),
        static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (utf8Count <= 0) return {};
    std::string utf8(static_cast<size_t>(utf8Count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
        utf8.data(), utf8Count, nullptr, nullptr);
    return utf8;
}

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    const int wcharCount = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
        static_cast<int>(utf8.size()), nullptr, 0);
    if (wcharCount <= 0) return {};
    std::wstring wide(static_cast<size_t>(wcharCount), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
        wide.data(), wcharCount);
    return wide;
}

std::vector<std::string> SplitTabColumns(const std::string& line) {
    std::vector<std::string> cols;
    std::string cur;
    for (char c : line) {
        if (c == '\t') {
            cols.push_back(cur);
            cur.clear();
        } else if (c != '\r') {
            cur.push_back(c);
        }
    }
    cols.push_back(cur);
    return cols;
}

bool IsValidUtf8(const std::string& s) {
    size_t i = 0;
    while (i < s.size()) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 0x7F) { ++i; continue; }
        int extra = 0;
        if ((c & 0xE0) == 0xC0) extra = 1;
        else if ((c & 0xF0) == 0xE0) extra = 2;
        else if ((c & 0xF8) == 0xF0) extra = 3;
        else return false;
        if (i + static_cast<size_t>(extra) >= s.size()) return false;
        for (int j = 1; j <= extra; ++j) {
            if ((static_cast<unsigned char>(s[i + j]) & 0xC0) != 0x80) return false;
        }
        i += static_cast<size_t>(extra) + 1;
    }
    return true;
}

bool IsPrintableDisplayText(const std::string& s) {
    if (s.empty()) return false;
    if (!IsValidUtf8(s)) return false;
    for (unsigned char c : s) {
        if (c < 0x20 && c != '\t') return false;
        if (c == 0x7F) return false;
    }
    return true;
}

bool IsMostlyLatinText(const std::string& s) {
    if (s.empty() || !IsValidUtf8(s)) return false;
    int content = 0;
    int latin = 0;
    for (unsigned char c : s) {
        if (c == ' ' || c == '\t') continue;
        ++content;
        if (c < 0x80 || (c >= 0xC0 && c <= 0xFF)) ++latin;
    }
    return content > 0 && latin * 100 / content >= 90;
}

std::string StripTranslationQuotes(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}

static std::wstring ReadWideContent(const std::string& bytes) {
    if (bytes.size() < 2) return {};

    const bool hasBom = static_cast<unsigned char>(bytes[0]) == 0xFF
        && static_cast<unsigned char>(bytes[1]) == 0xFE;

    try {
        std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>> conv;
        if (hasBom) return conv.from_bytes(bytes);
        return conv.from_bytes(std::string("\xff\xfe", 2) + bytes);
    } catch (...) {}

    const size_t start = hasBom ? 2 : 0;
    if (bytes.size() <= start + 1) return {};
    std::wstring wide((bytes.size() - start) / 2, L'\0');
    for (size_t i = 0; i < wide.size(); ++i) {
        wide[i] = static_cast<wchar_t>(
            static_cast<unsigned char>(bytes[start + i * 2])
            | (static_cast<unsigned char>(bytes[start + i * 2 + 1]) << 8));
    }
    return wide;
}

static void SplitUtf8Lines(const std::string& text, std::vector<std::string>& outLines) {
    size_t start = 0;
    while (start <= text.size()) {
        const size_t end = text.find('\n', start);
        std::string line = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        outLines.push_back(std::move(line));
        if (end == std::string::npos) break;
        start = end + 1;
    }
    while (!outLines.empty() && outLines.back().empty()) outLines.pop_back();
}

bool ReadTextDataLines(const std::wstring& path, std::vector<std::string>& outLines) {
    outLines.clear();
    std::ifstream fs(path, std::ios::binary);
    if (!fs.is_open()) return false;

    const std::string bytes((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    if (bytes.empty()) return false;

    const bool looksUtf16 = (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0xFF
        && static_cast<unsigned char>(bytes[1]) == 0xFE)
        || (bytes.size() >= 4 && bytes[1] == '\0' && bytes[3] == '\0');

    if (looksUtf16) {
        const std::wstring wide = ReadWideContent(bytes);
        if (!wide.empty()) {
            SplitUtf8Lines(WideToUtf8(wide), outLines);
            return !outLines.empty();
        }
    }

    SplitUtf8Lines(bytes, outLines);
    return !outLines.empty();
}

} // namespace TextDecode
