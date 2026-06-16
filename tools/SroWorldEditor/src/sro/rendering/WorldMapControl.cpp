#include "WorldMapControl.h"
#include "MapPlayerIcon.h"
#include "Core/Log.h"
#include "Core/Utils.h"
#include "Rendering/Texture.h"
#include <imgui.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <cstdio>
#include <set>
#include <windowsx.h>

// ============================================================================
// String Utility Functions (private to this translation unit)
// ============================================================================

static std::string TrimAscii(const std::string &s) {
  size_t start = 0;
  while (start < s.size() && (s[start] == ' ' || s[start] == '\t' ||
                              s[start] == '\r' || s[start] == '\n')) {
    ++start;
  }
  size_t end = s.size();
  while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' ||
                         s[end - 1] == '\r' || s[end - 1] == '\n')) {
    --end;
  }
  return s.substr(start, end - start);
}

static std::wstring TrimWideText(const std::wstring &s) {
  size_t start = 0;
  while (start < s.size() && iswspace(s[start])) {
    ++start;
  }
  size_t end = s.size();
  while (end > start && iswspace(s[end - 1])) {
    --end;
  }
  return s.substr(start, end - start);
}

static std::vector<std::string> SplitTabs(const std::string &line) {
  std::vector<std::string> cols;
  std::stringstream ss(line);
  std::string item;
  while (std::getline(ss, item, '\t')) {
    cols.push_back(item);
  }
  return cols;
}

static std::vector<std::wstring> SplitTabsWide(const std::wstring &line) {
  std::vector<std::wstring> cols;
  std::wstringstream ss(line);
  std::wstring item;
  while (std::getline(ss, item, L'\t')) {
    cols.push_back(item);
  }
  return cols;
}

static std::string ToUtf8FromWide(const std::wstring &wide) {
  if (wide.empty()) return {};
  int utf8Count = WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                                      static_cast<int>(wide.size()), nullptr, 0,
                                      nullptr, nullptr);
  if (utf8Count <= 0) return {};
  std::string utf8(static_cast<size_t>(utf8Count), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                      utf8.data(), utf8Count, nullptr, nullptr);
  return utf8;
}

static std::string ToUtf8FromGameText(const std::string &bytes) {
  if (bytes.empty()) return {};
  auto convertToWide = [&](UINT codePage, DWORD flags) -> std::wstring {
    int wcharCount = MultiByteToWideChar(codePage, flags, bytes.data(),
                                         static_cast<int>(bytes.size()), nullptr, 0);
    if (wcharCount <= 0) return {};
    std::wstring wide(static_cast<size_t>(wcharCount), L'\0');
    if (MultiByteToWideChar(codePage, flags, bytes.data(),
                            static_cast<int>(bytes.size()), wide.data(), wcharCount) <= 0) {
      return {};
    }
    return wide;
  };

  std::wstring wide = convertToWide(CP_UTF8, MB_ERR_INVALID_CHARS);
  if (wide.empty()) wide = convertToWide(949, 0);
  if (wide.empty()) wide = convertToWide(CP_ACP, 0);
  if (wide.empty()) return bytes;

  int utf8Count = WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                                      static_cast<int>(wide.size()), nullptr, 0,
                                      nullptr, nullptr);
  if (utf8Count <= 0) return bytes;
  std::string utf8(static_cast<size_t>(utf8Count), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                      utf8.data(), utf8Count, nullptr, nullptr);
  return utf8;
}

static bool ParseIntColumn(const std::vector<std::string> &cols, size_t idx, int &out) {
  if (idx >= cols.size()) return false;
  std::string value = TrimAscii(cols[idx]);
  if (value.empty() || value == "xxx") return false;
  try { out = std::stoi(value); return true; } catch (...) { return false; }
}

static bool ParseIntWideColumn(const std::vector<std::wstring> &cols, size_t idx, int &out) {
  if (idx >= cols.size()) return false;
  std::wstring value = TrimWideText(cols[idx]);
  if (value.empty() || value == L"xxx") return false;
  try { out = std::stoi(value); return true; } catch (...) { return false; }
}

static bool FileHasUtf16LeBom(const std::wstring &path) {
  std::ifstream fs(path, std::ios::binary);
  if (!fs) return false;
  unsigned char bom[2] = {};
  fs.read(reinterpret_cast<char *>(bom), 2);
  return bom[0] == 0xFF && bom[1] == 0xFE;
}

static std::wstring Utf8ToWide(const std::string &utf8) {
  if (utf8.empty()) return {};
  int wcharCount = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                                       nullptr, 0);
  if (wcharCount <= 0) return {};
  std::wstring wide(static_cast<size_t>(wcharCount), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(),
                      wcharCount);
  return wide;
}

static std::optional<std::wstring> FindTextDataFilePath(const std::wstring &clientPath,
                                                        const wchar_t *relativePath) {
  const std::wstring candidates[] = {
      clientPath + L"/Media/textdata/" + relativePath,
      clientPath + L"/Media/server_dep/silkroad/textdata/" + relativePath};
  for (const auto &path : candidates) {
    if (std::filesystem::exists(path)) return path;
  }
  return std::nullopt;
}

static bool AppendDungeonMapDefinitionFromCols(
    const std::vector<std::string> &cols, const std::wstring &clientPath,
    std::vector<WorldMapControl::InstanceMapDefinition> &out) {
  if (cols.size() < 16) return false;

  WorldMapControl::InstanceMapDefinition def;
  if (!ParseIntColumn(cols, 0, def.Id) || !ParseIntColumn(cols, 8, def.RenderWidth) ||
      !ParseIntColumn(cols, 9, def.RenderHeight)) {
    return false;
  }
  int mx0 = 0, my0 = 0, mx1 = 0, my1 = 0;
  if (!ParseIntColumn(cols, 10, mx0) || !ParseIntColumn(cols, 11, my0) ||
      !ParseIntColumn(cols, 12, mx1) || !ParseIntColumn(cols, 13, my1)) {
    return false;
  }

  def.NameUtf8 = ToUtf8FromGameText(TrimAscii(cols[1]));
  if (!cols[2].empty()) def.FloorKind = cols[2][0];
  ParseIntColumn(cols, 3, def.FloorIndex);
  ParseIntColumn(cols, 4, def.FloorCount);
  ParseIntColumn(cols, 6, def.TileCols);
  ParseIntColumn(cols, 7, def.TileRows);
  def.TextureWidth = def.RenderWidth;
  def.TextureHeight = def.RenderHeight;
  def.MinMx = (std::min)(mx0, mx1);
  def.MaxMx = (std::max)(mx0, mx1);
  def.MinMy = (std::min)(my0, my1);
  def.MaxMy = (std::max)(my0, my1);

  std::string texPrefix = TrimAscii(cols[15]);
  std::replace(texPrefix.begin(), texPrefix.end(), '\\', '/');
  def.IsTiled = true;
  def.TilePrefixPath = clientPath + L"/Media/" +
                       std::wstring(texPrefix.begin(), texPrefix.end());
  def.TexturePath = def.TilePrefixPath;
  if (cols.size() > 17) {
    std::string folder = TrimAscii(cols[17]);
    def.DungeonFolder = std::wstring(folder.begin(), folder.end());
    def.DungeonGroupKey = sro::map::DungeonGroupKeyFromFolder(folder);
  }
  if (cols.size() > 16) {
    std::string prefix = TrimAscii(cols[16]);
    def.TilePrefix = std::wstring(prefix.begin(), prefix.end());
  }
  if (def.RenderWidth <= 0 || def.RenderHeight <= 0) return false;
  out.push_back(std::move(def));
  return true;
}

static std::wstring CleanTranslation(const std::wstring &s) {
  if (s.empty()) return s;
  std::wstring res = s;
  if (res.front() == L'\"' && res.back() == L'\"' && res.size() >= 2) {
    res = res.substr(1, res.size() - 2);
  }
  return res;
}

static std::wstring StripSpaces(const std::wstring &s) {
  std::wstring res;
  for (wchar_t c : s) {
    if (c != L' ' && c != L'\t' && c != L'\r' && c != L'\n') {
      res.push_back(c);
    }
  }
  return res;
}

// Global for canvas offset (used by mouse input)
extern float g_canvasStartX;
extern float g_canvasStartY;

// ============================================================================
// WorldMapControl Implementation
// ============================================================================

WorldMapControl::WorldMapControl(LPDIRECT3DDEVICE9 device, TextureManager *texMgr, MapTextureCache *mapCache)
    : m_device(device), m_texManager(texMgr), m_mapCache(mapCache),
      m_minMx(46), m_maxMx(180), m_minMy(70), m_maxMy(116), m_worldMinMx(46),
      m_worldMaxMx(180), m_worldMinMy(70), m_worldMaxMy(113), m_zoom(1.0f),
      m_panOffset(0.0f, 0.0f), m_lastMousePos(0.0f, 0.0f), m_isDragging(false),
      m_hasCenteredInitialRegion(false), m_lastPanOrZoomTick(0) {}

void WorldMapControl::ScanRegionFiles(const std::wstring &clientPath) {
  LogMsgW(L"[WorldMapControl] ScanRegionFiles started for path: " + clientPath);
  m_clientPath = clientPath;
  m_worldTiles.clear();
  m_worldOverlayTiles.clear();
  m_worldPreloadPaths.clear();
  m_worldPreloadIndex = 0;
  m_worldPreloadComplete = false;
  m_worldPreloadLogged = false;
  m_mapRenderDirty = true;
  m_minimapIndex.Clear();
  m_minimapIndexComplete = false;
  m_minimapScanDirs.clear();
  m_minimapScanDirIndex = 0;
  m_minimapIndexFilesTotal = 0;
  m_minimapIndexFilesProcessed = 0;
  if (m_minimapScanHandle != INVALID_HANDLE_VALUE) {
    FindClose(m_minimapScanHandle);
    m_minimapScanHandle = INVALID_HANDLE_VALUE;
  }
  m_worldGuideMarkers.clear();
  m_localMapDefs.clear();
  m_instanceMapDefs.clear();
  m_localPoiMarkers.clear();
  m_worldCoveredRegions.clear();
  SelectedLocalMapId = 0;
  SelectedDungeonFloorId = 0;
  m_hasSavedWorldView = false;
  m_worldMinMx = 46; m_worldMaxMx = 180; m_worldMinMy = 70; m_worldMaxMy = 113;
  m_minimapMinMx = 46; m_minimapMaxMx = 180; m_minimapMinMy = 70; m_minimapMaxMy = 116;

  LogMsgW(L"[WorldMapControl] Scanning world tiles...");
  ScanWorldTiles();
  LogMsgW(L"[WorldMapControl] Found " + std::to_wstring(m_worldTiles.size()) + L" world tiles.");
  LogMsgW(L"[WorldMapControl] Scanning world overlay tiles...");
  ScanWorldOverlayTiles();
  LogMsgW(L"[WorldMapControl] Found " + std::to_wstring(m_worldOverlayTiles.size()) + L" world overlay tiles.");
  LoadWorldGuideMarkers();
  LoadWorldMapDefinitions();
  LoadDungeonMapDefinitions();
  if (m_instanceMapDefs.empty()) {
    LogMsg("[WorldMapControl] No dungeon definitions from dungeonmap.txt; trying "
           "worldmap_instanceinfo.txt fallback...");
    LoadInstanceMapDefinitions();
  }
  LoadTranslations();
  LoadLocalPoiMarkers();
  LinkDungeonEntrances();

  LogMsgW(L"[WorldMapControl] Applying bounds...");
  ApplyBoundsForCurrentStyle();

  const size_t activeWorldTiles = CountActiveWorldTiles();
  LogMsgW(L"[WorldMapControl] ActiveRegions: " + std::to_wstring(ActiveRegions.size()) +
          L", loaded mega-tiles: " + std::to_wstring(m_worldTiles.size()) +
          L" (MFO-active mega-tiles: " + std::to_wstring(activeWorldTiles) + L")" +
          L", current region (" + std::to_wstring(CurrentRx) + L"," + std::to_wstring(CurrentRy) +
          L") active=" + (IsRegionActive(CurrentRx, CurrentRy) ? L"yes" : L"no"));

  BeginMinimapIndexScan();
  BeginWorldPreload();
  LogMsgW(L"[WorldMapControl] Metadata scan complete. " +
          std::to_wstring(m_worldTiles.size()) + L" world tiles indexed; minimap index building in background.");
}

void WorldMapControl::SetMinimapIndexListener(std::function<void()> listener) {
  m_minimapIndexListener = std::move(listener);
}

void WorldMapControl::SetMinimapHudActive(bool active, int centerRx, int centerRy) {
  m_minimapHudActive = active;
  m_hudCenterRx = centerRx;
  m_hudCenterRy = centerRy;
}

void WorldMapControl::EnsureOverlayTextures() {
  auto load = [this](Texture*& slot, const wchar_t* relPath) {
    if (slot && slot->pTexture) return;
    slot = m_texManager->GetTexture(m_clientPath + relPath, false);
  };
  load(m_worldMonsterIconTex, L"/Media/interface/worldmap/wmap_monster_icon.ddj");
  load(m_localPoiIconTex, L"/Media/interface/worldmap/wmap_sign_questnpc.ddj");
  load(m_playerMarkerTex, L"/Media/interface/minimap/mm_sign_character.ddj");
  load(m_partyMarkerTex, L"/Media/interface/worldmap/wmap_sign_party.ddj");
  load(m_questNpcMarkerTex, L"/Media/interface/minimap/mm_sign_questnpc.ddj");
}

void WorldMapControl::InvalidateOverlayTextures() {
  m_worldMonsterIconTex = nullptr;
  m_localPoiIconTex = nullptr;
  m_playerMarkerTex = nullptr;
  m_partyMarkerTex = nullptr;
  m_questNpcMarkerTex = nullptr;
}

std::optional<std::wstring> WorldMapControl::PathForMinimapRegion(int rx, int ry) const {
  if (SelectedDungeonFloorId > 0) {
    const InstanceMapDefinition *floor = FindInstanceMapById(SelectedDungeonFloorId);
    if (floor && !floor->DungeonFolder.empty() && !floor->TilePrefix.empty()) {
      std::wstring path = m_clientPath + L"/Media/minimap_d/" + floor->DungeonFolder + L"/" +
                          floor->TilePrefix + std::to_wstring(ry) + L"x" + std::to_wstring(rx) +
                          L".ddj";
      if (std::filesystem::exists(path)) return path;
    }
  }
  auto path = m_minimapIndex.PathForRegion(rx, ry);
  if (path && sro::map::PathHasSpacedCoords(*path)) return std::nullopt;
  return path;
}

Texture *WorldMapControl::AcquireMapTexture(const std::wstring &path, MapLoadPolicy policy) {
  if (!m_mapCache || path.empty()) return nullptr;
  return m_mapCache->Acquire(path, policy);
}

void WorldMapControl::BeginWorldPreload() {
  m_worldPreloadPaths.clear();
  m_worldPreloadPaths.reserve(m_worldTiles.size());
  for (const auto &kv : m_worldTiles) {
    m_worldPreloadPaths.push_back(kv.second);
  }
  if (m_mapCache) m_mapCache->PinSet(m_worldPreloadPaths);
  m_worldPreloadIndex = 0;
  m_worldPreloadComplete = m_worldPreloadPaths.empty();
  m_worldPreloadLogged = false;
  MarkMapRenderDirty();
}

void WorldMapControl::TickWorldPreload(int maxPerTick) {
  if (m_worldPreloadComplete || !m_mapCache || maxPerTick <= 0) return;

  int loaded = 0;
  while (m_worldPreloadIndex < m_worldPreloadPaths.size() && loaded < maxPerTick) {
    const std::wstring &path = m_worldPreloadPaths[m_worldPreloadIndex++];
    const bool wasReady = m_mapCache->IsReady(path);
    Texture *tex = m_mapCache->Preload(path);
    if (!wasReady && tex && tex->pTexture) MarkMapRenderDirty();
    ++loaded;
  }

  if (m_worldPreloadIndex >= m_worldPreloadPaths.size()) {
    m_worldPreloadComplete = true;
    if (!m_worldPreloadLogged) {
      m_worldPreloadLogged = true;
      size_t ready = 0;
      size_t failed = 0;
      for (const auto &path : m_worldPreloadPaths) {
        if (m_mapCache->IsReady(path)) ++ready;
        else ++failed;
      }
      LogMsgW(L"[WorldMapControl] World tiles preloaded: " + std::to_wstring(ready) + L"/" +
              std::to_wstring(m_worldPreloadPaths.size()) +
              (failed > 0 ? (L" (" + std::to_wstring(failed) + L" failed)") : L""));
      MarkMapRenderDirty();
    }
  }
}

float WorldMapControl::WorldPreloadProgress() const {
  if (m_worldPreloadPaths.empty()) return 1.0f;
  if (m_worldPreloadComplete) return 1.0f;
  return static_cast<float>(m_worldPreloadIndex) / static_cast<float>(m_worldPreloadPaths.size());
}

void WorldMapControl::TickBackground() {
  TickMinimapIndexScan(400);
  TickWorldPreload(64);
}

void WorldMapControl::BeginMinimapIndexScan() {
  m_minimapIndex.Clear();
  m_minimapIndexComplete = false;
  m_minimapScanDirs.clear();
  m_minimapScanDirIndex = 0;
  m_minimapIndexFilesProcessed = 0;
  m_minimapIndexFilesTotal = 0;

  if (m_minimapScanHandle != INVALID_HANDLE_VALUE) {
    FindClose(m_minimapScanHandle);
    m_minimapScanHandle = INVALID_HANDLE_VALUE;
  }

  m_minimapScanDirs.push_back(m_clientPath + L"/Media/minimap");
  std::wstring dungeonDir = m_clientPath + L"/Media/minimap_d";
  WIN32_FIND_DATAW fd;
  HANDLE hFind = FindFirstFileW((dungeonDir + L"/*").c_str(), &fd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fd.cFileName[0] != L'.') {
        m_minimapScanDirs.push_back(dungeonDir + L"/" + fd.cFileName);
      }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
  }

  for (const auto &dir : m_minimapScanDirs) {
    HANDLE h = FindFirstFileW((dir + L"/*.ddj").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
      do { ++m_minimapIndexFilesTotal; } while (FindNextFileW(h, &fd));
      FindClose(h);
    }
  }

  LogMsgW(L"[WorldMapControl] Minimap index scan queued: " +
          std::to_wstring(m_minimapIndexFilesTotal) + L" files in " +
          std::to_wstring(m_minimapScanDirs.size()) + L" directories.");
}

