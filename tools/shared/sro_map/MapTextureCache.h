#pragma once
#include <d3d9.h>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class Texture;

enum class MapLoadPolicy {
    Throttled,
    Immediate,
    NoLoad
};

// Isolated LRU cache for 2D map/minimap DDJs — not cleared by 3D TextureManager eviction.
class MapTextureCache {
public:
    static constexpr int kMaxLoadsPerFrame = 8;
    static constexpr int kBurstLoadsPerFrame = 48;
    static constexpr size_t kMaxStreamEntries = 1024;

    explicit MapTextureCache(LPDIRECT3DDEVICE9 device);

    void BeginFrame();
    void SetLoadBudget(int maxLoadsPerFrame);
    int LoadBudget() const { return m_loadBudget; }

    Texture* Acquire(const std::wstring& path, MapLoadPolicy policy = MapLoadPolicy::Throttled);
    Texture* Acquire(const std::wstring& path, bool allowLoad);
    Texture* Preload(const std::wstring& path);

    void Pin(const std::wstring& path);
    void PinSet(const std::vector<std::wstring>& paths);
    void PinVisible(const std::wstring& path);
    bool IsPinned(const std::wstring& path) const;
    size_t PinCount() const { return m_pinned.size(); }
    void UnpinFailed(const std::wstring& path);

    void Clear();

    int LoadsThisFrame() const { return m_loadsThisFrame; }
    size_t Size() const { return m_cache.size(); }
    bool IsReady(const std::wstring& path) const;

private:
    LPDIRECT3DDEVICE9 m_device = nullptr;
    std::map<std::wstring, std::unique_ptr<Texture>> m_cache;
    std::list<std::wstring> m_lru;
    std::set<std::wstring> m_pinned;
    std::set<std::wstring> m_visiblePinned;
    std::set<std::wstring> m_loggedFailures;
    int m_loadsThisFrame = 0;
    int m_loadBudget = kMaxLoadsPerFrame;

    bool IsProtected(const std::wstring& path) const;
    size_t StreamEntryCount() const;
    void TouchLru(const std::wstring& path);
    void RemoveFromLru(const std::wstring& path);
    void EvictIfNeeded();
    Texture* LoadTexture(const std::wstring& path, bool countTowardBudget);
    void LogLoadFailure(const std::wstring& path, const wchar_t* reason);
};
