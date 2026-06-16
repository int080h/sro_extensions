# ISRO World Map / Minimap Reverse Engineering Notes

Binary: `C:\Users\alpka\Desktop\Game\debug\sro_client.exe` (ISRO, 32-bit)  
IDA database: `sro_client.exe.i64`

## Renamed functions (IDA)

| Address | Name | Role |
|---------|------|------|
| `0x774280` | `CWorldMapView_LoadWorldTiles` | Loads `interface\worldmap\map\map_world_%dx%d.ddj` grid |
| `0x577030` | `CMinimapWidget_LoadRegionTiles` | In-game minimap HUD tile streaming |
| `0x772140` | `CWorldMapView_UpdatePlayerPosition` | World-map player icon position |

## Asset paths (verified strings)

| Path | Usage |
|------|--------|
| `interface\worldmap\map\map_world_%dx%d.ddj` | World map tiles (128×128 px, 4×4 regions) |
| `interface\worldmap\map\map_*.ddj` | City / fortress inner maps (e.g. `map_jangan.ddj`) |
| `interface\worldmap\map\map_return.ddj` | Back button from inner city map to world map |
| `interface\worldmap\monster\%s_%dx%d.ddj` | Monster density overlay |
| `minimap\` | Field minimap tiles `%d x %d.ddj` (on-disk: `ryxrx.ddj`, **no spaces**) |
| `minimap_d\%s\%s_` | Dungeon minimap prefix + `%s%dx%d.ddj` (on-disk: no spaces around `x`) |
| `interface\worldmap\wmap_sign_*.ddj` | World map marker icons |
| `interface\minimap\mm_sign_*.ddj` | Minimap HUD marker icons |

## Textdata (via `CPS_LoadGameTextData` @ `0x94419f`)

| File | Class |
|------|-------|
| `%stextdata\wlocalmap.txt` | `CWorldMap_MapInfo` (city inner map defs: texture path, bounds) |
| `%stextdata\worldmap_localinfo.txt` | `CWorldMap_LocalInfo` |
| `%stextdata\worldmap_instanceinfo.txt` | `CWorldMap_InstanceInfo` |
| `%stextdata\worldmapguidedata_region.txt` | `CWorldMapGuideData` |

Note: ISRO client loads **`wlocalmap.txt`** for inner city maps (`CWorldMap_MapInfo`). `worldmap_mapinfo.txt` is the server_dep equivalent with the same columns; editor tools try `wlocalmap.txt` first, then `worldmap_mapinfo.txt`, under both `Media/textdata/` and `Media/server_dep/silkroad/textdata/`.

## World map tile grid (`CWorldMapView_LoadWorldTiles`)

```c
for (mx = 46; mx <= 180; mx += 4)
  for (my = 113; my >= 69; my -= 4)
    sprintf(path, "interface\\worldmap\\map\\map_world_%dx%d.ddj", mx, my);