bool WorldMapControl::RegisterMinimapFile(const std::wstring &dirPath, const std::wstring &filename) {
  size_t dot = filename.find_last_of(L'.');
  if (dot == std::wstring::npos) return false;

  std::wstring stem = filename.substr(0, dot);
  std::wstring coordPart = stem;
  size_t lastUnderscore = stem.find_last_of(L'_');
  if (lastUnderscore != std::wstring::npos) {
    std::wstring candidate = stem.substr(lastUnderscore + 1);
    if (candidate.find(L'x') != std::wstring::npos) coordPart = candidate;
  }

  size_t xPos = coordPart.find(L'x');
  if (xPos == std::wstring::npos || xPos == 0 || xPos >= coordPart.size() - 1) return false;

  try {
    int ry = std::stoi(coordPart.substr(0, xPos));
    int rx = std::stoi(coordPart.substr(xPos + 1));
    if (!m_minimapIndex.PathForRegion(rx, ry)) {
      std::wstring fullPath = dirPath + L"/" + filename;
      if (sro::map::PathHasSpacedCoords(fullPath)) {
        LogMsgW(L"[WorldMapControl] Skipping minimap path with spaced coords: " + fullPath);
        return false;
      }
      m_minimapIndex.Insert(rx, ry, std::move(fullPath));
      m_minimapMinMx = (std::min)(m_minimapMinMx, ry);
      m_minimapMaxMx = (std::max)(m_minimapMaxMx, ry);
      m_minimapMinMy = (std::min)(m_minimapMinMy, rx);
      m_minimapMaxMy = (std::max)(m_minimapMaxMy, rx);
      return true;
    }
  } catch (...) {}
  return false;
}

bool WorldMapControl::TickMinimapIndexScan(int maxFilesPerChunk) {
  if (m_minimapIndexComplete) return false;

  int processed = 0;
  while (processed < maxFilesPerChunk) {
    if (m_minimapScanDirIndex >= m_minimapScanDirs.size()) {
      m_minimapIndexComplete = true;
      ApplyBoundsForCurrentStyle();
      LogMsgW(L"[WorldMapControl] Minimap index complete: " +
              std::to_wstring(m_minimapIndex.Size()) + L" region tiles, " +
              std::to_wstring(m_worldTiles.size()) + L" world tiles.");
      if (m_minimapIndexListener) m_minimapIndexListener();
      return false;
    }

    const std::wstring &dir = m_minimapScanDirs[m_minimapScanDirIndex];
    if (m_minimapScanHandle == INVALID_HANDLE_VALUE) {
      m_minimapScanHandle = FindFirstFileW((dir + L"/*.ddj").c_str(), &m_minimapScanFd);
      if (m_minimapScanHandle == INVALID_HANDLE_VALUE) {
        ++m_minimapScanDirIndex;
        continue;
      }
    }

    RegisterMinimapFile(dir, m_minimapScanFd.cFileName);
    ++m_minimapIndexFilesProcessed;
    ++processed;

    if (!FindNextFileW(m_minimapScanHandle, &m_minimapScanFd)) {
      FindClose(m_minimapScanHandle);
      m_minimapScanHandle = INVALID_HANDLE_VALUE;
      ++m_minimapScanDirIndex;
    }
  }
  return true;
}

float WorldMapControl::MinimapIndexProgress() const {
  if (m_minimapIndexComplete) return 1.0f;
  if (m_minimapIndexFilesTotal == 0) return 0.0f;
  return static_cast<float>(m_minimapIndexFilesProcessed) / static_cast<float>(m_minimapIndexFilesTotal);
}

void WorldMapControl::QueueVisibleLoads(float width, float height) {
  if (!m_mapCache || width <= 0 || height <= 0) return;

  float worldX0 = (0.0f - m_panOffset.x) / m_zoom;
  float worldX1 = (width - m_panOffset.x) / m_zoom;
  float worldY0 = (0.0f - m_panOffset.y) / m_zoom;
  float worldY1 = (height - m_panOffset.y) / m_zoom;

  int minMx = m_minMx + static_cast<int>(floorf(worldX0 / sro::map::kMapPixelsPerRegion)) - 1;
  int maxMx = m_minMx + static_cast<int>(floorf(worldX1 / sro::map::kMapPixelsPerRegion)) + 1;
  int maxMy = m_maxMy - static_cast<int>(floorf(worldY0 / sro::map::kMapPixelsPerRegion)) + 1;
  int minMy = m_maxMy - static_cast<int>(floorf(worldY1 / sro::map::kMapPixelsPerRegion)) - 1;

  float centerWorldX = (width * 0.5f - m_panOffset.x) / m_zoom;
  float centerWorldY = (height * 0.5f - m_panOffset.y) / m_zoom;
  int centerMx = 0, centerMy = 0;
  sro::map::WorldPixelToMapCell(centerWorldX, centerWorldY, m_minMx, m_maxMy, centerMx, centerMy);

  int loadBudget = MapTextureCache::kMaxLoadsPerFrame;
  const DWORD now = GetTickCount();
  if (now - m_lastPanOrZoomTick < 300) {
    loadBudget = MapTextureCache::kBurstLoadsPerFrame;
  }
  m_mapCache->SetLoadBudget(loadBudget);

  if (m_minimapIndexComplete) {
    for (const auto &path : m_worldPreloadPaths) {
      m_mapCache->UnpinFailed(path);
    }
    if (MapStyle == 1) {
      int mmMinMx = std::clamp(minMx, m_minimapMinMx, m_minimapMaxMx);
      int mmMaxMx = std::clamp(maxMx, m_minimapMinMx, m_minimapMaxMx);
      int mmMinMy = std::clamp(minMy, m_minimapMinMy, m_minimapMaxMy);
      int mmMaxMy = std::clamp(maxMy, m_minimapMinMy, m_minimapMaxMy);
      for (int my = mmMinMy; my <= mmMaxMy; ++my) {
        for (int mx = mmMinMx; mx <= mmMaxMx; ++mx) {
          if (auto path = m_minimapIndex.PathForRegion(my, mx)) {
            m_mapCache->UnpinFailed(*path);
          }
        }
      }
    }
  }

  struct LoadCandidate {
    std::wstring path;
    int priority = 0;
  };
  std::vector<LoadCandidate> candidates;
  candidates.reserve(128);

  auto cellDistance = [&](int mx, int my) {
    return (std::abs)(mx - centerMx) + (std::abs)(my - centerMy);
  };

  if (MapStyle == 0 && SelectedLocalMapId == 0 && SelectedDungeonFloorId == 0) {
    for (const auto &tile : m_worldTiles) {
      int mx = tile.first.first;
      int my = tile.first.second;
      if (mx + 3 < minMx || mx > maxMx || my < minMy || my - 3 > maxMy) continue;
      if (!m_mapCache->IsReady(tile.second)) {
        candidates.push_back({tile.second, cellDistance(mx + 1, my - 1)});
      }
    }
  } else if (SelectedLocalMapId > 0) {
    const LocalMapDefinition *local = FindLocalMapById(SelectedLocalMapId);
    if (local && !local->TexturePath.empty() && !m_mapCache->IsReady(local->TexturePath)) {
      candidates.push_back({local->TexturePath, 0});
    }
  } else if (SelectedDungeonFloorId > 0) {
    const InstanceMapDefinition *floor = FindInstanceMapById(SelectedDungeonFloorId);
    if (floor) {
      if (floor->IsTiled) {
        for (int my = floor->MinMy; my <= floor->MaxMy; ++my) {
          for (int mx = floor->MinMx; mx <= floor->MaxMx; ++mx) {
            std::wstring tilePath = DungeonTilePath(*floor, mx, my);
            if (!tilePath.empty() && !m_mapCache->IsReady(tilePath)) {
              candidates.push_back({tilePath, cellDistance(mx, my)});
            }
          }
        }
      } else if (!floor->TexturePath.empty() && !m_mapCache->IsReady(floor->TexturePath)) {
        candidates.push_back({floor->TexturePath, 0});
      }
    }
  } else if (MapStyle == 1) {
    int mmMinMx = std::clamp(minMx, m_minimapMinMx, m_minimapMaxMx);
    int mmMaxMx = std::clamp(maxMx, m_minimapMinMx, m_minimapMaxMx);
    int mmMinMy = std::clamp(minMy, m_minimapMinMy, m_minimapMaxMy);
    int mmMaxMy = std::clamp(maxMy, m_minimapMinMy, m_minimapMaxMy);
    for (int my = mmMinMy; my <= mmMaxMy; ++my) {
      for (int mx = mmMinMx; mx <= mmMaxMx; ++mx) {
        int rx = my, ry = mx;
        if (auto path = m_minimapIndex.PathForRegion(rx, ry)) {
          if (!m_mapCache->IsReady(*path)) {
            candidates.push_back({*path, cellDistance(mx, my)});
          }
        }
      }
    }
  } else if (MapStyle == 2) {
    const InstanceMapDefinition *inst = nullptr;
    if (SelectedInstanceId > 0) inst = FindInstanceMapById(SelectedInstanceId);
    if (!inst) inst = FindInstanceMapForRegion(CurrentRx, CurrentRy);
    if (inst && !inst->TexturePath.empty() && !m_mapCache->IsReady(inst->TexturePath)) {
      candidates.push_back({inst->TexturePath, 0});
    }
  }

  std::sort(candidates.begin(), candidates.end(),
            [](const LoadCandidate &a, const LoadCandidate &b) { return a.priority < b.priority; });

  for (const auto &candidate : candidates) {
    if (m_mapCache->LoadsThisFrame() >= m_mapCache->LoadBudget()) break;
    m_mapCache->PinVisible(candidate.path);
    const bool wasReady = m_mapCache->IsReady(candidate.path);
    Texture *tex = AcquireMapTexture(candidate.path, MapLoadPolicy::Throttled);
    if (!wasReady && tex && tex->pTexture) MarkMapRenderDirty();
  }

  auto pinVisiblePath = [&](const std::wstring &path) {
    if (!path.empty()) m_mapCache->PinVisible(path);
  };

  if (MapStyle == 0 && SelectedLocalMapId == 0 && SelectedDungeonFloorId == 0) {
    for (const auto &tile : m_worldTiles) {
      int mx = tile.first.first;
      int my = tile.first.second;
      if (mx + 3 < minMx || mx > maxMx || my < minMy || my - 3 > maxMy) continue;
      pinVisiblePath(tile.second);
    }
  } else if (SelectedLocalMapId > 0) {
    const LocalMapDefinition *local = FindLocalMapById(SelectedLocalMapId);
    if (local) pinVisiblePath(local->TexturePath);
  } else if (SelectedDungeonFloorId > 0) {
    const InstanceMapDefinition *floor = FindInstanceMapById(SelectedDungeonFloorId);
    if (floor && floor->IsTiled) {
      for (int my = floor->MinMy; my <= floor->MaxMy; ++my) {
        for (int mx = floor->MinMx; mx <= floor->MaxMx; ++mx) {
          pinVisiblePath(DungeonTilePath(*floor, mx, my));
        }
      }
    } else if (floor) {
      pinVisiblePath(floor->TexturePath);
    }
  } else if (MapStyle == 1) {
    int mmMinMx = std::clamp(minMx, m_minimapMinMx, m_minimapMaxMx);
    int mmMaxMx = std::clamp(maxMx, m_minimapMinMx, m_minimapMaxMx);
    int mmMinMy = std::clamp(minMy, m_minimapMinMy, m_minimapMaxMy);
    int mmMaxMy = std::clamp(maxMy, m_minimapMinMy, m_minimapMaxMy);
    for (int my = mmMinMy; my <= mmMaxMy; ++my) {
      for (int mx = mmMinMx; mx <= mmMaxMx; ++mx) {
        if (auto path = m_minimapIndex.PathForRegion(my, mx)) pinVisiblePath(*path);
      }
    }
  }
}

void WorldMapControl::Tick(float viewportW, float viewportH) {
  m_lastViewportW = viewportW;
  m_lastViewportH = viewportH;
  TickMinimapIndexScan(400);
  TickWorldPreload(128);
  QueueVisibleLoads(viewportW, viewportH);

  m_pendingTextureLoads = 0;
  if (!m_minimapIndexComplete) {
    m_pendingTextureLoads = 1;
    MarkMapRenderDirty();
    return;
  }
  if (!m_mapCache || viewportW <= 0 || viewportH <= 0) {
    if (!m_worldPreloadComplete) MarkMapRenderDirty();
    return;
  }

  float worldX0 = (0.0f - m_panOffset.x) / m_zoom;
  float worldX1 = (viewportW - m_panOffset.x) / m_zoom;
  float worldY0 = (0.0f - m_panOffset.y) / m_zoom;
  float worldY1 = (viewportH - m_panOffset.y) / m_zoom;
  int minMx = m_minMx + static_cast<int>(floorf(worldX0 / sro::map::kMapPixelsPerRegion)) - 1;
  int maxMx = m_minMx + static_cast<int>(floorf(worldX1 / sro::map::kMapPixelsPerRegion)) + 1;
  int maxMy = m_maxMy - static_cast<int>(floorf(worldY0 / sro::map::kMapPixelsPerRegion)) + 1;
  int minMy = m_maxMy - static_cast<int>(floorf(worldY1 / sro::map::kMapPixelsPerRegion)) - 1;

  if (MapStyle == 0 && SelectedLocalMapId == 0 && SelectedDungeonFloorId == 0) {
    for (const auto &tile : m_worldTiles) {
      int mx = tile.first.first;
      int my = tile.first.second;
      if (mx + 3 < minMx || mx > maxMx || my < minMy || my - 3 > maxMy) continue;
      if (!m_mapCache->IsReady(tile.second)) ++m_pendingTextureLoads;
    }
  } else if (SelectedLocalMapId > 0) {
    const LocalMapDefinition *local = FindLocalMapById(SelectedLocalMapId);
    if (local && !local->TexturePath.empty() && !m_mapCache->IsReady(local->TexturePath)) {
      ++m_pendingTextureLoads;
    }
  } else if (MapStyle == 1) {
    int mmMinMx = std::clamp(minMx, m_minimapMinMx, m_minimapMaxMx);
    int mmMaxMx = std::clamp(maxMx, m_minimapMinMx, m_minimapMaxMx);
    int mmMinMy = std::clamp(minMy, m_minimapMinMy, m_minimapMaxMy);
    int mmMaxMy = std::clamp(maxMy, m_minimapMinMy, m_minimapMaxMy);
    for (int my = mmMinMy; my <= mmMaxMy; ++my) {
      for (int mx = mmMinMx; mx <= mmMaxMx; ++mx) {
        if (auto path = m_minimapIndex.PathForRegion(my, mx)) {
          if (!m_mapCache->IsReady(*path)) ++m_pendingTextureLoads;
        }
      }
    }
  }

  if (!m_worldPreloadComplete && MapStyle == 0) {
    ++m_pendingTextureLoads;
  }

  if (m_minimapHudActive) {
    for (int drx = -1; drx <= 1; ++drx) {
      for (int dry = -1; dry <= 1; ++dry) {
        int rx = 0;
        int ry = 0;
        sro::map::MinimapHudNeighborRegion(drx, dry, m_hudCenterRx, m_hudCenterRy, rx, ry);
        if (rx < 0 || rx > 255 || ry < 0 || ry > 255) continue;
        if (auto path = PathForMinimapRegion(rx, ry)) {
          if (!m_mapCache->IsReady(*path)) ++m_pendingTextureLoads;
        }
      }
    }
  }

  if (m_pendingTextureLoads > 0) {
    MarkMapRenderDirty();
  }
}

void WorldMapControl::ScanWorldTiles() {
  std::wstring worldMapDir = m_clientPath + L"/Media/interface/worldmap/map";
  std::wstring searchPath = worldMapDir + L"/map_world_*.ddj";

  WIN32_FIND_DATAW fd;
  HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
  if (hFind == INVALID_HANDLE_VALUE) return;

  do {
    std::wstring filename = fd.cFileName;
    if (filename.rfind(L"map_world_", 0) == 0) {
      size_t dot = filename.find_last_of(L'.');
      if (dot != std::wstring::npos && dot > 10) {
        std::wstring coords = filename.substr(10, dot - 10);
        size_t xPos = coords.find(L'x');
        if (xPos != std::wstring::npos) {
          std::wstring mxStr = coords.substr(0, xPos);
          std::wstring myStr = coords.substr(xPos + 1);
          try {
            int mx = std::stoi(mxStr);
            int my = std::stoi(myStr);
            // Match ISRO CWorldMapView_LoadWorldTiles: mx 46..180 step 4, my 113..69 step -4.
            // Extra map_world_* files at other anchors exist in some packs but the client ignores them.
            if (mx >= sro::map::kWorldTileMinMx && mx <= sro::map::kWorldTileMaxMx &&
                ((mx - sro::map::kWorldTileMinMx) % sro::map::kWorldTileStep) == 0 &&
                my >= sro::map::kWorldTileEndMy && my <= sro::map::kWorldTileStartMy &&
                ((sro::map::kWorldTileStartMy - my) % sro::map::kWorldTileStep) == 0) {
                m_worldTiles[{mx, my}] = worldMapDir + L"/" + filename;
                for (int x = mx; x <= mx + 3; ++x) {
                  for (int y = my - 3; y <= my; ++y) {
                    m_worldCoveredRegions.insert({x, y});
                    m_worldMinMx = (std::min)(m_worldMinMx, x);
                    m_worldMaxMx = (std::max)(m_worldMaxMx, x);
                    m_worldMinMy = (std::min)(m_worldMinMy, y);
                    m_worldMaxMy = (std::max)(m_worldMaxMy, y);
                  }
                }
            }
          } catch (...) {}
        }
      }
    }
  } while (FindNextFileW(hFind, &fd));
  FindClose(hFind);
}

