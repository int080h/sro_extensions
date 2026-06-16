#include "sro_map/MapTextureCache.h"
#include "Rendering/Texture.h"
#include "Parsers/DdjParser.h"
#include "Core/Log.h"
#include <fstream>

MapTextureCache::MapTextureCache(LPDIRECT3DDEVICE9 device) : m_device(device) {}

void MapTextureCache::BeginFrame() {
    m_loadsThisFrame = 0;
    m_visiblePinned.clear();
}

void MapTextureCache::SetLoadBudget(int maxLoadsPerFrame) {
    m_loadBudget = maxLoadsPerFrame > 0 ? maxLoadsPerFrame : kMaxLoadsPerFrame;
}

bool MapTextureCache::IsProtected(const std::wstring& path) const {
    return m_pinned.count(path) > 0 || m_visiblePinned.count(path) > 0;
}

size_t MapTextureCache::StreamEntryCount() const {
    size_t count = 0;
    for (const auto& entry : m_cache) {
        if (!IsProtected(entry.first)) ++count;
    }
    return count;
}

void MapTextureCache::RemoveFromLru(const std::wstring& path) {
    for (auto it = m_lru.begin(); it != m_lru.end(); ++it) {
        if (*it == path) {
            m_lru.erase(it);
            return;
        }
    }
}

void MapTextureCache::TouchLru(const std::wstring& path) {
    if (IsProtected(path)) return;
    RemoveFromLru(path);
    m_lru.push_back(path);
}

void MapTextureCache::EvictIfNeeded() {
    size_t guard = m_lru.size();
    while (StreamEntryCount() >= kMaxStreamEntries && !m_lru.empty() && guard > 0) {
        --guard;
        const std::wstring victim = m_lru.front();
        m_lru.pop_front();
        if (IsProtected(victim)) continue;
        m_cache.erase(victim);
    }
}

void MapTextureCache::LogLoadFailure(const std::wstring& path, const wchar_t* reason) {
    if (m_loggedFailures.count(path)) return;
    m_loggedFailures.insert(path);
    LogMsgW(std::wstring(L"[MapTextureCache] ") + reason + L": " + path);
}

Texture* MapTextureCache::LoadTexture(const std::wstring& path, bool countTowardBudget) {
    if (path.empty()) return nullptr;

    auto it = m_cache.find(path);
    if (it != m_cache.end()) {
        if (it->second) {
            TouchLru(path);
            return it->second.get();
        }
        // Drop failed tombstone so a later attempt can retry (preload / index complete).
        m_cache.erase(it);
        RemoveFromLru(path);
        m_loggedFailures.erase(path);
    }

    if (countTowardBudget && m_loadsThisFrame >= m_loadBudget) {
        return nullptr;
    }

    {
        std::ifstream probe(path, std::ios::binary);
        if (!probe.is_open()) {
            LogLoadFailure(path, L"File not found");
            m_cache[path] = nullptr;
            return nullptr;
        }
    }

    std::vector<uint8_t> ddsBytes = DdjParser::LoadToDdsBytes(path);
    if (ddsBytes.empty()) {
        LogLoadFailure(path, L"Failed to decode DDJ/DDS bytes");
        m_cache[path] = nullptr;
        return nullptr;
    }

    auto tex = Texture::LoadFromDdsBytes(m_device, ddsBytes);
    if (!tex) {
        LogLoadFailure(path, L"Failed to parse DDS texture");
        m_cache[path] = nullptr;
        return nullptr;
    }

    if (countTowardBudget) ++m_loadsThisFrame;
    Texture* ptr = tex.get();
    m_cache[path] = std::move(tex);
    TouchLru(path);
    EvictIfNeeded();
    return ptr;
}

Texture* MapTextureCache::Preload(const std::wstring& path) {
    return LoadTexture(path, false);
}

Texture* MapTextureCache::Acquire(const std::wstring& path, MapLoadPolicy policy) {
    switch (policy) {
    case MapLoadPolicy::NoLoad: {
        auto it = m_cache.find(path);
        return (it != m_cache.end() && it->second) ? it->second.get() : nullptr;
    }
    case MapLoadPolicy::Immediate:
        return LoadTexture(path, false);
    case MapLoadPolicy::Throttled:
    default:
        return LoadTexture(path, true);
    }
}

Texture* MapTextureCache::Acquire(const std::wstring& path, bool allowLoad) {
    return Acquire(path, allowLoad ? MapLoadPolicy::Throttled : MapLoadPolicy::NoLoad);
}

void MapTextureCache::Pin(const std::wstring& path) {
    if (path.empty()) return;
    m_pinned.insert(path);
    RemoveFromLru(path);
}

void MapTextureCache::PinSet(const std::vector<std::wstring>& paths) {
    for (const auto& path : paths) Pin(path);
}

void MapTextureCache::PinVisible(const std::wstring& path) {
    if (path.empty()) return;
    m_visiblePinned.insert(path);
    RemoveFromLru(path);
}

bool MapTextureCache::IsPinned(const std::wstring& path) const {
    return m_pinned.count(path) > 0;
}

void MapTextureCache::UnpinFailed(const std::wstring& path) {
    auto it = m_cache.find(path);
    if (it != m_cache.end() && !it->second) {
        m_cache.erase(it);
        RemoveFromLru(path);
        m_loggedFailures.erase(path);
    }
}

void MapTextureCache::Clear() {
    m_cache.clear();
    m_lru.clear();
    m_pinned.clear();
    m_visiblePinned.clear();
    m_loggedFailures.clear();
    m_loadsThisFrame = 0;
}

bool MapTextureCache::IsReady(const std::wstring& path) const {
    auto it = m_cache.find(path);
    return it != m_cache.end() && it->second && it->second->pTexture;
}