```

- Each tile covers regions `[mx..mx+3] × [my-3..my]` (4×4 = 16 regions)
- Tile draw size: **128×128 px** → **32 px per region**
- Some client packs ship extra `map_world_%dx%d.ddj` files at non-step-4 anchors (e.g. `65x85`); **ISRO ignores them** — only the loop above is loaded/drawn
- `mapinfo.mfo` dims inactive **cells** on top of drawn tiles; it does not skip loading whole mega-tiles

## Coordinate systems

### Simple / display coords (`CWorldMapView_UpdatePlayerPosition`)

Region ID is a WORD: low byte = Ry, high byte = Rx.

```
simpleX = 192.0f * (Rx - 92)  + localX / 10.0f
simpleZ = 192.0f * (Ry - 135) + localZ / 10.0f
```

Matches `RenderManager::LocalToWorld` in tools.

### Map grid display

```
displayMx = Ry
displayMy = Rx
mapWorldX = (displayMx - minMx) * 32
mapWorldY = (maxMy - displayMy) * 32   // Y grows downward on screen
```

### Scene / engine coords

- Region size in scene space: **1920** units
- Region streaming offsets use Ry for X, Rx for Z (see `SceneSpace`)

### Minimap HUD (`CMinimapWidget_LoadRegionTiles`)

- Loads **3×3** neighborhood around player region
- Path: `minimap\%d x %d.ddj` in client sprintf (display); **on-disk filenames** are `ryxrx.ddj` without spaces (e.g. `167x97.ddj`)
- Indices `(ry + offset - 1, rx - offset + 1)`
- Player dot UV: `localPos / 1920.0f * widgetSize`
- Dungeons (region WORD < 0): `minimap_d\{dungeon}\{prefix}_%d x %d.ddj` (on-disk: `{prefix}{ry}x{rx}.ddj`, no spaces)

## Client class hierarchy (RTTI)

- `CWorldMapViewController` — orchestrates map view
- `CNIFWorldMapView` / `CIFWorldMap` — UI layers
- `CWorldMap_MapInfo`, `CWorldMap_LocalInfo`, `CWorldMap_InstanceInfo` — data tables
- Colleague markers: `CWorldMapPlayerColleague`, `CWorldMapPartyColleague`, `CWorldMapQuestNPCColleague`, etc.

## `mapinfo.mfo` / ActiveRegions

String `mapinfo.mfo` @ `0x10440a4`. Client uses active region bitmap to hide ocean / unused cells on world map (per-cell overlay). Tools draw all ISRO-grid mega-tiles and dim inactive cells; use `ActiveRegions` for click/travel filtering.

## Tool alignment checklist

- [x] Tile path and grid loop match client
- [x] 32 px/region, 128 px/world tile
- [x] Region axis swap (displayMx=Ry, displayMy=Rx)
- [x] ActiveRegions per-cell dimming + click filter when MFO loaded (not mega-tile skip)
- [x] `worldmap_instanceinfo.txt` parsing
- [x] Live player marker (simple coord formula)
- [x] Separate MinimapWidget (3×3, 1920-based HUD) vs world map panel
- [x] `wlocalmap.txt` / `worldmap_mapinfo.txt` local map defs + `map_*.ddj` inner textures
- [x] City icon click drill-down with Back / Escape to world map

## `wlocalmap.txt` columns (`CWorldMap_MapInfo`)

Tab-separated. Jangan example:

`1  1  장안  interface\worldmap\map\map_jangan.ddj  1024  1024  1024  768  1024  768  166  99  170  96  ...`

Hotan example (coord space differs from render — POI `PixelX`/`PixelY` use cols 8–9):

`3  1  호탄  interface\worldmap\map\map_khotan.ddj  512  1024  512  838  640  1047  134  95  136  91  ...`

Constantinople example (texture wider than render — horizontal UV left-align):

`5  1  콘스탄티노플  interface\worldmap\map\map_constantinople.ddj  1024  1024  890  1024  890  1024  77  108  81  102  ...`

| Cols | Field | Meaning |
|------|-------|---------|
| 0 | Id | Local map ID |
| 2 | Name | Display name |
| 3 | Path | DDJ under `Media/` |
| 4–5 | TextureWidth/Height | Full DDJ bitmap size (e.g. 1024×1024) |
| 6–7 | RenderWidth/Height | Active map pixel area for **UV crop** and world scale (`sub_6FFBA0` / `sub_6FD9C0`) |
| 8–9 | CoordWidth/Height | **POI coordinate space only** — `worldmap_localinfo` PixelX/Y; scaled to render before placement |
| 10–13 | mx0,my0,mx1,my1 | Region bounds → **Ry** min/max, **Rx** min/max |

Inner-city POI positions: `worldmap_localinfo` PixelX/Y are in **coord space** (cols 8–9; defaults to render when unset). Tools scale `renderX = pixelX * RenderWidth / CoordWidth` before mapping to the screen quad. Live player positions use **render-space** world scale (`worldSpan / RenderWidth|Height`). DDJ UV crop uses **render** vs texture only: **left/top-aligned** when texture exceeds render (`u ∈ [0, RenderW/TexW]`, `v ∈ [0, RenderH/TexH]`; Jangan vertical: `vMax = 768/1024`; Constantinople horizontal: `uMax = 890/1024`). Inner-city map quad height uses render aspect (`worldH = worldW * RenderHeight / RenderWidth`), north-anchored. **No region grid** on inner-city drill-down (ISRO client). Inner-city POI overlay matches `MapId` to `wlocalmap.txt` Id; `worldmap_localinfo.txt` cols 9–10 are always **TargetRy, TargetRx** (same order as world rows).

### Inner-city labels and tooltips (editor)

`worldmap_localinfo.txt` col 2 (`MarkerKind`):

| MarkerKind | Behavior |
|------------|----------|
| `1` | Always-on label when display name is meaningful |
| Other | Icon or green pin only; display name on **hover** |

Display name for labels and hover: **col 5 `Name`** (translated via textdata keys), then col 4 `AreaName` as fallback. Col 4 is the parent city/zone (e.g. "Jangan") — not the shop label. Permanent labels are suppressed when the resolved text would only repeat the inner map name from `wlocalmap.txt`.

`ShouldShowInnerCityPermanentLabel` / `ShouldDrawInnerCityPermanentLabel` in [`WorldMapCoords.h`](tools/shared/sro/WorldMapCoords.h) encode this policy.

### Inner-city label colors (editor)

POI file RGB is not used directly. `ResolveInnerCityLabelColor` mirrors the world-map pass:

| Condition | Color |
|-----------|-------|
| `city_` / `fort_` texture, non-fortress name | Yellow `(255, 247, 182)` |
| `city_` / `fort_` texture or label contains Fortress | Gold `(255, 225, 80)` |
| `MarkerKind == 1` (area / zone label) | Yellow |
| Shop / NPC with icon texture | White `(245, 245, 230)` |
| Else | `ColorR/G/B` from `worldmap_localinfo.txt` cols 15–17 (R, G, B per column) |

`worldmap_localinfo.txt` lists **R, G, B** in separate tab columns. The client also uses packed **BGR** `COLORREF` (`0x00BBGGRR`) in memory — swap R/B only when decoding a DWORD, not these text columns.

### Live player on inner city (`RegionLocalToRenderPixel`)

Uses Ry→simpleX, Rx→simpleZ (same as `SceneSpace::LocalToWorld`). Render Y: `RenderHeight - (simpleZ - worldZSouth) / scaleY`.

Jangan South Gate POI: `TargetRx=96`, `TargetRy=168`, `PixelY=700` → render center **y≈708**. Player at gate entrance (`rx=96`, `localZ≈200`) lands **y≈748** (within ~40px of the POI). Standing just inside the city often reports **rx=97**, which places the dot north of the gate POI on row 96 — expected if viewport region row differs from POI art row, not a Y-flip bug. Inner-map player tooltip shows region, local coords, and computed render pixel for comparison.