void WorldMapControl::ScanWorldOverlayTiles() {
  std::wstring overlayDir = m_clientPath + L"/Media/interface/worldmap/monster";
  std::wstring searchPath = overlayDir + L"/*.ddj";

  WIN32_FIND_DATAW fd;
  HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
  if (hFind == INVALID_HANDLE_VALUE) return;

  do {
    std::wstring filename = fd.cFileName;
    size_t dot = filename.find_last_of(L'.');
    if (dot == std::wstring::npos) continue;

    std::wstring stem = filename.substr(0, dot);
    size_t xPos = stem.rfind(L'x');
    size_t underscorePos = stem.rfind(L'_');
    if (xPos == std::wstring::npos || underscorePos == std::wstring::npos || underscorePos >= xPos) continue;

    try {
      int mx = std::stoi(stem.substr(underscorePos + 1, xPos - underscorePos - 1));
      int my = std::stoi(stem.substr(xPos + 1));
      m_worldOverlayTiles[{mx, my}] = overlayDir + L"/" + filename;
    } catch (...) {}
  } while (FindNextFileW(hFind, &fd));
  FindClose(hFind);
}

void WorldMapControl::LoadWorldGuideMarkers() {
  std::wstring path = m_clientPath + L"/Media/server_dep/silkroad/textdata/worldmapguidedata.txt";
  std::ifstream fs(path, std::ios::binary);
  if (!fs.is_open()) return;

  enum class Section { None, Monster };
  Section section = Section::None;

  std::string line;
  while (std::getline(fs, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::string trimmed = TrimAscii(line);
    if (trimmed.empty() || trimmed.rfind("//", 0) == 0) continue;
    if (trimmed.rfind("#section", 0) == 0) {
      section = (trimmed.find("MONSTER") != std::string::npos) ? Section::Monster : Section::None;
      continue;
    }
    if (section != Section::Monster) continue;

    std::vector<std::string> cols = SplitTabs(line);
    if (cols.size() < 15) continue;

    int service = 0, pixelX = 0, pixelY = 0, rx = 0, ry = 0, localX = 0, localY = 0;
    if (!ParseIntColumn(cols, 0, service) || service == 0) continue;
    if (!ParseIntColumn(cols, 8, pixelX) || !ParseIntColumn(cols, 9, pixelY) ||
        !ParseIntColumn(cols, 10, rx) || !ParseIntColumn(cols, 11, ry) ||
        !ParseIntColumn(cols, 12, localX) || !ParseIntColumn(cols, 13, localY)) continue;
    if (pixelX <= 0 && pixelY <= 0) continue;
    if (TrimAscii(cols[4]) != "sub") continue;

    WorldGuideMarker marker;
    marker.NameUtf8 = ToUtf8FromGameText(TrimAscii(cols[1]));
    marker.CodeName = TrimAscii(cols[2]);
    marker.MapGroup = TrimAscii(cols[3]);
    marker.PixelX = pixelX; marker.PixelY = pixelY;
    marker.Rx = rx; marker.Ry = ry;
    marker.LocalX = localX; marker.LocalY = localY;
    marker.IconToken = TrimAscii(cols[14]);
    m_worldGuideMarkers.push_back(std::move(marker));
  }
}

void WorldMapControl::ParseLocalMapDefinitionLines(std::wifstream &fs) {
  std::wstring line;
  while (std::getline(fs, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::wstring trimmed = TrimWideText(line);
    if (trimmed.empty() || trimmed.rfind(L"//", 0) == 0 || trimmed.rfind(L"#", 0) == 0) continue;

    std::vector<std::wstring> cols = SplitTabsWide(line);
    if (cols.size() < 14) continue;

    LocalMapDefinition def;
    if (!ParseIntWideColumn(cols, 0, def.Id) ||
        !ParseIntWideColumn(cols, 4, def.TextureWidth) ||
        !ParseIntWideColumn(cols, 5, def.TextureHeight) ||
        !ParseIntWideColumn(cols, 6, def.RenderWidth) ||
        !ParseIntWideColumn(cols, 7, def.RenderHeight)) continue;
    if (cols.size() > 8) ParseIntWideColumn(cols, 8, def.CoordWidth);
    if (cols.size() > 9) ParseIntWideColumn(cols, 9, def.CoordHeight);
    int mx0 = 0, my0 = 0, mx1 = 0, my1 = 0;
    if (!ParseIntWideColumn(cols, 10, mx0) || !ParseIntWideColumn(cols, 11, my0) ||
        !ParseIntWideColumn(cols, 12, mx1) || !ParseIntWideColumn(cols, 13, my1)) continue;

    def.NameUtf8 = ToUtf8FromWide(TrimWideText(cols[2]));
    std::wstring texPath = TrimWideText(cols[3]);
    def.TexturePath = texPath;
    std::replace(def.TexturePath.begin(), def.TexturePath.end(), L'\\', L'/');
    def.TexturePath = m_clientPath + L"/Media/" + def.TexturePath;
    def.MinMx = (std::min)(mx0, mx1); def.MaxMx = (std::max)(mx0, mx1);
    def.MinMy = (std::min)(my0, my1); def.MaxMy = (std::max)(my0, my1);
    def.RawMx0 = mx0; def.RawMy0 = my0; def.RawMx1 = mx1; def.RawMy1 = my1;
    if (def.RenderWidth <= 0 || def.RenderHeight <= 0) continue;
    m_localMapDefs.push_back(std::move(def));
  }
}

void WorldMapControl::LoadWorldMapDefinitions() {
  m_localMapDefs.clear();
  std::wifstream fs;
  const wchar_t *sourceFile = nullptr;
  if (TryOpenTextDataFile(L"wlocalmap.txt", fs)) {
    sourceFile = L"wlocalmap.txt";
  } else if (TryOpenTextDataFile(L"worldmap_mapinfo.txt", fs)) {
    sourceFile = L"worldmap_mapinfo.txt";
  } else {
    LogMsg("[WorldMapControl] No local map definitions file found (wlocalmap.txt / worldmap_mapinfo.txt).");
    return;
  }

  fs.imbue(std::locale(fs.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));
  ParseLocalMapDefinitionLines(fs);
  LogMsg("[WorldMapControl] Loaded " + std::to_string(m_localMapDefs.size()) +
         " local map definitions from " + ToUtf8FromWide(sourceFile) + ".");
  for (const auto &def : m_localMapDefs) {
    const int coordW = def.CoordWidth > 0 ? def.CoordWidth : def.RenderWidth;
    const int coordH = def.CoordHeight > 0 ? def.CoordHeight : def.RenderHeight;
    if (coordW != def.RenderWidth || coordH != def.RenderHeight) {
      LogMsg("[WorldMapControl] Local map id=" + std::to_string(def.Id) + " \"" + def.NameUtf8 +
             "\" render=" + std::to_string(def.RenderWidth) + "x" +
             std::to_string(def.RenderHeight) + " coord=" + std::to_string(coordW) + "x" +
             std::to_string(coordH) + " texture=" + std::to_string(def.TextureWidth) + "x" +
             std::to_string(def.TextureHeight));
    }
  }
}

bool WorldMapControl::TryOpenTextDataFile(const wchar_t* relativePath, std::wifstream& out) const {
  const std::wstring candidates[] = {
      m_clientPath + L"/Media/textdata/" + relativePath,
      m_clientPath + L"/Media/server_dep/silkroad/textdata/" + relativePath};
  for (const auto& path : candidates) {
    out.open(path, std::ios::binary);
    if (out.is_open()) return true;
    out.clear();
  }
  return false;
}

bool WorldMapControl::TryOpenAsciiTextDataFile(const wchar_t* relativePath, std::ifstream& out) const {
  const std::wstring candidates[] = {
      m_clientPath + L"/Media/textdata/" + relativePath,
      m_clientPath + L"/Media/server_dep/silkroad/textdata/" + relativePath};
  for (const auto& path : candidates) {
    out.open(path, std::ios::binary);
    if (out.is_open()) return true;
    out.clear();
  }
  return false;
}

void WorldMapControl::LoadDungeonMapDefinitions() {
  const auto path = FindTextDataFilePath(m_clientPath, L"dungeonmap.txt");
  if (!path) {
    LogMsgW(L"[WorldMapControl] WARNING: dungeonmap.txt not found under Media/textdata/ or "
            L"Media/server_dep/silkroad/textdata/");
    return;
  }

  const size_t before = m_instanceMapDefs.size();
  std::wstring encoding;
  const bool utf16Bom = FileHasUtf16LeBom(*path);

  if (utf16Bom) {
    std::wifstream wfs(*path, std::ios::binary);
    if (wfs.is_open()) {
      wfs.imbue(std::locale(wfs.getloc(),
                            new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));
      std::wstring line;
      while (std::getline(wfs, line)) {
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        std::wstring trimmed = TrimWideText(line);
        if (trimmed.empty() || trimmed.rfind(L"//", 0) == 0 || trimmed.rfind(L"#", 0) == 0) {
          continue;
        }

        std::vector<std::wstring> wcols = SplitTabsWide(line);
        std::vector<std::string> cols;
        cols.reserve(wcols.size());
        for (const auto &col : wcols) {
          cols.push_back(ToUtf8FromGameText(ToUtf8FromWide(TrimWideText(col))));
        }
        AppendDungeonMapDefinitionFromCols(cols, m_clientPath, m_instanceMapDefs);
      }
      if (m_instanceMapDefs.size() > before) encoding = L"UTF-16";
    }
  }

  if (m_instanceMapDefs.size() == before) {
    std::ifstream fs(*path, std::ios::binary);
    if (fs.is_open()) {
      std::string line;
      while (std::getline(fs, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string trimmed = TrimAscii(line);
        if (trimmed.empty() || trimmed.rfind("//", 0) == 0 || trimmed.rfind("#", 0) == 0) {
          continue;
        }
        AppendDungeonMapDefinitionFromCols(SplitTabs(line), m_clientPath, m_instanceMapDefs);
      }
      if (m_instanceMapDefs.size() > before) encoding = utf16Bom ? L"UTF-8" : L"ASCII/UTF-8";
    }
  }

  const size_t loaded = m_instanceMapDefs.size() - before;
  if (loaded == 0) {
    LogMsgW(L"[WorldMapControl] WARNING: dungeonmap.txt found at " + *path +
            L" but no valid floor rows were parsed.");
    return;
  }

  LogMsgW(L"[WorldMapControl] Loaded " + std::to_wstring(loaded) +
          L" dungeon map definitions from dungeonmap.txt (" + encoding + L").");
}

void WorldMapControl::LoadInstanceMapDefinitions() {
  std::wifstream fs;
  if (!TryOpenTextDataFile(L"worldmap_instanceinfo.txt", fs)) return;
  fs.imbue(std::locale(fs.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));

  const size_t before = m_instanceMapDefs.size();
  std::wstring line;
  while (std::getline(fs, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::wstring trimmed = TrimWideText(line);
    if (trimmed.empty() || trimmed.rfind(L"//", 0) == 0 || trimmed.rfind(L"#", 0) == 0) continue;

    std::vector<std::wstring> cols = SplitTabsWide(line);
    if (cols.size() < 14) continue;

    InstanceMapDefinition def;
    if (!ParseIntWideColumn(cols, 0, def.Id) ||
        !ParseIntWideColumn(cols, 4, def.TextureWidth) ||
        !ParseIntWideColumn(cols, 5, def.TextureHeight) ||
        !ParseIntWideColumn(cols, 6, def.RenderWidth) ||
        !ParseIntWideColumn(cols, 7, def.RenderHeight)) continue;
    int mx0 = 0, my0 = 0, mx1 = 0, my1 = 0;
    if (!ParseIntWideColumn(cols, 10, mx0) || !ParseIntWideColumn(cols, 11, my0) ||
        !ParseIntWideColumn(cols, 12, mx1) || !ParseIntWideColumn(cols, 13, my1)) continue;

    def.NameUtf8 = ToUtf8FromWide(TrimWideText(cols[2]));
    std::wstring texPath = TrimWideText(cols[3]);
    def.TexturePath = texPath;
    std::replace(def.TexturePath.begin(), def.TexturePath.end(), L'\\', L'/');
    def.TexturePath = m_clientPath + L"/Media/" + def.TexturePath;
    def.MinMx = (std::min)(mx0, mx1);
    def.MaxMx = (std::max)(mx0, mx1);
    def.MinMy = (std::min)(my0, my1);
    def.MaxMy = (std::max)(my0, my1);
    if (cols.size() > 15) ParseIntWideColumn(cols, 14, def.RegionRx);
    if (cols.size() > 16) ParseIntWideColumn(cols, 15, def.RegionRy);
    if (cols.size() > 17) def.DungeonFolder = TrimWideText(cols[16]);
    if (cols.size() > 18) def.TilePrefix = TrimWideText(cols[17]);
    if (def.RenderWidth <= 0 || def.RenderHeight <= 0) continue;
    m_instanceMapDefs.push_back(std::move(def));
  }
  const size_t loaded = m_instanceMapDefs.size() - before;
  if (loaded == 0) {
    LogMsg("[WorldMapControl] worldmap_instanceinfo.txt found but no instance rows parsed "
           "(column 0 must be numeric; Client_Rigid file may be wlocalmap data).");
  } else {
    LogMsg("[WorldMapControl] Loaded " + std::to_string(loaded) + " instance map definitions.");
  }
}

void WorldMapControl::LoadTranslations() {
  std::wstring dirPath = m_clientPath + L"/Media/server_dep/silkroad/textdata";
  if (!std::filesystem::exists(dirPath)) return;

  LogMsgW(L"[WorldMapControl] Loading translations from textdata...");
  int fileCount = 0;

  static const std::vector<std::wstring> prefixes = {
      L"textdata_object", L"textdata_equip&skill", L"textuisystem",
      L"textquest_speech&name", L"textquest_queststring", L"textzonename",
      L"textquest_otherstring", L"texthelp", L"textevent"};

  for (const auto &entry : std::filesystem::directory_iterator(dirPath)) {
    if (!entry.is_regular_file()) continue;
    std::wstring filename = entry.path().filename().wstring();

    bool isTarget = false;
    if (filename.size() >= 4 && filename.substr(filename.size() - 4) == L".txt") {
      std::wstring nameWithoutExt = filename.substr(0, filename.size() - 4);
      for (const auto &prefix : prefixes) {
        if (nameWithoutExt == prefix || (nameWithoutExt.rfind(prefix + L"_", 0) == 0)) {
          isTarget = true;
          break;
        }
      }
    }

    if (isTarget) {
      std::wifstream fs(entry.path(), std::ios::binary);
      if (!fs.is_open()) continue;
      fs.imbue(std::locale(fs.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));
      fileCount++;

      std::wstring line;
      while (std::getline(fs, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == L'/' || line[0] == L'#') continue;
        std::vector<std::wstring> cols = SplitTabsWide(line);
        if (cols.size() > 9) {
          std::wstring key = TrimWideText(cols[2]);
          std::wstring korean = TrimWideText(cols[3]);
          std::wstring english = CleanTranslation(TrimWideText(cols[9]));
          if (!english.empty() && english != L"0" && english != L"xxx") {
            if (!key.empty()) m_translations[StripSpaces(key)] = english;
            if (!korean.empty()) m_translations[StripSpaces(korean)] = english;
          }
          if (filename.rfind(L"textzonename", 0) == 0) {
            try {
              int regionId = std::stoi(key);
              std::string koName = ToUtf8FromWide(korean);
              std::string enName = ToUtf8FromWide(english);
              if (regionId > 0 && !koName.empty() && koName != "0" && koName != "xxx") {
                m_regionNames[regionId] = {koName, enName};
              }
            } catch (...) {}
          }
        }
      }
    }
  }
  LogMsgW(L"[WorldMapControl] Loaded " + std::to_wstring(m_translations.size()) +
          L" translation keys from " + std::to_wstring(fileCount) + L" files.");
}

void WorldMapControl::LoadLocalPoiMarkers() {
  std::wifstream fs;
  if (!TryOpenTextDataFile(L"worldmap_localinfo.txt", fs)) return;
  fs.imbue(std::locale(fs.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));

  size_t worldMarkerCount = 0, localMarkerCount = 0, texturedMarkerCount = 0;
  std::wstring line;
  while (std::getline(fs, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::wstring trimmed = TrimWideText(line);
    if (trimmed.empty() || trimmed.rfind(L"//", 0) == 0 || trimmed.rfind(L"#", 0) == 0) continue;

    std::vector<std::wstring> cols = SplitTabsWide(line);
    if (cols.size() < 13) continue;

    LocalPoiMarker poi;
    if (!ParseIntWideColumn(cols, 8, poi.MapId)) continue;
    int probeA = -1, probeB = -1, probeC = -1, probeD = -1;
    ParseIntWideColumn(cols, 9, probeA);
    ParseIntWideColumn(cols, 10, probeB);
    ParseIntWideColumn(cols, 11, probeC);
    ParseIntWideColumn(cols, 12, probeD);
    int kind = 0;
    ParseIntWideColumn(cols, 2, kind);
    poi.MarkerKind = kind;
    poi.ZoneCode = ToUtf8FromWide(TrimWideText(cols[3]));

    std::wstring zoneWide = StripSpaces(TrimWideText(cols[3]));
    std::wstring areaWide = TrimWideText(cols[4]);
    std::wstring nameWide = TrimWideText(cols[5]);

    poi.NameKoUtf8 = ToUtf8FromWide(nameWide);
    poi.AreaKoUtf8 = ToUtf8FromWide(areaWide);

    std::wstring extraKey;
    if (cols.size() > 21) {
      std::wstring lastCol = TrimWideText(cols.back());
      size_t comma = lastCol.find(L',');
      extraKey = StripSpaces(comma != std::wstring::npos ? lastCol.substr(0, comma) : lastCol);
    }

    std::wstring areaWideStripped = StripSpaces(areaWide);
    std::wstring nameWideStripped = StripSpaces(nameWide);

    bool translated = false;
    if (extraKey.rfind(L"SN_", 0) == 0) {
      auto itE = m_translations.find(extraKey);
      if (itE != m_translations.end()) { nameWide = itE->second; nameWideStripped = StripSpaces(nameWide); translated = true; }
    }
    if (!translated && zoneWide.rfind(L"SN_", 0) == 0) {
      auto itZ = m_translations.find(zoneWide);
      if (itZ != m_translations.end()) { nameWide = itZ->second; nameWideStripped = StripSpaces(nameWide); translated = true; }
    }
    if (!translated) {
      if (nameWide.empty() && !zoneWide.empty()) {
        auto itZ = m_translations.find(zoneWide);
        if (itZ != m_translations.end()) { nameWide = itZ->second; nameWideStripped = StripSpaces(nameWide); translated = true; }
      }
      if (!translated && nameWide.empty() && !extraKey.empty()) {
        auto itE = m_translations.find(extraKey);
        if (itE != m_translations.end()) { nameWide = itE->second; nameWideStripped = StripSpaces(nameWide); translated = true; }
      }
    }

    auto itA = m_translations.find(areaWideStripped);
    if (itA != m_translations.end()) areaWide = itA->second;
    auto itN = m_translations.find(nameWideStripped);
    if (itN != m_translations.end()) nameWide = itN->second;

    poi.AreaNameUtf8 = ToUtf8FromWide(areaWide);
    poi.NameUtf8 = ToUtf8FromWide(nameWide);

    if (poi.MapId > 0 && poi.MarkerKind == 2) {
      if (cols.size() > 6) {
        std::wstring teleportWide = TrimWideText(cols[6]);
        if (!teleportWide.empty() && teleportWide != L"0" && teleportWide != L"xxx") {
          poi.TeleportLabelUtf8 = ToUtf8FromWide(teleportWide);
        }
      }
      if (cols.size() > 21) {
        const std::wstring storeKey = StripSpaces(TrimWideText(cols[21]));
        if (!storeKey.empty()) {
          auto itStore = m_translations.find(storeKey);
          if (itStore != m_translations.end() && !itStore->second.empty()) {
            poi.TeleportLabelUtf8 = ToUtf8FromWide(itStore->second);
          }
        }
      }
    }

    if (poi.MapId > 0 && poi.MarkerKind == 1) {
      const std::wstring col3Key = StripSpaces(TrimWideText(cols[3]));
      if (col3Key.rfind(L"SN_", 0) == 0 || col3Key.rfind(L"STORE_", 0) == 0) {
        auto itRoom = m_translations.find(col3Key);
        if (itRoom != m_translations.end() && !itRoom->second.empty()) {
          poi.NameUtf8 = ToUtf8FromWide(itRoom->second);
        }
      }
    }

    std::wstring texPath = TrimWideText(cols[3]);
    if (texPath.find(L'\\') != std::wstring::npos || texPath.find(L'/') != std::wstring::npos) {
      poi.ZoneCode.clear();
      poi.TexturePath = texPath;
      std::replace(poi.TexturePath.begin(), poi.TexturePath.end(), L'\\', L'/');
      poi.TexturePath = m_clientPath + L"/Media/" + poi.TexturePath;
    }

    ParseIntWideColumn(cols, 9, poi.TargetRy);
    ParseIntWideColumn(cols, 10, poi.TargetRx);
    ParseIntWideColumn(cols, 11, poi.PixelX);
    ParseIntWideColumn(cols, 12, poi.PixelY);
    ParseIntWideColumn(cols, 13, poi.IconWidth);
    ParseIntWideColumn(cols, 14, poi.IconHeight);

    if (cols.size() > 17) {
      // Tab-separated cols 15–17 are R, G, B (not packed BGR DWORD).
      ParseIntWideColumn(cols, 15, poi.ColorR);
      ParseIntWideColumn(cols, 16, poi.ColorG);
      ParseIntWideColumn(cols, 17, poi.ColorB);
    }
    if (cols.size() > 19) {
      ParseIntWideColumn(cols, 19, poi.IsStrong);
    }

    if (poi.PixelX < 0 || poi.PixelY < 0) continue;
    if (!poi.TexturePath.empty()) ++texturedMarkerCount;
    if (cols.size() > 7) {
      int linkId = 0;
      if (ParseIntWideColumn(cols, 7, linkId) && linkId > 0) {
        poi.LinkedDungeonId = linkId;
      }
    }
    if (poi.MapId == 0) ++worldMarkerCount; else ++localMarkerCount;
    m_localPoiMarkers.push_back(std::move(poi));
  }

  LogMsg("[WorldMapControl] Loaded " + std::to_string(m_localPoiMarkers.size()) +
         " POI markers. world=" + std::to_string(worldMarkerCount) +
         " local=" + std::to_string(localMarkerCount) +
         " textured=" + std::to_string(texturedMarkerCount));
}

void WorldMapControl::LinkDungeonEntrances() {
  size_t caveEntrances = 0;
  std::set<int> missingDungeonIds;
  for (const auto &poi : m_localPoiMarkers) {
    if (!IsDungeonEntrancePoi(poi)) continue;
    ++caveEntrances;
    bool linked = false;
    for (auto &def : m_instanceMapDefs) {
      if (def.Id == poi.LinkedDungeonId) {
        def.RegionRx = poi.TargetRx;
        def.RegionRy = poi.TargetRy;
        linked = true;
      }
    }
    if (!linked) missingDungeonIds.insert(poi.LinkedDungeonId);
  }
  for (int dungeonId : missingDungeonIds) {
    LogMsg("[WorldMapControl] WARNING: cave entrance links to dungeon id " +
           std::to_string(dungeonId) + " but no floor definition was loaded.");
  }
  LogMsg("[WorldMapControl] Dungeon metadata: " + std::to_string(m_instanceMapDefs.size()) +
         " floor defs, " + std::to_string(caveEntrances) + " cave entrances with LinkedDungeonId.");
}

const WorldMapControl::LocalMapDefinition *WorldMapControl::FindLocalMapForRegion(int rx, int ry) const {
  const LocalMapDefinition *best = nullptr;
  int bestArea = INT_MAX;
  for (const auto &def : m_localMapDefs) {
    if (ry < def.MinMx || ry > def.MaxMx || rx < def.MinMy || rx > def.MaxMy) continue;
    int area = (def.MaxMx - def.MinMx + 1) * (def.MaxMy - def.MinMy + 1);
    if (area < bestArea) { best = &def; bestArea = area; }
  }
  return best;
}

const WorldMapControl::LocalMapDefinition *WorldMapControl::FindLocalMapById(int id) const {
  for (const auto &def : m_localMapDefs) {
    if (def.Id == id) return &def;
  }
  return nullptr;
}

std::vector<const WorldMapControl::LocalMapDefinition *>
WorldMapControl::FindLocalMapsInViewport(int minMx, int maxMx, int minMy, int maxMy) const {
  std::vector<const LocalMapDefinition *> result;
  for (const auto &def : m_localMapDefs) {
    if (def.MaxMx < minMx || def.MinMx > maxMx || def.MaxMy < minMy || def.MinMy > maxMy) continue;
    result.push_back(&def);
  }
  return result;
}

const WorldMapControl::LocalMapDefinition *WorldMapControl::FindLocalMapForPoi(const LocalPoiMarker &poi) const {
  if (poi.MapId != 0 || poi.TargetRx < 0 || poi.TargetRy < 0) return nullptr;
  return FindLocalMapForRegion(poi.TargetRx, poi.TargetRy);
}

bool WorldMapControl::IsCityOrFortressPoi(const LocalPoiMarker &poi) {
  if (poi.TexturePath.empty()) return false;
  return poi.TexturePath.find(L"city_") != std::wstring::npos ||
         poi.TexturePath.find(L"fort_") != std::wstring::npos;
}

bool WorldMapControl::IsDungeonEntrancePoi(const LocalPoiMarker &poi) {
  if (poi.MapId != 0 || poi.LinkedDungeonId <= 0 || poi.TargetRx < 0 || poi.TargetRy < 0) return false;
  if (poi.TexturePath.find(L"xy_dungeon") != std::wstring::npos) return true;
  return poi.MarkerKind == 2 && poi.LinkedDungeonId > 0;
}

namespace {

bool IsFortressLabel(const std::string &label) {
  return label.find("Fortress") != std::string::npos ||
         label.find("\xec\x9a\x94\xec\x83\x88") != std::string::npos;
}

bool IsCityOrFortressTexture(const WorldMapControl::LocalPoiMarker &poi) {
  if (poi.TexturePath.empty()) return false;
  return poi.TexturePath.find(L"city_") != std::wstring::npos ||
         poi.TexturePath.find(L"fort_") != std::wstring::npos;
}

ImU32 PoiFileColorToImU32(int colorR, int colorG, int colorB) {
  return IM_COL32(colorR, colorG, colorB, 255);
}

ImU32 ResolveInnerCityLabelColor(const WorldMapControl::LocalPoiMarker &poi,
                                 const std::string &label) {
  if (IsCityOrFortressTexture(poi)) {
    return IsFortressLabel(label) ? IM_COL32(255, 225, 80, 255) : IM_COL32(255, 247, 182, 255);
  }
  if (poi.MarkerKind == 1) {
    return IM_COL32(255, 247, 182, 255);
  }
  if (!poi.TexturePath.empty()) {
    return IM_COL32(245, 245, 230, 255);
  }
  if (poi.ColorR == 255 && poi.ColorG == 255 && poi.ColorB == 255 && IsFortressLabel(label)) {
    return IM_COL32(255, 225, 80, 255);
  }
  if (poi.ColorR == 0 && poi.ColorG == 0 && poi.ColorB == 0) {
    return IM_COL32(245, 245, 230, 255);
  }
  return PoiFileColorToImU32(poi.ColorR, poi.ColorG, poi.ColorB);
}

bool ShouldDrawInnerCityIconPass(const WorldMapControl::LocalPoiMarker &poi) {
  return !(poi.MarkerKind == 1 && poi.TexturePath.empty());
}

std::string InnerCityDisplayName(const WorldMapControl::LocalPoiMarker &poi) {
  if (!poi.NameUtf8.empty()) return poi.NameUtf8;
  return poi.AreaNameUtf8;
}

std::string DungeonPoiHoverTooltip(const WorldMapControl::LocalPoiMarker &poi) {
  if (!poi.TeleportLabelUtf8.empty()) return poi.TeleportLabelUtf8;
  return InnerCityDisplayName(poi);
}

bool ShouldDrawDungeonPermanentLabel(const WorldMapControl::LocalPoiMarker &poi,
                                     const std::string &label,
                                     const std::string &dungeonDisplayName) {
  if (label.empty() || poi.MarkerKind != 1) return false;
  if (!poi.TeleportLabelUtf8.empty()) return false;
  if (!dungeonDisplayName.empty() && label == dungeonDisplayName) return false;
  return sro::map::ShouldDrawInnerCityPermanentLabel(poi.MarkerKind, label, dungeonDisplayName);
}

bool ShouldDrawInnerCityLabel(const WorldMapControl::LocalPoiMarker &poi, const std::string &label,
                              const std::string &localMapNameUtf8) {
  return sro::map::ShouldDrawInnerCityPermanentLabel(poi.MarkerKind, label, localMapNameUtf8);
}

} // namespace

sro::map::LocalMapWorldRect WorldMapControl::ComputeLocalMapWorldRect(
    const LocalMapDefinition &local) const {
  const float x0 = MapToWorldX(static_cast<float>(local.MinMx));
  const float y0 = MapToWorldY(static_cast<float>(local.MaxMy));
  return sro::map::LocalMapWorldRectFromCellBounds(local.MinMx, local.MaxMx, local.RenderWidth,
                                                   local.RenderHeight, x0, y0);
}

sro::map::LocalMapWorldRect WorldMapControl::ComputeLocalMapGridWorldRect(
    const LocalMapDefinition &local) const {
  const float x0 = MapToWorldX(static_cast<float>(local.MinMx));
  const float y0 = MapToWorldY(static_cast<float>(local.MaxMy));
  return sro::map::LocalMapWorldRectGridAligned(local.MinMx, local.MaxMx, local.MinMy, local.MaxMy,
                                                x0, y0);
}

void WorldMapControl::FitLocalMapInViewport(const LocalMapDefinition &local, float viewportW, float viewportH) {
  const sro::map::LocalMapWorldRect rect = ComputeLocalMapWorldRect(local);
  float fitZoom = (std::min)(viewportW / rect.w, viewportH / rect.h) * 0.95f;
  m_zoom = std::clamp(fitZoom, 0.12f, 7.0f);
  float centerX = rect.x0 + rect.w * 0.5f;
  float centerY = rect.y0 + rect.h * 0.5f;
  m_panOffset.x = viewportW * 0.5f - centerX * m_zoom;
  m_panOffset.y = viewportH * 0.5f - centerY * m_zoom;
  MarkMapRenderDirty();
}

void WorldMapControl::OpenLocalMap(int mapId, float viewportW, float viewportH) {
  const LocalMapDefinition *local = FindLocalMapById(mapId);
  if (!local) return;

  CloseDungeonMap();

  if (!m_hasSavedWorldView) {
    m_savedPanOffset = m_panOffset;
    m_savedZoom = m_zoom;
    m_hasSavedWorldView = true;
  }

  SelectedLocalMapId = mapId;
  ApplyBoundsForCurrentStyle();
  FitLocalMapInViewport(*local, viewportW, viewportH);
}

void WorldMapControl::CloseLocalMap() {
  if (SelectedLocalMapId == 0) return;
  SelectedLocalMapId = 0;
  if (m_hasSavedWorldView && SelectedDungeonFloorId == 0) {
    m_panOffset = m_savedPanOffset;
    m_zoom = m_savedZoom;
    m_hasSavedWorldView = false;
  }
  ApplyBoundsForCurrentStyle();
  MarkMapRenderDirty();
}

sro::map::LocalMapWorldRect WorldMapControl::ComputeDungeonFloorWorldRect(
    const InstanceMapDefinition &floor) const {
  return sro::map::DungeonFloorWorldRectFromRender(floor.MinMx, floor.MaxMy, floor.RenderWidth,
                                                   floor.RenderHeight);
}

void WorldMapControl::FitDungeonFloorInViewport(const InstanceMapDefinition &floor, float viewportW,
                                                float viewportH) {
  const sro::map::LocalMapWorldRect rect = ComputeDungeonFloorWorldRect(floor);
  float fitZoom = (std::min)(viewportW / rect.w, viewportH / rect.h) * 0.95f;
  m_zoom = std::clamp(fitZoom, 0.12f, 7.0f);
  float centerX = rect.x0 + rect.w * 0.5f;
  float centerY = rect.y0 + rect.h * 0.5f;
  m_panOffset.x = viewportW * 0.5f - centerX * m_zoom;
  m_panOffset.y = viewportH * 0.5f - centerY * m_zoom;
  MarkMapRenderDirty();
}

std::wstring WorldMapControl::DungeonTilePath(const InstanceMapDefinition &floor, int mx,
                                              int my) const {
  if (!floor.IsTiled || floor.TilePrefixPath.empty()) return {};
  return floor.TilePrefixPath + std::to_wstring(mx) + L"x" + std::to_wstring(my) + L".ddj";
}

bool WorldMapControl::OpenDungeonMap(int floorId, float viewportW, float viewportH) {
  const InstanceMapDefinition *floor = FindInstanceMapById(floorId);
  if (!floor) {
    LogMsg("[WorldMapControl] ERROR: Cannot open dungeon floor " + std::to_string(floorId) +
           " — dungeon map data not loaded (check dungeonmap.txt and restart editor after rebuild).");
    return false;
  }

  CloseLocalMap();

  if (!m_hasSavedWorldView) {
    m_savedPanOffset = m_panOffset;
    m_savedZoom = m_zoom;
    m_hasSavedWorldView = true;
  }

  SelectedDungeonFloorId = floorId;
  ApplyBoundsForCurrentStyle();
  FitDungeonFloorInViewport(*floor, viewportW, viewportH);

  const std::vector<int> floors = FindDungeonFloorIds(floorId);
  LogMsg("[WorldMapControl] Dungeon floor group: " + std::to_string(floors.size()) + " floors");
  return true;
}

void WorldMapControl::CloseDungeonMap() {
  if (SelectedDungeonFloorId == 0) return;
  SelectedDungeonFloorId = 0;
  if (m_hasSavedWorldView && SelectedLocalMapId == 0) {
    m_panOffset = m_savedPanOffset;
    m_zoom = m_savedZoom;
    m_hasSavedWorldView = false;
  }
  ApplyBoundsForCurrentStyle();
  MarkMapRenderDirty();
}

void WorldMapControl::SetDungeonFloor(int floorId, float viewportW, float viewportH) {
  const InstanceMapDefinition *floor = FindInstanceMapById(floorId);
  if (!floor || SelectedDungeonFloorId == 0) return;
  SelectedDungeonFloorId = floorId;
  ApplyBoundsForCurrentStyle();
  FitDungeonFloorInViewport(*floor, viewportW, viewportH);
}

bool WorldMapControl::CycleDungeonFloor(int direction, float viewportW, float viewportH) {
  if (SelectedDungeonFloorId == 0 || direction == 0) return false;

  const std::vector<int> floors = FindDungeonFloorIds(SelectedDungeonFloorId);
  if (floors.size() <= 1) {
    LogMsg("[WorldMapControl] WARNING: Dungeon floor group has " + std::to_string(floors.size()) +
           " floors — check dungeonmap.txt grouping");
    return false;
  }

  size_t currentIndex = floors.size();
  for (size_t i = 0; i < floors.size(); ++i) {
    if (floors[i] == SelectedDungeonFloorId) {
      currentIndex = i;
      break;
    }
  }
  if (currentIndex >= floors.size()) return false;

  const int nextIndex = static_cast<int>(currentIndex) + (direction > 0 ? 1 : -1);
  if (nextIndex < 0 || static_cast<size_t>(nextIndex) >= floors.size()) return false;

  const int nextFloorId = floors[static_cast<size_t>(nextIndex)];
  SetDungeonFloor(nextFloorId, viewportW, viewportH);

  std::string msg = "Switched to dungeon floor " + std::to_string(nextFloorId);
  const std::string floorLabel = GetDungeonFloorLabel(nextFloorId);
  if (!floorLabel.empty()) msg += " (" + floorLabel + ")";
  LogMsg("[WorldMapControl] " + msg);
  return true;
}

std::vector<int> WorldMapControl::FindDungeonFloorIds(int firstFloorId) const {
  std::vector<sro::map::DungeonFloorGroupInput> rows;
  rows.reserve(m_instanceMapDefs.size());
  for (const auto &def : m_instanceMapDefs) {
    sro::map::DungeonFloorGroupInput row;
    row.id = def.Id;
    row.nameUtf8 = def.NameUtf8;
    row.groupKey = def.DungeonGroupKey;
    row.floorIndex = def.FloorIndex;
    row.floorCount = def.FloorCount;
    rows.push_back(std::move(row));
  }
  return sro::map::GroupDungeonFloorIds(rows, firstFloorId);
}

std::string WorldMapControl::ResolveDungeonDisplayName(const InstanceMapDefinition &floor) const {
  if (!floor.NameUtf8.empty()) {
    const std::wstring nameWide = Utf8ToWide(floor.NameUtf8);
    if (!nameWide.empty()) {
      auto it = m_translations.find(StripSpaces(nameWide));
      if (it != m_translations.end() && !it->second.empty()) {
        return ToUtf8FromWide(it->second);
      }
    }
    return floor.NameUtf8;
  }
  if (!floor.DungeonGroupKey.empty()) return floor.DungeonGroupKey;
  return "Dungeon";
}

std::string WorldMapControl::GetDungeonFloorLabel(int floorId) const {
  const InstanceMapDefinition *floor = FindInstanceMapById(floorId);
  if (!floor || floor->FloorIndex <= 0) return {};
  return sro::map::FormatFloorLabel(floor->FloorKind, floor->FloorIndex);
}

std::string WorldMapControl::GetDungeonViewTitle() const {
  const InstanceMapDefinition *floor = FindInstanceMapById(SelectedDungeonFloorId);
  if (!floor) return "Dungeon Map";
  const std::string floorLabel = GetDungeonFloorLabel(SelectedDungeonFloorId);
  const std::string name = ResolveDungeonDisplayName(*floor);
  if (floor->FloorKind == 'B' && floor->FloorIndex > 0) {
    return "Underground Level " + std::to_string(floor->FloorIndex) + " of " + name;
  }
  if (!floorLabel.empty()) {
    return name + " (" + floorLabel + ")";
  }
  return name;
}

std::string WorldMapControl::GetInstanceComboLabel(const InstanceMapDefinition &inst) const {
  std::string label = ResolveDungeonDisplayName(inst);
  const std::string floor = GetDungeonFloorLabel(inst.Id);
  if (!floor.empty()) label += " (" + floor + ")";
  if (label.empty()) label = "ID " + std::to_string(inst.Id);
  return label;
}

std::optional<int> WorldMapControl::HitTestWorldPoi(const Vector2 &screenPos, float width, float height) const {
  if (SelectedLocalMapId > 0 || SelectedDungeonFloorId > 0) return std::nullopt;
  if (MapStyle != 0 && MapStyle != 1) return std::nullopt;

  const float worldMonsterIconSize = (std::clamp)(12.0f * m_zoom, 12.0f, 20.0f);
  for (const auto &poi : m_localPoiMarkers) {
    if (poi.MapId != 0 || poi.TargetRx < 0 || poi.TargetRy < 0) continue;
    if (!IsCityOrFortressPoi(poi)) continue;
    if (poi.PixelX < 0 || poi.PixelX >= 256 || poi.PixelY < 0 || poi.PixelY >= 256) continue;
    if (!FindLocalMapForPoi(poi)) continue;

    float worldX = MapToWorldX(static_cast<float>(poi.TargetRy)) + static_cast<float>(poi.PixelX) +
                   static_cast<float>(poi.IconWidth) * 0.5f;
    float worldY = MapToWorldY(static_cast<float>(poi.TargetRx)) + static_cast<float>(poi.PixelY) +
                   static_cast<float>(poi.IconHeight) * 0.5f;
    float screenX = m_panOffset.x + worldX * m_zoom;
    float screenY = m_panOffset.y + worldY * m_zoom;
    float iconW = poi.IconWidth > 0 ? static_cast<float>(poi.IconWidth) * m_zoom : worldMonsterIconSize;
    float iconH = poi.IconHeight > 0 ? static_cast<float>(poi.IconHeight) * m_zoom : worldMonsterIconSize;
    if (screenPos.x >= screenX - iconW * 0.5f && screenPos.x <= screenX + iconW * 0.5f &&
        screenPos.y >= screenY - iconH * 0.5f && screenPos.y <= screenY + iconH * 0.5f) {
      const LocalMapDefinition *local = FindLocalMapForPoi(poi);
      if (local) return local->Id;
    }
  }
  return std::nullopt;
}

std::optional<int> WorldMapControl::HitTestDungeonEntrance(const Vector2 &screenPos, float width,
                                                           float height) const {
  if (SelectedLocalMapId > 0 || SelectedDungeonFloorId > 0) return std::nullopt;
  if (MapStyle != 0 && MapStyle != 1) return std::nullopt;

  const float worldMonsterIconSize = (std::clamp)(12.0f * m_zoom, 12.0f, 20.0f);
  for (const auto &poi : m_localPoiMarkers) {
    if (!IsDungeonEntrancePoi(poi)) continue;
    if (poi.PixelX < 0 || poi.PixelX >= 256 || poi.PixelY < 0 || poi.PixelY >= 256) continue;

    float worldX = MapToWorldX(static_cast<float>(poi.TargetRy)) + static_cast<float>(poi.PixelX) +
                   static_cast<float>(poi.IconWidth) * 0.5f;
    float worldY = MapToWorldY(static_cast<float>(poi.TargetRx)) + static_cast<float>(poi.PixelY) +
                   static_cast<float>(poi.IconHeight) * 0.5f;
    float screenX = m_panOffset.x + worldX * m_zoom;
    float screenY = m_panOffset.y + worldY * m_zoom;
    float iconW = poi.IconWidth > 0 ? static_cast<float>(poi.IconWidth) * m_zoom : worldMonsterIconSize;
    float iconH = poi.IconHeight > 0 ? static_cast<float>(poi.IconHeight) * m_zoom : worldMonsterIconSize;
    if (screenPos.x >= screenX - iconW * 0.5f && screenPos.x <= screenX + iconW * 0.5f &&
        screenPos.y >= screenY - iconH * 0.5f && screenPos.y <= screenY + iconH * 0.5f) {
      return poi.LinkedDungeonId;
    }
  }
  return std::nullopt;
}

bool WorldMapControl::HitTestAnyWorldMapPoi(const Vector2 &screenPos, float width, float height) const {
  return HitTestWorldPoi(screenPos, width, height).has_value() ||
         HitTestDungeonEntrance(screenPos, width, height).has_value();
}

const WorldMapControl::InstanceMapDefinition *WorldMapControl::FindInstanceMapForRegion(int rx, int ry) const {
  const InstanceMapDefinition *best = nullptr;
  int bestArea = INT_MAX;
  for (const auto &def : m_instanceMapDefs) {
    if (def.RegionRx > 0 && def.RegionRy > 0 && (def.RegionRx != rx || def.RegionRy != ry)) continue;
    if (ry < def.MinMx || ry > def.MaxMx || rx < def.MinMy || rx > def.MaxMy) continue;
    int area = (def.MaxMx - def.MinMx + 1) * (def.MaxMy - def.MinMy + 1);
    if (area < bestArea) { best = &def; bestArea = area; }
  }
  return best;
}

const WorldMapControl::InstanceMapDefinition *WorldMapControl::FindInstanceMapById(int id) const {
  for (const auto &def : m_instanceMapDefs) {
    if (def.Id == id) return &def;
  }
  return nullptr;
}

Texture *WorldMapControl::GetColleagueTexture(int type) const {
  switch (type) {
    case 0: return m_partyMarkerTex ? m_partyMarkerTex : m_localPoiIconTex;
    case 1: return m_questNpcMarkerTex ? m_questNpcMarkerTex : m_localPoiIconTex;
    default: return m_localPoiIconTex;
  }
}

bool WorldMapControl::IsRegionActive(int rx, int ry) const {
  if (ActiveRegions.empty()) return true;
  return ActiveRegions.count({rx, ry}) != 0;
}

bool WorldMapControl::IsTileRegionActive(int mx, int my) const {
  if (ActiveRegions.empty()) return true;
  int rx = my, ry = mx;
  return IsRegionActive(rx, ry);
}

size_t WorldMapControl::CountActiveWorldTiles() const {
  size_t count = 0;
  for (const auto &tile : m_worldTiles) {
    int mx = tile.first.first;
    int my = tile.first.second;
    bool tileActive = false;
    for (int x = mx; x <= mx + 3 && !tileActive; ++x) {
      for (int y = my - 3; y <= my; ++y) {
        if (IsTileRegionActive(x, y)) { tileActive = true; break; }
      }
    }
    if (tileActive) ++count;
  }
  return count;
}

void WorldMapControl::SetPlayerPosition(int rx, int ry, float localX, float localZ, float headingRad) {
  m_hasPlayerPosition = true;
  m_playerRx = rx;
  m_playerRy = ry;
  m_playerLocalX = localX;
  m_playerLocalZ = localZ;
  m_playerHeadingRad = headingRad;
}

void WorldMapControl::ClearPlayerPosition() { m_hasPlayerPosition = false; }

bool WorldMapControl::IsPoiTextureLoaded(const LocalPoiMarker &marker) const {
  if (marker.TexturePath.empty()) return false;
  Texture *tex = m_texManager->GetTexture(marker.TexturePath, false);
  return tex && tex->pTexture;
}

Texture *WorldMapControl::GetMarkerTexture(const LocalPoiMarker &marker) const {
  if (!marker.TexturePath.empty()) {
    Texture *tex = m_texManager->GetTexture(marker.TexturePath, false);
    if (tex && tex->pTexture) return tex;
  }
  return m_localPoiIconTex;
}

void WorldMapControl::ApplyBoundsForCurrentStyle() {
  if (SelectedLocalMapId > 0) {
    const LocalMapDefinition *local = FindLocalMapById(SelectedLocalMapId);
    if (local) {
      m_minMx = local->MinMx; m_maxMx = local->MaxMx;
      m_minMy = local->MinMy; m_maxMy = local->MaxMy;
      return;
    }
  }
  if (SelectedDungeonFloorId > 0) {
    const InstanceMapDefinition *floor = FindInstanceMapById(SelectedDungeonFloorId);
    if (floor) {
      m_minMx = floor->MinMx;
      m_maxMx = floor->MinMx + (std::max)(1, (floor->RenderWidth + 31) / 32) - 1;
      m_minMy = floor->MinMy;
      m_maxMy = floor->MinMy + (std::max)(1, (floor->RenderHeight + 31) / 32) - 1;
      return;
    }
  }
  if (MapStyle == 0) {
    m_minMx = m_worldMinMx; m_maxMx = m_worldMaxMx;
    m_minMy = m_worldMinMy; m_maxMy = m_worldMaxMy;
    return;
  }
  if (MapStyle == 1) {
    m_minMx = m_minimapMinMx;
    m_maxMx = m_minimapMaxMx;
    m_minMy = m_minimapMinMy;
    m_maxMy = m_minimapMaxMy;
    return;
  }
  if (MapStyle == 2) {
    const InstanceMapDefinition *inst = nullptr;
    if (SelectedInstanceId > 0) inst = FindInstanceMapById(SelectedInstanceId);
    if (!inst) inst = FindInstanceMapForRegion(CurrentRx, CurrentRy);
    if (inst) {
      m_minMx = inst->MinMx; m_maxMx = inst->MaxMx;
      m_minMy = inst->MinMy; m_maxMy = inst->MaxMy;
      return;
    }
  }
  m_minMx = (std::min)(m_worldMinMx, m_minimapMinMx);
  m_maxMx = (std::max)(m_worldMaxMx, m_minimapMaxMx);
  m_minMy = (std::min)(m_worldMinMy, m_minimapMinMy);
  m_maxMy = (std::max)(m_worldMaxMy, m_minimapMaxMy);
}

void WorldMapControl::CenterOnRegion(int rx, int ry, float width, float height) {
  int displayX, displayY;
  RegionToDisplay(rx, ry, displayX, displayY);
  float worldX = MapToWorldX(static_cast<float>(displayX) + 0.5f);
  float worldY = MapToWorldY(static_cast<float>(displayY) - 0.5f);
  m_panOffset.x = width * 0.5f - worldX * m_zoom;
  m_panOffset.y = height * 0.5f - worldY * m_zoom;
  m_hasCenteredInitialRegion = true;
  MarkMapRenderDirty();
}

void WorldMapControl::SetDragging(bool dragging) {
  m_isDragging = dragging;
  if (dragging) m_lastPanOrZoomTick = GetTickCount();
}

void WorldMapControl::NotifyPanOrZoom() {
  m_lastPanOrZoomTick = GetTickCount();
  MarkMapRenderDirty();
}
void WorldMapControl::RefreshStyleBounds() {
  ApplyBoundsForCurrentStyle();
  MarkMapRenderDirty();
}

float WorldMapControl::MapToWorldX(float mx) const {
  return sro::map::MapCellToWorldX(static_cast<int>(mx), m_minMx);
}
float WorldMapControl::MapToWorldY(float my) const {
  return sro::map::MapCellToWorldY(static_cast<int>(my), m_maxMy);
}

void WorldMapControl::RegionToDisplay(int rx, int ry, int &displayX, int &displayY) const {
  sro::map::RegionToDisplay(rx, ry, displayX, displayY);
}

void WorldMapControl::DisplayToRegion(int displayX, int displayY, int &rx, int &ry) const {
  sro::map::DisplayToRegion(displayX, displayY, rx, ry);
}

void WorldMapControl::TranslateScreenToMap(const Vector2 &screenPos, int &mx, int &my, float width, float height) const {
  float worldX = (screenPos.x - m_panOffset.x) / m_zoom;
  float worldY = (screenPos.y - m_panOffset.y) / m_zoom;
  sro::map::WorldPixelToMapCell(worldX, worldY, m_minMx, m_maxMy, mx, my);
}

void WorldMapControl::TranslateScreenToRegion(const Vector2 &screenPos, int &rx, int &ry, float width, float height) const {
  int mx, my;
  TranslateScreenToMap(screenPos, mx, my, width, height);
  DisplayToRegion(mx, my, rx, ry);
  rx = std::clamp(rx, 0, 255);
  ry = std::clamp(ry, 0, 255);
}

bool WorldMapControl::TryTranslateScreenToActiveRegion(const Vector2 &screenPos, int &rx, int &ry,
                                                       float width, float height) const {
  TranslateScreenToRegion(screenPos, rx, ry, width, height);
  return IsRegionActive(rx, ry);
}

void WorldMapControl::HandleMouseInput(HWND hwnd, UINT msg, WPARAM wParam,
                                       LPARAM lParam, float width, float height,
                                       bool &regionDoubleClicked,
                                       int &clickedRx, int &clickedRy) {
  regionDoubleClicked = false;

  if (msg == WM_LBUTTONDOWN) {
    m_lastMousePos = Vector2(static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)));
    m_isDragging = false;
    SetCapture(hwnd);
  } else if (msg == WM_MOUSEMOVE) {
    if (GetCapture() == hwnd) {
      Vector2 pos(static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)));
      Vector2 delta(pos.x - m_lastMousePos.x, pos.y - m_lastMousePos.y);
      if (!m_isDragging && (fabsf(delta.x) > 3.0f || fabsf(delta.y) > 3.0f)) m_isDragging = true;
      if (m_isDragging) {
        m_panOffset.x += delta.x; m_panOffset.y += delta.y;
        m_lastMousePos = pos; m_lastPanOrZoomTick = GetTickCount();
      }
    }
  } else if (msg == WM_LBUTTONUP) {
    if (GetCapture() == hwnd) {
      bool wasDragging = m_isDragging;
      ReleaseCapture(); m_isDragging = false;
      if (!wasDragging) {
        m_lastMousePos = Vector2(static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)));
        int mx, my; TranslateScreenToMap(m_lastMousePos, mx, my, width, height);
        int rx, ry; DisplayToRegion(mx, my, rx, ry);
        CurrentRx = std::clamp(rx, 0, 255);
        CurrentRy = std::clamp(ry, 0, 255);
      }
    }
  } else if (msg == WM_LBUTTONDBLCLK) {
    Vector2 pos(static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)));
    int rx, ry;
    if (TryTranslateScreenToActiveRegion(pos, rx, ry, width, height)) {
      clickedRx = rx; clickedRy = ry;
      CurrentRx = clickedRx; CurrentRy = clickedRy;
      regionDoubleClicked = true;
    }
  } else if (msg == WM_MOUSEWHEEL) {
    POINT pt;
    pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
    ScreenToClient(hwnd, &pt);
    Vector2 mousePos(static_cast<float>(pt.x) - g_canvasStartX, static_cast<float>(pt.y) - g_canvasStartY);
    short delta = GET_WHEEL_DELTA_WPARAM(wParam);
    float oldZoom = m_zoom;
    float factor = (delta > 0) ? 1.18f : 0.84f;
    m_zoom = std::clamp(m_zoom * factor, 0.12f, 7.0f);
    m_panOffset = mousePos - (mousePos - m_panOffset) * (m_zoom / oldZoom);
    m_lastPanOrZoomTick = GetTickCount();
  }
}

