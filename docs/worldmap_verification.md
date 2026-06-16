# World Map Verification Checklist (ISRO vs Tools)

Use this checklist for side-by-side validation against the ISRO client.

## Setup

1. Load the same client `Media` folder in **SroWorldEditor**.
2. Import `mapinfo.mfo` so `ActiveRegions` is populated.
3. Open world map at **Jangan (97, 167)** in both client and tool.

## Performance / robustness (SroWorldEditor)

| Check | Expected |
|-------|----------|
| Press **M** at Jangan — panel appears in &lt;100 ms | |
| No log errors for `167 x 97.ddj` (spaces) — on-disk names are `167x97.ddj` | |
| World map tiles stream in over ~1–2 s (background preload, no UI freeze on **M**) | |
| Region Minimaps mode pans/zooms without building full atlas | |
| Corner HUD shows 3×3 minimap at Jangan (97, 167) | |
| Corner HUD: enable checkbox on World Map — terrain appears within ~1–2 s after index (not arrow-only) | |
| Corner HUD: walk across region boundary — 3×3 tiles update (ImGui foreground layer, not RTT-baked) | |
| Corner HUD: terrain drawn via ImGui `AddImage` on foreground (not composited into map RTT) | |
| Corner HUD: while indexing, shows bordered placeholder + “Minimap…” / % (not blank) | |
| Corner HUD: auto-on for Region Minimaps and dungeons; off by default on World Map | |
| Corner HUD: click inset recenters main map on player; caption shows Ry,Rx and local coords | |
| Corner HUD: 3×3 neighbors align (col=Ry east, row=Rx north); no jigsaw at Jangan (97, 167) | |
| Corner HUD: walk north/south — player arrow moves same direction as on Region Minimaps player marker | |
| Region Minimaps: no world mega-tile underlay; Jangan walls/gates align across 32 px cells | |
| Region Minimaps: no vertical seam / torn column through city when zoomed on Jangan | |
| Region Minimaps: inactive `mapinfo.mfo` cells still show minimap art when DDJ exists (no ActiveRegions holes) | |
| Region Minimaps: player arrow icon visible at current position (same as World Map) | |
| Region Minimaps: city/fortress/dungeon entrance icons visible on minimap grid | |
| Region Minimaps: inner-city shop icons align with walls when zoomed on Jangan (no icons in black void) | |
| Instance / Dungeon combo: readable English names with floor suffix (not `????`) | |
| `xy_mon.ddj` POI misses are harmless (optional marker icons) | |
| World terrain tiles visible under POI icons (map renders to offscreen RTT, displayed via `ImGui::Image` — same pattern as Viewport) | |
| World map: all mega-tiles preloaded in background (~2 s after import); log `[WorldMapControl] World tiles preloaded: N/N` | |
| World map: no persistent checkerboard holes after preload completes | |
| Region Minimaps: pan away from player — viewport center fills first (burst load after pan/zoom) | |
| Region Minimaps: unloaded cells show dim placeholder quads, not pure black holes | |
| Map panel idle: RTT skipped when nothing changed (dirty-gated redraw) | |
| Map DDJ load failures log as `[MapTextureCache]`, not `[TextureManager]` with spaced paths | |
| Rebuild via `tools/SroWorldEditor/build.bat` before testing | |

## Jangan (97, 167) — World Map
|-------|-------------|------|
| World tile alignment at Jangan | | |
| City POI icon position | | |
| Player marker on world map | | |
| Inactive ocean cells hidden/dimmed | | |
| Region grid 32 px per cell | | |
| Double-click travel lands on correct region | | |

## Jangan (97, 167) — City Inner Map

| Check | ISRO client | Tool |
|-------|-------------|------|
| Log loads `wlocalmap.txt` (or `worldmap_mapinfo.txt` fallback) with local defs | | |
| City POI icon on world map | | |
| Single-click city icon opens `map_jangan.ddj` inner map | | |
| Inner-map NPC / gate POIs from `worldmap_localinfo.txt` | | |
| **Back to World Map** / Escape restores prior world viewport | | |
| Player dot on inner map when standing in Jangan | | |
| Inner city drill-down (Hotan, Constantinople, Samarkand): shop icons on map art; labels not clipped at panel edge | | |

## Qin-Shi Tomb (102, 172) — Cave Drill-down

After changing world-map code, rebuild with `tools/SroWorldEditor/build.bat` and **restart SroWorldEditor** (or re-import the client) so `ScanRegionFiles` reloads metadata. On startup the Console should log something like `Loaded 26 dungeon map definitions from dungeonmap.txt` and `Dungeon metadata: 26 floor defs, N cave entrances`.

| Check | ISRO client | Tool |
|-------|-------------|------|
| Cave `xy_dungeon.ddj` icon at entrance | | |
| Hover shows hand cursor + “Click to open dungeon map” | | |
| Single-click opens underground map (B1 / floor 2005) | | |
| Tiled `map_world_jinf01_*` art visible | | |
| Title: “Underground Level 1 of …” (English via translation table when available) | | |
| Sidebar **Previous/Next floor** or top-right **B1** selector; **^** / **v** or **PageUp** / **PageDown** cycles B2–B6 | | |
| **W** / Back / Escape restores world map viewport | | |
| Dunhuang cave (147, 106) opens F1–F4 floors | | |

### Per-floor POI placement (Qin-Shi Tomb)

After rebuild, cycle through B1–B6 and confirm icons/labels sit on map features (not in empty margins):

| Floor | Check | ISRO client | Tool |
|-------|-------|-------------|------|
| B1 | Exit gate + down-stair teleports on corridors | | |
| B2 | Treasure room labels (`MarkerKind==1`) on rooms | | |
| B3 | Corner teleports on doorways | | |
| B4 | Teleport icons aligned with passages | | |
| B5 | Room names (e.g. Room of Ordeal) on rooms | | |
| B6 | Teleport icons on map art | | |
| All | Hover shows `STORE_QT_GATE_*` English route tooltips | | |
| All | Teleport icons use per-POI texture from `worldmap_localinfo` col 3 (`icontel` / `iconwf`) | | |

## Jangan Cave (97, 160) — Instance / Dungeon

| Check | ISRO client | Tool |
|-------|-------------|------|
| Instance map texture visible (Map Style: Instance) | | |
| Instance POI markers | | |
| `minimap_d` 3×3 HUD in corner | | |
| Player dot at local/1920 position | | |

## Coordinate unit tests

Run `SroCoreTests.exe` — validates:

- `SceneLocalToSimpleWorld` @ sub_772140 formula
- Region display axis swap (Ry→Mx, Rx→My)
- Map pixel round-trip for Jangan center
- Dungeon floor grouping (`2005` → six Qin-Shi floors) and `B1`/`F2` labels
- Dungeon tile scale vs render size (B1=1.0, B2=0.5, B3=0.25)
- `MinimapHudVScreen` north-up dot (0→1, 1920→0)
- `LocalMapWorldRectGridAligned` height = rows×32; texture-space POI Y normalization

## Known acceptable differences

- Tool uses ImGui panel, not `CNIFWorldMapView` / `.2dt` UI assets.
- `worldmap_mapinfo.txt` may only exist under `server_dep`; client uses `wlocalmap.txt` for the same data.
