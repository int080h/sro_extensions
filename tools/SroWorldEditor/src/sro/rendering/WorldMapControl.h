#pragma once
#include <d3d9.h>
#include <optional>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include "Rendering/Texture.h"
#include "Core/Math.h"
#include "Parsers/DdjParser.h"
#include "sro/WorldMapCoords.h"
#include "sro_map/MinimapTileIndex.h"
#include "sro_map/MapTextureCache.h"

// ============================================================================
// 2D Map Viewport Controller — world map, minimaps, POI markers, overlays
// ============================================================================

class WorldMapControl {
public:
  // 0 = World Map, 1 = Region Minimaps, 2 = Instance / Dungeon maps
  int MapStyle = 0;
  int CurrentRx = 97;
  int CurrentRy = 167;
  int SelectedInstanceId = 0;
  int SelectedLocalMapId = 0;
  int SelectedDungeonFloorId = 0;
  std::set<std::pair<int, int>> ActiveRegions;

  struct ColleagueMarker {
    int Type = 0;
    int Rx = 0;
    int Ry = 0;
    float LocalX = 0.0f;
    float LocalZ = 0.0f;
    std::string LabelUtf8;
  };
  std::vector<ColleagueMarker> ColleagueMarkers;

  struct WorldGuideMarker {
    std::string NameUtf8;
    std::string CodeName;
    std::string MapGroup;
    int PixelX = 0;
    int PixelY = 0;
    int Rx = 0;
    int Ry = 0;
    int LocalX = 0;
    int LocalY = 0;
    std::string IconToken;
  };

  struct LocalMapDefinition {
    int Id = 0;
    std::string NameUtf8;
    std::wstring TexturePath;
    int TextureWidth = 0;
    int TextureHeight = 0;
    int RenderWidth = 0;
    int RenderHeight = 0;
    int CoordWidth = 0;
    int CoordHeight = 0;
    int MinMx = 0;
    int MaxMx = 0;
    int MinMy = 0;
    int MaxMy = 0;
    int RawMx0 = 0;
    int RawMy0 = 0;
    int RawMx1 = 0;
    int RawMy1 = 0;
  };

  struct InstanceMapDefinition {
    int Id = 0;
    std::string NameUtf8;
    std::wstring TexturePath;
    int TextureWidth = 0;
    int TextureHeight = 0;
    int RenderWidth = 0;
    int RenderHeight = 0;
    int CoordWidth = 0;
    int CoordHeight = 0;
    int RegionRx = 0;
    int RegionRy = 0;
    int MinMx = 0;
    int MaxMx = 0;
    int MinMy = 0;
    int MaxMy = 0;
    std::wstring DungeonFolder;
    std::wstring TilePrefix;
    std::string DungeonGroupKey;
    char FloorKind = 'F';
    int FloorIndex = 0;
    int FloorCount = 0;
    int TileCols = 0;
    int TileRows = 0;
    bool IsTiled = false;
    std::wstring TilePrefixPath;
  };

  struct LocalPoiMarker {
    int MapId = 0;
    int LinkedDungeonId = 0;
    std::string AreaNameUtf8;
    std::string NameUtf8;
    std::string ZoneCode;
    std::wstring TexturePath;
    int TargetRx = -1;
    int TargetRy = -1;
    int PixelX = 0;
    int PixelY = 0;
    int IconWidth = 0;
    int IconHeight = 0;
    int MarkerKind = 0;
    int ColorR = 255;
    int ColorG = 255;
    int ColorB = 255;
    int IsStrong = 0;
    std::string NameKoUtf8;
    std::string AreaKoUtf8;
    std::string TeleportLabelUtf8;
  };

  float m_zoom = 1.0f;
  Vector2 m_panOffset = Vector2(0.0f, 0.0f);

  WorldMapControl(LPDIRECT3DDEVICE9 device, TextureManager* texMgr, MapTextureCache* mapCache);
  ~WorldMapControl() = default;

  void ScanRegionFiles(const std::wstring& clientPath);
  void Tick(float viewportW, float viewportH);
  void SetMinimapIndexListener(std::function<void()> listener);
  void SetMinimapHudActive(bool active, int centerRx = 0, int centerRy = 0);

  bool IsMinimapIndexComplete() const { return m_minimapIndexComplete; }
  float MinimapIndexProgress() const;
  int PendingTextureLoads() const { return m_pendingTextureLoads; }
  float WorldPreloadProgress() const;
  bool IsWorldPreloadComplete() const { return m_worldPreloadComplete; }

  bool IsMapRenderDirty() const { return m_mapRenderDirty; }
  void MarkMapRenderDirty() { m_mapRenderDirty = true; }
  void ClearMapRenderDirty() { m_mapRenderDirty = false; }