namespace {
struct QuadVertex {
  float x, y, z;
  float u, v;
  static const DWORD FVF = D3DFVF_XYZ | D3DFVF_TEX1;
};

struct LineVertex {
  float x, y, z;
  DWORD color;
  static const DWORD FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
};
} // namespace

void WorldMapControl::Render2D(float width, float height) {
  if (width <= 0 || height <= 0) return;
  EnsureOverlayTextures();

  if (!m_hasCenteredInitialRegion) {
    CenterOnRegion(CurrentRx, CurrentRy, width, height);
  }

  Matrix4 view, scale, translation, proj;
  translation = MatrixTranslation(m_panOffset.x, m_panOffset.y, 0.0f);
  scale = MatrixScaling(m_zoom, m_zoom, 1.0f);
  view = scale * translation;
  m_device->SetTransform(D3DTS_VIEW, reinterpret_cast<const D3DMATRIX *>(&view));

  proj = MatrixOrthoOffCenterLH(0.0f, width, height, 0.0f, 0.0f, 1.0f);
  m_device->SetTransform(D3DTS_PROJECTION, reinterpret_cast<const D3DMATRIX *>(&proj));

  Matrix4 identity = Matrix4::Identity();
  m_device->SetTransform(D3DTS_WORLD, reinterpret_cast<const D3DMATRIX *>(&identity));

  m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  m_device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
  m_device->SetRenderState(D3DRS_FOGENABLE, FALSE);
  m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

  m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

  m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
  m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
  m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  m_device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
  m_device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
  m_device->SetFVF(QuadVertex::FVF);

  float worldX0 = (0.0f - m_panOffset.x) / m_zoom;
  float worldX1 = (width - m_panOffset.x) / m_zoom;
  float worldY0 = (0.0f - m_panOffset.y) / m_zoom;
  float worldY1 = (height - m_panOffset.y) / m_zoom;

  int minMx = m_minMx + static_cast<int>(floorf(worldX0 / sro::map::kMapPixelsPerRegion)) - 1;
  int maxMx = m_minMx + static_cast<int>(floorf(worldX1 / sro::map::kMapPixelsPerRegion)) + 1;
  int maxMy = m_maxMy - static_cast<int>(floorf(worldY0 / sro::map::kMapPixelsPerRegion)) + 1;
  int minMy = m_maxMy - static_cast<int>(floorf(worldY1 / sro::map::kMapPixelsPerRegion)) - 1;

  // 1. Draw World Underlay Tiles
  if (SelectedLocalMapId == 0 && SelectedDungeonFloorId == 0 && MapStyle == 0) {
  for (const auto &tile : m_worldTiles) {
    int mx = tile.first.first;
    int my = tile.first.second;
    if (mx + 3 < minMx || mx > maxMx || my < minMy || my - 3 > maxMy) continue;

    float x0 = MapToWorldX(static_cast<float>(mx));
    float y0 = MapToWorldY(static_cast<float>(my));
    float x1 = x0 + sro::map::kWorldTilePixels;
    float y1 = y0 + sro::map::kWorldTilePixels;

    Texture *tex = AcquireMapTexture(tile.second, MapLoadPolicy::NoLoad);
    if (!tex || !tex->pTexture) {
      tex = m_mapCache ? m_mapCache->Preload(tile.second) : nullptr;
    }
    if (tex && tex->pTexture) {
      m_device->SetTexture(0, tex->pTexture);
      if (!ActiveRegions.empty()) {
        m_device->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255, 255, 255, 255));
        m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
        m_device->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(220, 255, 255, 255));
      }
      QuadVertex q[4] = {{x0, y0, 0.5f, 0.0f, 0.0f}, {x1, y0, 0.5f, 1.0f, 0.0f},
                          {x0, y1, 0.5f, 0.0f, 1.0f}, {x1, y1, 0.5f, 1.0f, 1.0f}};
      m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(QuadVertex));
      m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
      m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
    } else if (MapStyle == 0) {
      m_device->SetTexture(0, nullptr);
      m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
      m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
      m_device->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(200, 28, 32, 40));
      QuadVertex q[4] = {{x0, y0, 0.45f, 0, 0}, {x1, y0, 0.45f, 0, 0},
                          {x0, y1, 0.45f, 0, 0}, {x1, y1, 0.45f, 0, 0}};
      m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(QuadVertex));
      m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
      m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    }
  }
  }

  // 1b. Dim inactive region cells on world map
  if (SelectedLocalMapId == 0 && MapStyle == 0 && !ActiveRegions.empty()) {
    m_device->SetTexture(0, nullptr);
    m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    m_device->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(140, 8, 12, 24));
    for (int my = minMy; my <= maxMy; ++my) {
      for (int mx = minMx; mx <= maxMx; ++mx) {
        if (IsTileRegionActive(mx, my)) continue;
        float x0 = MapToWorldX(static_cast<float>(mx));
        float y0 = MapToWorldY(static_cast<float>(my));
        float x1 = x0 + sro::map::kMapPixelsPerRegion;
        float y1 = y0 + sro::map::kMapPixelsPerRegion;
        QuadVertex q[4] = {{x0, y0, 0.4f, 0, 0}, {x1, y0, 0.4f, 0, 0},
                            {x0, y1, 0.4f, 0, 0}, {x1, y1, 0.4f, 0, 0}};
        m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(QuadVertex));
      }
    }
    m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  }

  // 2. Draw Minimaps (stream visible cells)
  if (SelectedLocalMapId == 0 && MapStyle == 1) {
    int mmMinMx = std::clamp(minMx, m_minimapMinMx, m_minimapMaxMx);
    int mmMaxMx = std::clamp(maxMx, m_minimapMinMx, m_minimapMaxMx);
    int mmMinMy = std::clamp(minMy, m_minimapMinMy, m_minimapMaxMy);
    int mmMaxMy = std::clamp(maxMy, m_minimapMinMy, m_minimapMaxMy);

    for (int my = mmMinMy; my <= mmMaxMy; ++my) {
      for (int mx = mmMinMx; mx <= mmMaxMx; ++mx) {
        float x0 = MapToWorldX(static_cast<float>(mx));
        float y0 = MapToWorldY(static_cast<float>(my));
        float x1 = x0 + sro::map::kMapPixelsPerRegion;
        float y1 = y0 + sro::map::kMapPixelsPerRegion;

        int rx = my, ry = mx;
        auto path = m_minimapIndex.PathForRegion(rx, ry);
        Texture *tex = nullptr;
        if (path) tex = AcquireMapTexture(*path, MapLoadPolicy::NoLoad);

        if (tex && tex->pTexture) {
          m_device->SetTexture(0, tex->pTexture);
          QuadVertex q[4] = {{x0, y0, 0.5f, 0.0f, 0.0f}, {x1, y0, 0.5f, 1.0f, 0.0f},
                              {x0, y1, 0.5f, 0.0f, 1.0f}, {x1, y1, 0.5f, 1.0f, 1.0f}};
          m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(QuadVertex));
        } else {
          m_device->SetTexture(0, nullptr);
          m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
          m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
          m_device->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(200, 28, 32, 40));
          QuadVertex q[4] = {{x0, y0, 0.45f, 0, 0}, {x1, y0, 0.45f, 0, 0},
                              {x0, y1, 0.45f, 0, 0}, {x1, y1, 0.45f, 0, 0}};
          m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(QuadVertex));
          m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
          m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        }
      }
    }
  }

  // 2b. Instance / dungeon map overlay
  if (SelectedLocalMapId == 0 && MapStyle == 2) {
    const InstanceMapDefinition *inst = nullptr;
    if (SelectedInstanceId > 0) inst = FindInstanceMapById(SelectedInstanceId);
    if (!inst) inst = FindInstanceMapForRegion(CurrentRx, CurrentRy);
    if (inst && !inst->TexturePath.empty()) {
      Texture *tex = AcquireMapTexture(inst->TexturePath, MapLoadPolicy::NoLoad);
      if (tex && tex->pTexture) {
        float x0 = MapToWorldX(static_cast<float>(inst->MinMx));
        float y0 = MapToWorldY(static_cast<float>(inst->MaxMy));
        float x1 = MapToWorldX(static_cast<float>(inst->MaxMx + 1));
        float y1 = MapToWorldY(static_cast<float>(inst->MinMy - 1));
        m_device->SetTexture(0, tex->pTexture);
        QuadVertex q[4] = {{x0, y0, 0.45f, 0.0f, 0.0f}, {x1, y0, 0.45f, 1.0f, 0.0f},
                            {x0, y1, 0.45f, 0.0f, 1.0f}, {x1, y1, 0.45f, 1.0f, 1.0f}};
        m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(QuadVertex));
      }
    }
  }

  // 2d. Dungeon drill-down (tiled floor art)
  if (SelectedDungeonFloorId > 0) {
    const InstanceMapDefinition *floor = FindInstanceMapById(SelectedDungeonFloorId);
    if (floor) {
      const sro::map::LocalMapWorldRect rect = ComputeDungeonFloorWorldRect(*floor);
      const float worldX0 = rect.x0;
      const float worldY0 = rect.y0;
      if (floor->IsTiled && !floor->TilePrefixPath.empty()) {
        const sro::map::DungeonFloorLayout layout = sro::map::DungeonFloorLayoutFromDef(
            floor->RenderWidth, floor->RenderHeight, floor->TileCols, floor->TileRows,
            floor->MinMx, floor->MaxMx, floor->MinMy, floor->MaxMy);
        const float tileW = sro::map::kDungeonTilePixels * layout.tileScaleX;
        const float tileH = sro::map::kDungeonTilePixels * layout.tileScaleY;
        for (int my = floor->MinMy; my <= floor->MaxMy; ++my) {
          for (int mx = floor->MinMx; mx <= floor->MaxMx; ++mx) {
            std::wstring tilePath = DungeonTilePath(*floor, mx, my);
            if (tilePath.empty()) continue;
            Texture *tex = AcquireMapTexture(tilePath, MapLoadPolicy::NoLoad);
            if (!tex || !tex->pTexture) continue;
            const float tx = worldX0 + static_cast<float>(mx - floor->MinMx) * tileW;
            const float ty = worldY0 + static_cast<float>(floor->MaxMy - my) * tileH;
            const float tx1 = tx + tileW;
            const float ty1 = ty + tileH;
            m_device->SetTexture(0, tex->pTexture);
            QuadVertex q[4] = {{tx, ty, 0.45f, 0.0f, 0.0f}, {tx1, ty, 0.45f, 1.0f, 0.0f},
                                {tx, ty1, 0.45f, 0.0f, 1.0f}, {tx1, ty1, 0.45f, 1.0f, 1.0f}};
            m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(QuadVertex));
          }
        }
      } else if (!floor->TexturePath.empty()) {
        Texture *tex = AcquireMapTexture(floor->TexturePath, MapLoadPolicy::NoLoad);
        if (tex && tex->pTexture) {
          const float x1 = worldX0 + rect.w;
          const float y1 = worldY0 + rect.h;
          m_device->SetTexture(0, tex->pTexture);
          QuadVertex q[4] = {{worldX0, worldY0, 0.45f, 0.0f, 0.0f}, {x1, worldY0, 0.45f, 1.0f, 0.0f},
                              {worldX0, y1, 0.45f, 0.0f, 1.0f}, {x1, y1, 0.45f, 1.0f, 1.0f}};
          m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(QuadVertex));
        }
      }
    }
  }

  // 2c. City / local inner map
  if (SelectedLocalMapId > 0) {
    const LocalMapDefinition *local = FindLocalMapById(SelectedLocalMapId);
    if (local && !local->TexturePath.empty()) {
      Texture *tex = AcquireMapTexture(local->TexturePath, MapLoadPolicy::NoLoad);
      const sro::map::LocalMapWorldRect rect = ComputeLocalMapWorldRect(*local);
      const float x0 = rect.x0;
      const float y0 = rect.y0;
      const float x1 = rect.x0 + rect.w;
      const float y1 = rect.y0 + rect.h;
      if (tex && tex->pTexture) {
        const sro::map::LocalMapViewState viewState = sro::map::LocalMapViewStateFromDef(
            local->RawMx0, local->RawMy0, local->RawMx1, local->RawMy1, local->MinMx,
            local->MaxMx, local->MinMy, local->MaxMy,
            local->CoordWidth > 0 ? local->CoordWidth : local->RenderWidth,
            local->CoordHeight > 0 ? local->CoordHeight : local->RenderHeight,
            local->RenderWidth, local->RenderHeight, local->TextureWidth, local->TextureHeight);
        m_device->SetTexture(0, tex->pTexture);
        QuadVertex q[4] = {{x0, y0, 0.45f, viewState.uv.uvMinU, viewState.uv.uvMinV},
                            {x1, y0, 0.45f, viewState.uv.uvMaxU, viewState.uv.uvMinV},
                            {x0, y1, 0.45f, viewState.uv.uvMinU, viewState.uv.uvMaxV},
                            {x1, y1, 0.45f, viewState.uv.uvMaxU, viewState.uv.uvMaxV}};
        m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(QuadVertex));
      } else {
        m_device->SetTexture(0, nullptr);
        m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
        m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
        m_device->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(200, 28, 32, 40));
        QuadVertex q[4] = {{x0, y0, 0.45f, 0, 0}, {x1, y0, 0.45f, 0, 0},
                            {x0, y1, 0.45f, 0, 0}, {x1, y1, 0.45f, 0, 0}};
        m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(QuadVertex));
        m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
      }
    }
  }

  m_device->SetTexture(0, nullptr);

  // 3. Draw Selected Highlight Box
  if (SelectedLocalMapId == 0 && SelectedDungeonFloorId == 0) {
  int curMx, curMy;
  RegionToDisplay(CurrentRx, CurrentRy, curMx, curMy);
  float xCur = MapToWorldX(static_cast<float>(curMx));
  float yCur = MapToWorldY(static_cast<float>(curMy));

  m_device->SetFVF(LineVertex::FVF);
  DWORD selectCol = D3DCOLOR_ARGB(255, 230, 60, 60);
  LineVertex box[5] = {{xCur, yCur, 0.0f, selectCol}, {xCur + sro::map::kMapPixelsPerRegion, yCur, 0.0f, selectCol},
                        {xCur + sro::map::kMapPixelsPerRegion, yCur + sro::map::kMapPixelsPerRegion, 0.0f, selectCol},
                        {xCur, yCur + sro::map::kMapPixelsPerRegion, 0.0f, selectCol},
                        {xCur, yCur, 0.0f, selectCol}};
  m_device->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, box, sizeof(LineVertex));
  }

  // 4. Draw Grid Lines (world / region-minimap only — ISRO hides grid on inner city / dungeon map)
  if (m_zoom >= 0.5f && SelectedLocalMapId == 0 && SelectedDungeonFloorId == 0) {
    std::vector<LineVertex> grid;
    DWORD gridCol = D3DCOLOR_ARGB(35, 0, 122, 204);
    DWORD majorCol = D3DCOLOR_ARGB(70, 0, 122, 204);

    float xStart = MapToWorldX(static_cast<float>(m_minMx));
    float xEnd = MapToWorldX(static_cast<float>(m_maxMx + 1));
    float yStart = MapToWorldY(static_cast<float>(m_maxMy + 1));
    float yEnd = MapToWorldY(static_cast<float>(m_minMy));

    for (int mx = m_minMx; mx <= m_maxMx + 1; ++mx) {
      float x = MapToWorldX(static_cast<float>(mx));
      DWORD col = ((mx & 3) == 0) ? majorCol : gridCol;
      grid.push_back({x, yStart, 0.0f, col});
      grid.push_back({x, yEnd, 0.0f, col});
    }
    for (int my = m_minMy - 1; my <= m_maxMy; ++my) {
      float y = MapToWorldY(static_cast<float>(my + 1));
      DWORD col = (((my + 1) & 3) == 0) ? majorCol : gridCol;
      grid.push_back({xStart, y, 0.0f, col});
      grid.push_back({xEnd, y, 0.0f, col});
    }

    m_device->DrawPrimitiveUP(D3DPT_LINELIST, static_cast<UINT>(grid.size() / 2), grid.data(), sizeof(LineVertex));
  }

  m_device->SetRenderState(D3DRS_ZENABLE, TRUE);
  m_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

