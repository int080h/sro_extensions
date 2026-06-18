#pragma once
#include <string>
#include <vector>

namespace TextDecode {

std::string WideToUtf8(const std::wstring& wide);
std::wstring Utf8ToWide(const std::string& utf8);

// Read Silkroad textdata split/index file (UTF-16 LE with optional BOM, or UTF-8).
bool ReadTextDataLines(const std::wstring& path, std::vector<std::string>& outLines);

std::vector<std::string> SplitTabColumns(const std::string& line);

bool IsValidUtf8(const std::string& s);
bool IsPrintableDisplayText(const std::string& s);
bool IsMostlyLatinText(const std::string& s);
std::string StripTranslationQuotes(const std::string& s);

} // namespace TextDecode