  int BoundsMinMx() const { return m_minMx; }
  int BoundsMaxMy() const { return m_maxMy; }

  void TickBackground();

  std::optional<std::wstring> PathForMinimapRegion(int rx, int ry) const;
  const MinimapTileIndex& GetMinimapIndex() const { return m_minimapIndex; }
  MapTextureCache* GetMapTextureCache() const { return m_mapCache; }

  void CenterOnRegion(int rx, int ry, float width, float height);
  void SetDragging(bool dragging);
  void NotifyPanOrZoom();
  void RefreshStyleBounds();
  const LocalMapDefinition* FindLocalMapForRegion(int rx, int ry) const;
  const LocalMapDefinition* FindLocalMapById(int id) const;
  const LocalMapDefinition* FindLocalMapForPoi(const LocalPoiMarker& poi) const;
  std::vector<const LocalMapDefinition*> FindLocalMapsInViewport(int minMx, int maxMx, int minMy,
                                                                 int maxMy) const;
  const InstanceMapDefinition* FindInstanceMapForRegion(int rx, int ry) const;
  const InstanceMapDefinition* FindInstanceMapById(int id) const;
  const std::vector<InstanceMapDefinition>& GetInstanceMapDefinitions() const { return m_instanceMapDefs; }
  const std::vector<LocalMapDefinition>& GetLocalMapDefinitions() const { return m_localMapDefs; }
  bool IsInLocalMapView() const { return SelectedLocalMapId > 0; }
  bool IsInDungeonMapView() const { return SelectedDungeonFloorId > 0; }

  void OpenLocalMap(int mapId, float viewportW, float viewportH);
  void CloseLocalMap();
  bool OpenDungeonMap(int floorId, float viewportW, float viewportH);
  void CloseDungeonMap();
  void SetDungeonFloor(int floorId, float viewportW, float viewportH);
  bool CycleDungeonFloor(int direction, float viewportW, float viewportH);
  std::vector<int> FindDungeonFloorIds(int firstFloorId) const;
  std::string GetDungeonFloorLabel(int floorId) const;
  std::string GetDungeonViewTitle() const;
  std::string GetInstanceComboLabel(const InstanceMapDefinition& inst) const;
  std::optional<int> HitTestWorldPoi(const Vector2& screenPos, float width, float height) const;
  std::optional<int> HitTestDungeonEntrance(const Vector2& screenPos, float width, float height) const;
  bool HitTestAnyWorldMapPoi(const Vector2& screenPos, float width, float height) const;

  void TranslateScreenToMap(const Vector2& screenPos, int& mx, int& my, float width, float height) const;
  void TranslateScreenToRegion(const Vector2& screenPos, int& rx, int& ry, float width, float height) const;
  bool TryTranslateScreenToActiveRegion(const Vector2& screenPos, int& rx, int& ry, float width, float height) const;
  void SetPlayerPosition(int rx, int ry, float localX, float localZ, float headingRad = 0.0f);
  void ClearPlayerPosition();

  void HandleMouseInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, float width, float height, bool& regionDoubleClicked, int& clickedRx, int& clickedRy);
  void Render2D(float width, float height);
  void RenderImGuiOverlays(float screenX, float screenY, float width, float height, bool mapHovered);
  void InvalidateOverlayTextures();
  std::string GetRegionName(int rx, int ry, bool preferEnglish = true) const;