void WorldMapControl::RenderImGuiOverlays(float screenX, float screenY,
                                          float width, float height, bool mapHovered) {
  if (width <= 0.0f || height <= 0.0f) return;
  EnsureOverlayTextures();

  ImDrawList *drawList = ImGui::GetWindowDrawList();
  const ImVec2 clipMin(screenX, screenY);
  const ImVec2 clipMax(screenX + width, screenY + height);
  drawList->PushClipRect(clipMin, clipMax, true);

  const ImVec2 mousePos = ImGui::GetMousePos();
  const float worldMonsterIconSize = (std::clamp)(12.0f * m_zoom, 12.0f, 20.0f);
  constexpr float kLocalPoiFallbackTexPx = 14.0f;

  auto worldToScreen = [&](float worldX, float worldY) -> ImVec2 {
    return ImVec2(screenX + m_panOffset.x + worldX * m_zoom,
                  screenY + m_panOffset.y + worldY * m_zoom);
  };

  auto texToScreenScale = [&](float worldSpan, int renderSize) -> float {
    return worldSpan / static_cast<float>(std::max(1, renderSize)) * m_zoom;
  };

  auto localIconScreenSize = [&](const LocalPoiMarker &poi, float sx, float sy) -> ImVec2 {
    if (poi.IconWidth > 0 && poi.IconHeight > 0) {
      return {(std::clamp)(static_cast<float>(poi.IconWidth) * sx, 6.0f, 40.0f),
              (std::clamp)(static_cast<float>(poi.IconHeight) * sy, 6.0f, 40.0f)};
    }
    const float s = (sx + sy) * 0.5f;
    const float side = (std::clamp)(kLocalPoiFallbackTexPx * s, 6.0f, 40.0f);
    return {side, side};
  };

  auto localMapViewStateFromDef = [](const LocalMapDefinition &def) -> sro::map::LocalMapViewState {
    return sro::map::LocalMapViewStateFromDef(
        def.RawMx0, def.RawMy0, def.RawMx1, def.RawMy1, def.MinMx, def.MaxMx, def.MinMy,
        def.MaxMy, def.CoordWidth > 0 ? def.CoordWidth : def.RenderWidth,
        def.CoordHeight > 0 ? def.CoordHeight : def.RenderHeight, def.RenderWidth, def.RenderHeight,
        def.TextureWidth, def.TextureHeight);
  };

  auto localPoiRenderCenter = [&](const LocalPoiMarker &poi,
                                  const sro::map::LocalMapViewState &state) -> sro::map::TexturePixelPos {
    return sro::map::LocalPoiToRenderPixel(
        poi.TargetRx, poi.TargetRy, static_cast<float>(poi.PixelX),
        static_cast<float>(poi.PixelY), static_cast<float>(poi.IconWidth) * 0.5f,
        static_cast<float>(poi.IconHeight) * 0.5f, state);
  };

  auto renderPixelToScreen = [&](const sro::map::TexturePixelPos &rp,
                                 const sro::map::LocalMapViewState &state, float worldX0,
                                 float worldY0, float worldW, float worldH) -> ImVec2 {
    const sro::map::TexturePixelPos norm =
        sro::map::RenderPixelToNormalized(rp, state.renderWidth, state.renderHeight);
    return worldToScreen(worldX0 + norm.x * worldW, worldY0 + norm.y * worldH);
  };

  auto buildDetailedTooltip = [this](const LocalPoiMarker &poi) -> std::string {
    std::string tooltip;
    std::string nameEng = poi.NameUtf8;
    std::string nameKo = poi.NameKoUtf8;
    if (nameEng.empty()) nameEng = nameKo;
    if (!nameEng.empty()) {
      tooltip += nameEng;
      if (!nameKo.empty() && nameKo != nameEng) tooltip += " (" + nameKo + ")";
    }

    std::string areaEng = poi.AreaNameUtf8;
    std::string areaKo = poi.AreaKoUtf8;

    auto isUnnecessaryArea = [](const std::string &str) -> bool {
      if (str.empty()) return true;
      std::string lower = str;
      for (char &c : lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
      return (lower == "\xec\x9b\x94\xeb\x93\x9c\xeb\xa7\xb5" || lower == "world map" ||
              lower == "worldmap" || lower == "xxx" || lower == "0");
    };

    if (isUnnecessaryArea(areaEng) && poi.TargetRx >= 0 && poi.TargetRy >= 0) {
      int regId = (poi.TargetRx << 8) + poi.TargetRy;
      auto itR = m_regionNames.find(regId);
      if (itR != m_regionNames.end()) {
        areaKo = itR->second.first; areaEng = itR->second.second;
        if (areaEng.empty()) areaEng = areaKo;
      }
    }

    if (!isUnnecessaryArea(areaEng) && areaEng != nameEng) {
      if (!tooltip.empty()) tooltip += "\n";
      tooltip += "Area: " + areaEng;
      if (!isUnnecessaryArea(areaKo) && areaKo != areaEng && areaKo != nameKo) tooltip += " (" + areaKo + ")";
    }

    if (poi.TargetRx >= 0 && poi.TargetRy >= 0) {
      if (!tooltip.empty()) tooltip += "\n";
      tooltip += "Region: (" + std::to_string(poi.TargetRx) + ", " + std::to_string(poi.TargetRy) + ")";
      tooltip += " Local: (" + std::to_string(poi.PixelX) + ", " + std::to_string(poi.PixelY) + ")";
    }

    if (!poi.ZoneCode.empty()) {
      if (!tooltip.empty()) tooltip += "\n";
      tooltip += "Code: " + poi.ZoneCode;
    }
    return tooltip;
  };

  auto drawLabeledMarker = [&](const ImVec2 &center, float iconW, float iconH,
                               Texture *tex, ImU32 tint, const std::string &labelUtf8,
                               const std::string &tooltipUtf8, bool drawText, bool drawTexture,
                               bool drawAtBottom = false, ImU32 textColor = IM_COL32(245, 245, 230, 255),
                               bool isStrong = false, bool isCity = false, float labelScale = 0.0f,
                               float maxFontSize = 48.0f) {
    if (labelScale <= 0.0f) labelScale = m_zoom;
    if (center.x + iconW * 0.5f < clipMin.x || center.x - iconW * 0.5f > clipMax.x ||
        center.y + iconH * 0.5f < clipMin.y || center.y - iconH * 0.5f > clipMax.y) return;

    ImVec2 p0(center.x - iconW * 0.5f, center.y - iconH * 0.5f);
    ImVec2 p1(center.x + iconW * 0.5f, center.y + iconH * 0.5f);
    if (drawTexture && tex && tex->pTexture) {
      drawList->AddImage(reinterpret_cast<ImTextureID>(tex->pTexture), p0, p1, ImVec2(0, 0), ImVec2(1, 1), tint);
    } else if (drawTexture) {
      p0 = ImVec2(center.x - 5.0f, center.y - 5.0f);
      p1 = ImVec2(center.x + 5.0f, center.y + 5.0f);
    }

    if (drawText && !labelUtf8.empty()) {
      ImFont *font = ImGui::GetFont();
      auto &io = ImGui::GetIO();
      if (io.Fonts->Fonts.Size > 1) font = io.Fonts->Fonts[1];

      float baseFontSize = ImGui::GetFontSize();
      float scaledFontSize = (std::clamp)(baseFontSize * labelScale, 5.0f, maxFontSize);

      ImVec2 textSize = font->CalcTextSizeA(scaledFontSize, FLT_MAX, 0.0f, labelUtf8.c_str());
      float textWidth = textSize.x, textHeight = textSize.y;

      ImVec2 textPos;
      if (tex) {
        if (drawAtBottom) {
          textPos = ImVec2(center.x - textWidth * 0.5f, center.y + iconH * 0.5f + 4.0f * labelScale);
        } else {
          textPos = ImVec2(center.x + iconW * 0.5f + 4.0f * labelScale, center.y - textHeight * 0.5f);
        }
      } else {
        textPos = ImVec2(center.x - textWidth * 0.5f, center.y - textHeight * 0.5f);
        bool isCityLabel = (textColor == IM_COL32(255, 247, 182, 255));
        if (isCityLabel) textPos.y += 5.f * labelScale;
      }

      const bool textVisible = !(textPos.x + textWidth < clipMin.x || textPos.x > clipMax.x ||
                                 textPos.y + textHeight < clipMin.y || textPos.y > clipMax.y);
      if (textVisible) {
      float shadowOffset = (std::max)(1.0f, roundf(labelScale * 0.6f));
      const ImVec2 shadowOffsets[] = {
          ImVec2(-shadowOffset, 0.0f), ImVec2(shadowOffset, 0.0f),
          ImVec2(0.0f, -shadowOffset), ImVec2(0.0f, shadowOffset)};
      for (const auto &off : shadowOffsets) {
        drawList->AddText(font, scaledFontSize, ImVec2(textPos.x + off.x, textPos.y + off.y),
                          IM_COL32(0, 0, 0, 200), labelUtf8.c_str());
      }
      drawList->AddText(font, scaledFontSize, textPos, textColor, labelUtf8.c_str());
      if (isStrong) {
        drawList->AddText(font, scaledFontSize, ImVec2(textPos.x + 0.5f * labelScale, textPos.y), textColor, labelUtf8.c_str());
      }
      }
    }

    if (mapHovered && mousePos.x >= p0.x && mousePos.x <= p1.x &&
        mousePos.y >= p0.y && mousePos.y <= p1.y && !tooltipUtf8.empty()) {
      ImGui::BeginTooltip();
      ImGui::TextUnformatted(tooltipUtf8.c_str());
      ImGui::EndTooltip();
    }
  };

  auto drawInnerCityMarkerIcon = [&](const ImVec2 &screenPos, const ImVec2 &iconSize, Texture *markerTex,
                                     const LocalPoiMarker &poi, const std::string &tooltip) {
    if (markerTex && markerTex->pTexture) {
      drawLabeledMarker(screenPos, iconSize.x, iconSize.y, markerTex, IM_COL32(255, 245, 190, 245), "",
                        tooltip, false, true, false);
      return;
    }
    const float r = (std::max)(iconSize.x, iconSize.y) * 0.4f;
    if (screenPos.x + r < clipMin.x || screenPos.x - r > clipMax.x || screenPos.y + r < clipMin.y ||
        screenPos.y - r > clipMax.y) {
      return;
    }
    drawList->AddCircleFilled(screenPos, r, IM_COL32(72, 180, 72, 230));
    drawList->AddCircle(screenPos, r, IM_COL32(20, 60, 20, 255), 0, 1.5f);
    if (mapHovered && !tooltip.empty()) {
      const float dx = mousePos.x - screenPos.x;
      const float dy = mousePos.y - screenPos.y;
      if (dx * dx + dy * dy <= r * r) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(tooltip.c_str());
        ImGui::EndTooltip();
      }
    }
  };

  auto drawWorldPoiOverlays = [&](bool drawWorldText) {
    for (const auto &poi : m_localPoiMarkers) {
      if (poi.MapId != 0 || poi.TargetRx < 0 || poi.TargetRy < 0) continue;
      if (poi.PixelX < 0 || poi.PixelX >= 256 || poi.PixelY < 0 || poi.PixelY >= 256) continue;

      float worldX = MapToWorldX(static_cast<float>(poi.TargetRy)) + static_cast<float>(poi.PixelX) +
                     static_cast<float>(poi.IconWidth) * 0.5f;
      float worldY = MapToWorldY(static_cast<float>(poi.TargetRx)) + static_cast<float>(poi.PixelY) +
                     static_cast<float>(poi.IconHeight) * 0.5f;
      ImVec2 screenPos = worldToScreen(worldX, worldY);
      if (screenPos.x < clipMin.x - 32.0f || screenPos.x > clipMax.x + 32.0f ||
          screenPos.y < clipMin.y - 32.0f || screenPos.y > clipMax.y + 32.0f) {
        continue;
      }

      Texture *markerTex = GetMarkerTexture(poi);
      if (markerTex && !poi.TexturePath.empty()) {
        std::string tooltip = buildDetailedTooltip(poi);
        if (IsCityOrFortressPoi(poi) && FindLocalMapForPoi(poi)) {
          if (!tooltip.empty()) tooltip += "\n";
          tooltip += "Click to open city map";
        }
        if (IsDungeonEntrancePoi(poi) && FindInstanceMapById(poi.LinkedDungeonId)) {
          if (!tooltip.empty()) tooltip += "\n";
          tooltip += "Click to open dungeon map";
        }
        const bool clickableCity = IsCityOrFortressPoi(poi) && FindLocalMapForPoi(poi);
        const bool clickableDungeon =
            IsDungeonEntrancePoi(poi) && FindInstanceMapById(poi.LinkedDungeonId);
        std::string tooltipToUse =
            (poi.MarkerKind == 1 && !clickableCity && !clickableDungeon) ? "" : tooltip;
        float iconW =
            poi.IconWidth > 0 ? static_cast<float>(poi.IconWidth) * m_zoom : worldMonsterIconSize;
        float iconH =
            poi.IconHeight > 0 ? static_cast<float>(poi.IconHeight) * m_zoom : worldMonsterIconSize;
        bool drawAtBottom = (poi.IconWidth >= 48 || poi.TexturePath.find(L"city_") != std::wstring::npos ||
                             poi.TexturePath.find(L"fort_") != std::wstring::npos);
        drawLabeledMarker(screenPos, iconW, iconH, markerTex, IM_COL32(255, 255, 255, 245), "",
                          tooltipToUse, false, true, drawAtBottom);
      }
    }

    for (const auto &poi : m_localPoiMarkers) {
      if (poi.MapId != 0 || poi.TargetRx < 0 || poi.TargetRy < 0) continue;
      if (poi.PixelX < 0 || poi.PixelX >= 256 || poi.PixelY < 0 || poi.PixelY >= 256) continue;

      float worldX = MapToWorldX(static_cast<float>(poi.TargetRy)) + static_cast<float>(poi.PixelX) +
                     static_cast<float>(poi.IconWidth) * 0.5f;
      float worldY = MapToWorldY(static_cast<float>(poi.TargetRx)) + static_cast<float>(poi.PixelY) +
                     static_cast<float>(poi.IconHeight) * 0.5f;
      ImVec2 screenPos = worldToScreen(worldX, worldY);
      if (screenPos.x < clipMin.x - 32.0f || screenPos.x > clipMax.x + 32.0f ||
          screenPos.y < clipMin.y - 32.0f || screenPos.y > clipMax.y + 32.0f) {
        continue;
      }

      std::string label = poi.NameUtf8.empty() ? poi.AreaNameUtf8 : poi.NameUtf8;
      bool isCityOrFortress =
          (!poi.TexturePath.empty() &&
           (poi.TexturePath.find(L"city_") != std::wstring::npos ||
            poi.TexturePath.find(L"fort_") != std::wstring::npos));
      std::string tooltip = buildDetailedTooltip(poi);
      if (isCityOrFortress && FindLocalMapForPoi(poi)) {
        if (!tooltip.empty()) tooltip += "\n";
        tooltip += "Click to open city map";
      }
      if (IsDungeonEntrancePoi(poi) && FindInstanceMapById(poi.LinkedDungeonId)) {
        if (!tooltip.empty()) tooltip += "\n";
        tooltip += "Click to open dungeon map";
      }
      Texture *markerTex = GetMarkerTexture(poi);
      bool hasIcon = (markerTex && !poi.TexturePath.empty());
      std::string tooltipToUse = (poi.MarkerKind == 1 || hasIcon) ? "" : tooltip;

      float iconW =
          poi.IconWidth > 0 ? static_cast<float>(poi.IconWidth) * m_zoom : worldMonsterIconSize;
      float iconH =
          poi.IconHeight > 0 ? static_cast<float>(poi.IconHeight) * m_zoom : worldMonsterIconSize;

      ImU32 textColorToUse = PoiFileColorToImU32(poi.ColorR, poi.ColorG, poi.ColorB);
      bool isCity = false;
      if (isCityOrFortress) {
        textColorToUse = IM_COL32(255, 247, 182, 255);
        if (label.find("Fortress") != std::string::npos ||
            label.find("\xec\x9a\x94\xec\x83\x88") != std::string::npos) {
          textColorToUse = IM_COL32(255, 225, 80, 255);
        } else {
          isCity = true;
        }
      } else if (poi.ColorR == 255 && poi.ColorG == 255 && poi.ColorB == 255) {
        if (label.find("Fortress") != std::string::npos ||
            label.find("\xec\x9a\x94\xec\x83\x88") != std::string::npos) {
          textColorToUse = IM_COL32(255, 225, 80, 255);
        }
      }

      bool drawText = drawWorldText && (poi.MarkerKind == 1);
      bool drawAtBottom = isCityOrFortress;
      Texture *texToUse = isCityOrFortress ? markerTex : nullptr;

      drawLabeledMarker(screenPos, iconW, iconH, texToUse, IM_COL32(255, 255, 255, 245), label,
                        tooltipToUse, drawText, false, drawAtBottom, textColorToUse,
                        poi.IsStrong != 0, isCity);
    }
  };

  if (MapStyle == 0 && SelectedLocalMapId == 0 && SelectedDungeonFloorId == 0) {
    drawWorldPoiOverlays(m_zoom >= 0.45f);

    if (m_zoom >= 1.1f) {
      for (const auto &marker : m_worldGuideMarkers) {
        ImVec2 screenPos = worldToScreen(static_cast<float>(marker.PixelX), 96.0f + static_cast<float>(marker.PixelY));
        std::string tooltip = marker.NameUtf8;
        if (!marker.CodeName.empty()) tooltip += "\n" + marker.CodeName;
        tooltip += "\nRegion: (" + std::to_string(marker.Rx) + ", " + std::to_string(marker.Ry) + ")";
        drawLabeledMarker(screenPos, worldMonsterIconSize, worldMonsterIconSize,
                          m_worldMonsterIconTex, IM_COL32(255, 255, 255, 235), marker.NameUtf8, tooltip, false, true, false);
      }
    }

    if (m_hasPlayerPosition) {
      sro::map::MapPixelPos mapPos = sro::map::RegionLocalToMapPixel(
          m_playerRx, m_playerRy, m_playerLocalX, m_playerLocalZ, m_minMx, m_maxMy);
      ImVec2 screenPos = worldToScreen(mapPos.x, mapPos.y);
      float iconSize = (std::clamp)(16.0f * m_zoom, 12.0f, 28.0f);
      std::string tooltip = "Player\nRegion: (" + std::to_string(m_playerRx) + ", " + std::to_string(m_playerRy) + ")";
      if (m_playerMarkerTex && m_playerMarkerTex->pTexture) {
        MapPlayerIcon::DrawRotatedMapIcon(drawList,
            reinterpret_cast<ImTextureID>(m_playerMarkerTex->pTexture), screenPos, iconSize, m_playerHeadingRad);
      }
      if (mapHovered) {
        const float hitR = iconSize * 0.55f;
        const float dx = mousePos.x - screenPos.x;
        const float dy = mousePos.y - screenPos.y;
        if (dx * dx + dy * dy <= hitR * hitR) {
          ImGui::BeginTooltip();
          ImGui::TextUnformatted(tooltip.c_str());
          ImGui::EndTooltip();
        }
      }
    }

    for (const auto &colleague : ColleagueMarkers) {
      if (!IsRegionActive(colleague.Rx, colleague.Ry)) continue;
      sro::map::MapPixelPos mapPos = sro::map::RegionLocalToMapPixel(
          colleague.Rx, colleague.Ry, colleague.LocalX, colleague.LocalZ, m_minMx, m_maxMy);
      ImVec2 screenPos = worldToScreen(mapPos.x, mapPos.y);
      float iconSize = (std::clamp)(14.0f * m_zoom, 10.0f, 24.0f);
      std::string tooltip = colleague.LabelUtf8.empty() ? "Colleague" : colleague.LabelUtf8;
      tooltip += "\nRegion: (" + std::to_string(colleague.Rx) + ", " + std::to_string(colleague.Ry) + ")";
      drawLabeledMarker(screenPos, iconSize, iconSize, GetColleagueTexture(colleague.Type),
                        IM_COL32(255, 255, 255, 240), colleague.LabelUtf8, tooltip, false, true, false);
    }
  }

  if (MapStyle == 1 && SelectedLocalMapId == 0 && SelectedDungeonFloorId == 0) {
    drawWorldPoiOverlays(m_zoom >= 0.45f);

    const float vpWorldX0 = (0.0f - m_panOffset.x) / m_zoom;
    const float vpWorldX1 = (width - m_panOffset.x) / m_zoom;
    const float vpWorldY0 = (0.0f - m_panOffset.y) / m_zoom;
    const float vpWorldY1 = (height - m_panOffset.y) / m_zoom;
    const int visMinMx =
        m_minMx + static_cast<int>(floorf(vpWorldX0 / sro::map::kMapPixelsPerRegion)) - 1;
    const int visMaxMx =
        m_minMx + static_cast<int>(floorf(vpWorldX1 / sro::map::kMapPixelsPerRegion)) + 1;
    const int visMaxMy =
        m_maxMy - static_cast<int>(floorf(vpWorldY0 / sro::map::kMapPixelsPerRegion)) + 1;
    const int visMinMy =
        m_maxMy - static_cast<int>(floorf(vpWorldY1 / sro::map::kMapPixelsPerRegion)) - 1;

    const bool drawLocalText = m_zoom >= 0.55f;
    const LocalMapDefinition *playerLocalMap = nullptr;

    for (const LocalMapDefinition *localMap :
         FindLocalMapsInViewport(visMinMx, visMaxMx, visMinMy, visMaxMy)) {
      const sro::map::LocalMapWorldRect gridRect = ComputeLocalMapGridWorldRect(*localMap);
      const float worldX0 = gridRect.x0;
      const float worldY0 = gridRect.y0;
      const float worldW = gridRect.w;
      const float worldH = gridRect.h;
      const sro::map::LocalMapViewState viewState = localMapViewStateFromDef(*localMap);
      const float sx = texToScreenScale(worldW, static_cast<int>(viewState.renderWidth));
      const float sy = texToScreenScale(worldH, static_cast<int>(viewState.renderHeight));
      const float labelScale = (sx + sy) * 0.5f;

      for (const auto &poi : m_localPoiMarkers) {
        if (!sro::map::ShouldDrawInnerCityPoi(poi.MapId, localMap->Id)) continue;
        if (!ShouldDrawInnerCityIconPass(poi)) continue;
        ImVec2 screenPos = renderPixelToScreen(localPoiRenderCenter(poi, viewState), viewState,
                                               worldX0, worldY0, worldW, worldH);
        Texture *markerTex = GetMarkerTexture(poi);
        ImVec2 iconSize = localIconScreenSize(poi, sx, sy);
        drawInnerCityMarkerIcon(screenPos, iconSize, markerTex, poi, InnerCityDisplayName(poi));
      }

      for (const auto &poi : m_localPoiMarkers) {
        if (!sro::map::ShouldDrawInnerCityPoi(poi.MapId, localMap->Id)) continue;
        const std::string label = InnerCityDisplayName(poi);
        const bool drawLabel =
            drawLocalText && ShouldDrawInnerCityLabel(poi, label, localMap->NameUtf8);
        if (!drawLabel) continue;

        ImVec2 screenPos = renderPixelToScreen(localPoiRenderCenter(poi, viewState), viewState,
                                               worldX0, worldY0, worldW, worldH);
        Texture *markerTex = GetMarkerTexture(poi);
        ImVec2 iconSize = localIconScreenSize(poi, sx, sy);
        const bool isCityOrFortress = IsCityOrFortressTexture(poi);
        bool isCity = false;
        if (isCityOrFortress && !IsFortressLabel(label)) {
          isCity = true;
        }
        const ImU32 textColor = ResolveInnerCityLabelColor(poi, label);
        drawLabeledMarker(screenPos, iconSize.x, iconSize.y, markerTex, IM_COL32(255, 245, 190, 245),
                          label, "", drawLabel, false, isCityOrFortress, textColor,
                          poi.IsStrong != 0, isCity, labelScale, 18.0f);
      }

      if (m_hasPlayerPosition && !playerLocalMap && m_playerRy >= localMap->MinMx &&
          m_playerRy <= localMap->MaxMx && m_playerRx >= localMap->MinMy &&
          m_playerRx <= localMap->MaxMy) {
        playerLocalMap = localMap;
      }
    }

    if (m_hasPlayerPosition) {
      ImVec2 screenPos;
      float iconSize = (std::clamp)(16.0f * m_zoom, 12.0f, 28.0f);
      char tooltip[192];
      std::snprintf(tooltip, sizeof(tooltip),
                    "Player\nRegion: (%d, %d)\nLocal: (%.0f, %.0f)",
                    m_playerRx, m_playerRy, m_playerLocalX, m_playerLocalZ);

      if (playerLocalMap) {
        const sro::map::LocalMapWorldRect gridRect = ComputeLocalMapGridWorldRect(*playerLocalMap);
        const sro::map::LocalMapViewState viewState = localMapViewStateFromDef(*playerLocalMap);
        const sro::map::TexturePixelPos rp = sro::map::RegionLocalToRenderPixel(
            m_playerRx, m_playerRy, m_playerLocalX, m_playerLocalZ, viewState);
        screenPos = renderPixelToScreen(rp, viewState, gridRect.x0, gridRect.y0, gridRect.w,
                                        gridRect.h);
        const float sx = texToScreenScale(gridRect.w, static_cast<int>(viewState.renderWidth));
        iconSize = (std::clamp)(16.0f * sx, 12.0f, 28.0f);
      } else {
        sro::map::MapPixelPos mapPos = sro::map::RegionLocalToMapPixel(
            m_playerRx, m_playerRy, m_playerLocalX, m_playerLocalZ, m_minMx, m_maxMy);
        screenPos = worldToScreen(mapPos.x, mapPos.y);
      }

      if (m_playerMarkerTex && m_playerMarkerTex->pTexture) {
        MapPlayerIcon::DrawRotatedMapIcon(drawList,
            reinterpret_cast<ImTextureID>(m_playerMarkerTex->pTexture), screenPos, iconSize,
            m_playerHeadingRad);
      }
      if (mapHovered) {
        const float hitR = iconSize * 0.55f;
        const float dx = mousePos.x - screenPos.x;
        const float dy = mousePos.y - screenPos.y;
        if (dx * dx + dy * dy <= hitR * hitR) {
          ImGui::BeginTooltip();
          ImGui::TextUnformatted(tooltip);
          ImGui::EndTooltip();
        }
      }
    }
  }

  if (MapStyle == 2) {
    const InstanceMapDefinition *inst = nullptr;
    if (SelectedInstanceId > 0) inst = FindInstanceMapById(SelectedInstanceId);
    if (!inst) inst = FindInstanceMapForRegion(CurrentRx, CurrentRy);
    if (inst) {
      float worldX0 = MapToWorldX(static_cast<float>(inst->MinMx));
      float worldY0 = MapToWorldY(static_cast<float>(inst->MaxMy));
      float worldX1 = MapToWorldX(static_cast<float>(inst->MaxMx + 1));
      float worldY1 = MapToWorldY(static_cast<float>(inst->MinMy - 1));
      float worldW = worldX1 - worldX0;
      float worldH = worldY1 - worldY0;
      const float sx = texToScreenScale(worldW, inst->RenderWidth);
      const float sy = texToScreenScale(worldH, inst->RenderHeight);
      const float labelScale = (sx + sy) * 0.5f;
      for (const auto &poi : m_localPoiMarkers) {
        if (poi.MapId != inst->Id) continue;
        float px = static_cast<float>(poi.PixelX) + static_cast<float>(poi.IconWidth) * 0.5f;
        float py = static_cast<float>(poi.PixelY) + static_cast<float>(poi.IconHeight) * 0.5f;
        float nx = px / static_cast<float>((std::max)(1, inst->RenderWidth));
        float ny = py / static_cast<float>((std::max)(1, inst->RenderHeight));
        ImVec2 screenPos = worldToScreen(worldX0 + nx * worldW, worldY0 + ny * worldH);
        Texture *markerTex = GetMarkerTexture(poi);
        ImVec2 iconSize = localIconScreenSize(poi, sx, sy);
        drawLabeledMarker(screenPos, iconSize.x, iconSize.y, markerTex, IM_COL32(255, 245, 190, 245),
                          poi.NameUtf8, buildDetailedTooltip(poi), m_zoom >= 0.85f, false, false,
                          IM_COL32(245, 245, 230, 255), false, false, labelScale, 18.0f);
      }
      if (m_hasPlayerPosition && m_playerRx == inst->RegionRx && m_playerRy == inst->RegionRy) {
        float nx = sro::map::MinimapHudU(m_playerLocalX);
        float ny = sro::map::MinimapHudV(m_playerLocalZ);
        ImVec2 screenPos = worldToScreen(worldX0 + nx * worldW, worldY0 + ny * worldH);
        float iconSize = (std::clamp)(16.0f * sx, 10.0f, 28.0f);
        if (m_playerMarkerTex && m_playerMarkerTex->pTexture) {
          MapPlayerIcon::DrawRotatedMapIcon(drawList,
              reinterpret_cast<ImTextureID>(m_playerMarkerTex->pTexture), screenPos, iconSize, m_playerHeadingRad);
        }
        if (mapHovered) {
          const float hitR = iconSize * 0.55f;
          const float dx = mousePos.x - screenPos.x;
          const float dy = mousePos.y - screenPos.y;
          if (dx * dx + dy * dy <= hitR * hitR) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Player");
            ImGui::EndTooltip();
          }
        }
      }
    }
  }

  if (SelectedLocalMapId > 0) {
    const LocalMapDefinition *local = FindLocalMapById(SelectedLocalMapId);
    if (local) {
      const sro::map::LocalMapWorldRect rect = ComputeLocalMapWorldRect(*local);
      const float worldX0 = rect.x0;
      const float worldY0 = rect.y0;
      const float worldW = rect.w;
      const float worldH = rect.h;
      const sro::map::LocalMapViewState viewState = localMapViewStateFromDef(*local);
      const float sx = texToScreenScale(worldW, static_cast<int>(viewState.renderWidth));
      const float sy = texToScreenScale(worldH, static_cast<int>(viewState.renderHeight));
      const float labelScale = (sx + sy) * 0.5f;
      const bool drawLocalText = m_zoom >= 0.55f;

      // Pass 1: Draw all icons
      for (const auto &poi : m_localPoiMarkers) {
        if (!sro::map::ShouldDrawInnerCityPoi(poi.MapId, local->Id)) continue;
        if (!ShouldDrawInnerCityIconPass(poi)) continue;
        ImVec2 screenPos = renderPixelToScreen(localPoiRenderCenter(poi, viewState), viewState,
                                               worldX0, worldY0, worldW, worldH);
        Texture *markerTex = GetMarkerTexture(poi);
        ImVec2 iconSize = localIconScreenSize(poi, sx, sy);
        drawInnerCityMarkerIcon(screenPos, iconSize, markerTex, poi, InnerCityDisplayName(poi));
      }

      // Pass 2: Draw permanent labels only (MarkerKind==1)
      for (const auto &poi : m_localPoiMarkers) {
        if (!sro::map::ShouldDrawInnerCityPoi(poi.MapId, local->Id)) continue;
        const std::string label = InnerCityDisplayName(poi);
        const bool drawLabel =
            drawLocalText && ShouldDrawInnerCityLabel(poi, label, local->NameUtf8);
        if (!drawLabel) continue;

        ImVec2 screenPos = renderPixelToScreen(localPoiRenderCenter(poi, viewState), viewState,
                                               worldX0, worldY0, worldW, worldH);
        ImVec2 iconSize = localIconScreenSize(poi, sx, sy);
        const bool isCityOrFortress = IsCityOrFortressTexture(poi);
        bool isCity = false;
        if (isCityOrFortress && !IsFortressLabel(label)) {
          isCity = true;
        }
        const ImU32 textColor = ResolveInnerCityLabelColor(poi, label);
        drawLabeledMarker(screenPos, iconSize.x, iconSize.y, nullptr, IM_COL32(255, 245, 190, 245),
                          label, "", drawLabel, false,
                          isCityOrFortress, textColor, poi.IsStrong != 0, isCity, labelScale, 18.0f);
      }

      if (m_hasPlayerPosition &&
          m_playerRy >= local->MinMx && m_playerRy <= local->MaxMx &&
          m_playerRx >= local->MinMy && m_playerRx <= local->MaxMy) {
        const sro::map::TexturePixelPos rp = sro::map::RegionLocalToRenderPixel(
            m_playerRx, m_playerRy, m_playerLocalX, m_playerLocalZ, viewState);
        ImVec2 screenPos = renderPixelToScreen(rp, viewState, worldX0, worldY0, worldW, worldH);
        float iconSize = (std::clamp)(16.0f * sx, 10.0f, 28.0f);
        if (m_playerMarkerTex && m_playerMarkerTex->pTexture) {
          MapPlayerIcon::DrawRotatedMapIcon(drawList,
              reinterpret_cast<ImTextureID>(m_playerMarkerTex->pTexture), screenPos, iconSize, m_playerHeadingRad);
        }
        if (mapHovered) {
          const float hitR = iconSize * 0.55f;
          const float dx = mousePos.x - screenPos.x;
          const float dy = mousePos.y - screenPos.y;
          if (dx * dx + dy * dy <= hitR * hitR) {
            char tooltip[192];
            std::snprintf(tooltip, sizeof(tooltip),
                          "Player\nRegion: (%d, %d)\nLocal: (%.0f, %.0f)\nRender: (%.0f, %.0f)",
                          m_playerRx, m_playerRy, m_playerLocalX, m_playerLocalZ, rp.x, rp.y);
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(tooltip);
            ImGui::EndTooltip();
          }
        }
      }
    }
  }

  if (SelectedDungeonFloorId > 0) {
    const InstanceMapDefinition *floor = FindInstanceMapById(SelectedDungeonFloorId);
    if (floor) {
      const sro::map::LocalMapWorldRect rect = ComputeDungeonFloorWorldRect(*floor);
      const float worldX0 = rect.x0;
      const float worldY0 = rect.y0;
      const float worldW = rect.w;
      const float worldH = rect.h;
      const float sx = texToScreenScale(worldW, floor->RenderWidth);
      const float sy = texToScreenScale(worldH, floor->RenderHeight);
      const float labelScale = (sx + sy) * 0.5f;

      const std::string dungeonDisplayName = ResolveDungeonDisplayName(*floor);
      const bool drawDungeonText = m_zoom >= 0.45f;
      const float renderW = static_cast<float>((std::max)(1, floor->RenderWidth));
      const float renderH = static_cast<float>((std::max)(1, floor->RenderHeight));

      auto dungeonPoiScreenPos = [&](const LocalPoiMarker &poi) -> ImVec2 {
        const float px = static_cast<float>(poi.PixelX) + static_cast<float>(poi.IconWidth) * 0.5f;
        const float py = static_cast<float>(poi.PixelY) + static_cast<float>(poi.IconHeight) * 0.5f;
        return worldToScreen(worldX0 + px / renderW * worldW, worldY0 + py / renderH * worldH);
      };

      for (const auto &poi : m_localPoiMarkers) {
        if (poi.MapId != floor->Id) continue;
        if (!ShouldDrawInnerCityIconPass(poi)) continue;
        ImVec2 screenPos = dungeonPoiScreenPos(poi);
        Texture *markerTex = GetMarkerTexture(poi);
        ImVec2 iconSize = localIconScreenSize(poi, sx, sy);
        drawInnerCityMarkerIcon(screenPos, iconSize, markerTex, poi, DungeonPoiHoverTooltip(poi));
      }

      for (const auto &poi : m_localPoiMarkers) {
        if (poi.MapId != floor->Id) continue;
        const std::string label = InnerCityDisplayName(poi);
        if (!drawDungeonText || !ShouldDrawDungeonPermanentLabel(poi, label, dungeonDisplayName)) {
          continue;
        }
        ImVec2 screenPos = dungeonPoiScreenPos(poi);
        ImVec2 iconSize = localIconScreenSize(poi, sx, sy);
        const ImU32 textColor = ResolveInnerCityLabelColor(poi, label);
        drawLabeledMarker(screenPos, iconSize.x, iconSize.y, nullptr, IM_COL32(255, 245, 190, 245),
                          label, "", true, false, false, textColor, poi.IsStrong != 0, false,
                          labelScale, 18.0f);
      }

      const std::string title = GetDungeonViewTitle();
      const ImVec2 titleSize = ImGui::CalcTextSize(title.c_str());
      drawList->AddRectFilled(ImVec2(screenX, screenY), ImVec2(screenX + width, screenY + 26.0f),
                              IM_COL32(12, 12, 16, 210));
      drawList->AddText(ImVec2(screenX + width * 0.5f - titleSize.x * 0.5f, screenY + 5.0f),
                        IM_COL32(245, 245, 230, 255), title.c_str());

      const std::string floorLabel = GetDungeonFloorLabel(SelectedDungeonFloorId);
      const float panelX = screenX + width - 92.0f;
      const float panelY = screenY + 30.0f;
      drawList->AddRectFilled(ImVec2(panelX, panelY), ImVec2(panelX + 84.0f, panelY + 72.0f),
                              IM_COL32(8, 8, 10, 220));
      drawList->AddRect(ImVec2(panelX, panelY), ImVec2(panelX + 84.0f, panelY + 72.0f),
                        IM_COL32(80, 80, 90, 255));

      const ImVec2 labelSize = ImGui::CalcTextSize(floorLabel.c_str());
      drawList->AddText(ImVec2(panelX + 42.0f - labelSize.x * 0.5f, panelY + 26.0f),
                        IM_COL32(120, 180, 255, 255), floorLabel.c_str());
    }
  }

  drawList->PopClipRect();
}

