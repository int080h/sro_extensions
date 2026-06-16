#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>

namespace sro {

class ObjectIndex {
public:
    bool Load(const std::wstring& clientPath);
    std::string ResolveBsrPath(uint32_t objId) const;
    bool HasObject(uint32_t objId) const { return m_mapping.count(objId) > 0; }
    size_t Count() const { return m_mapping.size(); }

private:
    bool ParseFile(const std::wstring& path);
    std::unordered_map<uint32_t, std::string> m_mapping;
};

} // namespace sro