private:
  static constexpr int kRegionGridSize = 256;

  LPDIRECT3DDEVICE9 m_device;
  std::wstring m_clientPath;
  TextureManager* m_texManager;
  MapTextureCache* m_mapCache;

  std::map<std::pair<int, int>, std::wstring> m_worldTiles;
  std::map<std::pair<int, int>, std::wstring> m_worldOverlayTiles;
  MinimapTileIndex m_minimapIndex;
  std::vector<WorldGuideMarker> m_worldGuideMarkers;
  std::vector<LocalMapDefinition> m_localMapDefs;
  std::vector<InstanceMapDefinition> m_instanceMapDefs;
  std::vector<LocalPoiMarker> m_localPoiMarkers;
  std::map<std::wstring, std::wstring> m_translations;
  std::map<int, std::pair<std::string, std::string>> m_regionNames;
  std::set<std::pair<int, int>> m_worldCoveredRegions;

  Texture* m_worldMonsterIconTex = nullptr;
  Texture* m_localPoiIconTex = nullptr;
  Texture* m_playerMarkerTex = nullptr;
  Texture* m_partyMarkerTex = nullptr;
  Texture* m_questNpcMarkerTex = nullptr;

  bool m_hasPlayerPosition = false;
  int m_playerRx = 0;
  int m_playerRy = 0;
  float m_playerLocalX = 0.0f;
  float m_playerLocalZ = 0.0f;
  float m_playerHeadingRad = 0.0f;

  int m_minMx = 0;
  int m_maxMx = 255;
  int m_minMy = 0;
  int m_maxMy = 255;
  int m_worldMinMx = 46;
  int m_worldMaxMx = 180;
  int m_worldMinMy = 70;
  int m_worldMaxMy = 113;
  int m_minimapMinMx = 46;
  int m_minimapMaxMx = 180;
  int m_minimapMinMy = 70;
  int m_minimapMaxMy = 116;

  Vector2 m_lastMousePos = Vector2(0.0f, 0.0f);
  bool m_isDragging = false;
  bool m_hasCenteredInitialRegion = false;
  DWORD m_lastPanOrZoomTick = 0;

  bool m_minimapIndexComplete = false;
  std::vector<std::wstring> m_minimapScanDirs;
  size_t m_minimapScanDirIndex = 0;
  HANDLE m_minimapScanHandle = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATAW m_minimapScanFd{};
  size_t m_minimapIndexFilesTotal = 0;
  size_t m_minimapIndexFilesProcessed = 0;
  int m_pendingTextureLoads = 0;
  bool m_minimapHudActive = false;
  int m_hudCenterRx = 97;
  int m_hudCenterRy = 167;
  float m_lastViewportW = 0.0f;
  float m_lastViewportH = 0.0f;
  std::function<void()> m_minimapIndexListener;

  std::vector<std::wstring> m_worldPreloadPaths;
  size_t m_worldPreloadIndex = 0;
  bool m_worldPreloadComplete = false;
  bool m_worldPreloadLogged = false;
  bool m_mapRenderDirty = true;

  bool m_hasSavedWorldView = false;
  Vector2 m_savedPanOffset = Vector2(0.0f, 0.0f);
  float m_savedZoom = 1.0f;

  void BeginWorldPreload();
  void ParseLocalMapDefinitionLines(std::wifstream& fs);
  void FitLocalMapInViewport(const LocalMapDefinition& local, float viewportW, float viewportH);
  void TickWorldPreload(int maxPerTick);
  Texture* AcquireMapTexture(const std::wstring& path, MapLoadPolicy policy = MapLoadPolicy::Throttled);
  void ScanWorldOverlayTiles();
  void BeginMinimapIndexScan();
  bool TickMinimapIndexScan(int maxFilesPerChunk);
  bool RegisterMinimapFile(const std::wstring& dirPath, const std::wstring& filename);
  void LoadWorldGuideMarkers();
  void LoadWorldMapDefinitions();
  void LoadDungeonMapDefinitions();
  void LoadInstanceMapDefinitions();
  void LoadLocalPoiMarkers();
  void LinkDungeonEntrances();
  bool TryOpenTextDataFile(const wchar_t* relativePath, std::wifstream& out) const;
  bool TryOpenAsciiTextDataFile(const wchar_t* relativePath, std::ifstream& out) const;
  void EnsureOverlayTextures();
  void LoadTranslations();
  void ApplyBoundsForCurrentStyle();
  static bool IsCityOrFortressPoi(const LocalPoiMarker& poi);
  static bool IsDungeonEntrancePoi(const LocalPoiMarker& poi);
  sro::map::LocalMapWorldRect ComputeDungeonFloorWorldRect(const InstanceMapDefinition& floor) const;
  void FitDungeonFloorInViewport(const InstanceMapDefinition& floor, float viewportW, float viewportH);
  std::string ResolveDungeonDisplayName(const InstanceMapDefinition& floor) const;
  std::wstring DungeonTilePath(const InstanceMapDefinition& floor, int mx, int my) const;
  bool IsPoiTextureLoaded(const LocalPoiMarker& marker) const;
  Texture* GetMarkerTexture(const LocalPoiMarker& marker) const;
  Texture* GetColleagueTexture(int type) const;
  bool IsRegionActive(int rx, int ry) const;
  bool IsTileRegionActive(int mx, int my) const;
  size_t CountActiveWorldTiles() const;
  void QueueVisibleLoads(float width, float height);

  void ScanWorldTiles();
  float MapToWorldX(float mx) const;
  float MapToWorldY(float my) const;
  sro::map::LocalMapWorldRect ComputeLocalMapWorldRect(const LocalMapDefinition& local) const;
  sro::map::LocalMapWorldRect ComputeLocalMapGridWorldRect(const LocalMapDefinition& local) const;
  void RegionToDisplay(int rx, int ry, int& displayX, int& displayY) const;
  void DisplayToRegion(int displayX, int displayY, int& rx, int& ry) const;
};
