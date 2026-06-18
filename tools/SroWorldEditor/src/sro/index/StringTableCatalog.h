#pragma once

#include <string>

#include <unordered_map>

#include <vector>



namespace sro {



class StringTableCatalog {

public:

    bool Load(const std::wstring& clientPath);

    void BeginIncrementalLoad(const std::wstring& clientPath);

    bool LoadNextIndexFile(std::wstring* outIndexPath = nullptr);

    size_t TotalIndexCount() const { return m_pendingIndexFiles.size(); }

    size_t LoadedIndexCount() const { return m_indexLoadPos; }

    bool Loaded() const { return m_loaded; }



    size_t Count() const { return m_strings.size(); }



    std::string Resolve(const std::string& key) const;

    const std::string* Find(const std::string& key) const;

    const std::string* FindEnglish(const std::string& key) const { return Find(key); }



private:

    bool LoadIndexFile(const std::wstring& indexPath);

    bool ParseSplitFile(const std::wstring& path);

    void StoreString(const std::string& key, const std::string& label);

    void LogLoadStats() const;

    bool IsStringIndexFile(const std::wstring& filename) const;

    bool IsStringSplitFile(const std::wstring& filename) const;

    static std::wstring TextDataRoot(const std::wstring& clientPath);

    static std::string PickEnglishLabel(const std::vector<std::string>& cols);



    std::wstring m_clientPath;

    std::vector<std::wstring> m_prefixes;

    std::vector<std::wstring> m_pendingIndexFiles;

    size_t m_indexLoadPos = 0;

    bool m_loaded = false;

    std::unordered_map<std::string, std::string> m_strings;

};



} // namespace sro