std::string WorldMapControl::GetRegionName(int rx, int ry, bool preferEnglish) const {
  int regId = (rx << 8) | ry;
  auto it = m_regionNames.find(regId);
  if (it != m_regionNames.end()) {
    std::string name = preferEnglish ? it->second.second : it->second.first;
    if (name.empty()) {
      name = preferEnglish ? it->second.first : it->second.second;
    }
    if (!name.empty() && name != "0" && name != "xxx") {
      return name;
    }
  }
  
  // Try fallback translation lookup directly using UI/zonename SN keys if SnKey Sn_Zone_... is built
  // SRO region SN keys are sn_zone_xx_yy
  char zoneKey[64];
  sprintf_s(zoneKey, "SN_ZONE_%02X_%02X", rx, ry);
  std::string zoneKeyStr = zoneKey;
  std::wstring wideKey(zoneKeyStr.begin(), zoneKeyStr.end());
  wideKey = StripSpaces(wideKey);
  auto itTrans = m_translations.find(wideKey);
  if (itTrans != m_translations.end() && !itTrans->second.empty()) {
    return ToUtf8FromWide(itTrans->second);
  }

  // Fallback to coordinates
  char buf[32];
  sprintf_s(buf, "Region (%d, %d)", rx, ry);
  return buf;
}

