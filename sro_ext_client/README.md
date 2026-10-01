# Silkroad Client Extension (`ext_client`)

Modern C++20 extension DLL for the Silkroad Online client (`sro_client.exe`).
Designed with strict adherence to the native game engine architecture — no memory hacks, dirty tricks, or fragile hooks.

---

## 1. Quick Start & Build Workflows

Build with Visual Studio Desktop C++ workload (MSVC toolset v143/v145), 32-bit x86 target:

```powershell
# Build Release with full validation checks
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Configuration Release -Check

# Quick Debug build
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Configuration Debug

# Quick Release build
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Configuration Release
```

Output binaries:
- `../Output/Release/ext_client.dll` & `ext_client.pdb`
- `../Output/Checks/client_checks.exe` (automated test suite)

### Deployment
To deploy to the game directory:
```cmd
if exist Z:\E\Workspace\Silkroad\RIGID_v234\ext_client.dll move /y Z:\E\Workspace\Silkroad\RIGID_v234\ext_client.dll Z:\E\Workspace\Silkroad\RIGID_v234\ext_client.dll.old
copy /y ..\Output\Release\ext_client.dll Z:\E\Workspace\Silkroad\RIGID_v234\ext_client.dll
copy /y ..\Output\Release\ext_client.pdb Z:\E\Workspace\Silkroad\RIGID_v234\ext_client.pdb
if exist Z:\E\Workspace\Silkroad\RIGID_v234\ext_client.dll.old del /q Z:\E\Workspace\Silkroad\RIGID_v234\ext_client.dll.old
```
*(Windows file-move trick allows replacing `ext_client.dll` even if the client process is actively running).*

---

## 2. Directory Structure & Architecture

```
sro_ext_client/
├── 10-Library/                  # Third-party libraries (ImGui, MinHook, detours)
├── 20-Program/
│   ├── core/                    # Engine core: plugin manager, config, event dispatch, detours
│   │   ├── hooks/               # Detour definitions (client_hooks, d3d_hooks, network_hooks)
│   │   ├── core_config.cpp/.hpp # Thread-safe configuration manager (atomic snapshots & debounce)
│   │   ├── core_event_manager   # High-performance pub/sub event system
│   │   └── core_plugin_manager  # Dynamic plugin lifecycle registry
│   ├── plugins/                 # High-level gameplay/UI features (completely decoupled)
│   │   ├── assert/              # Bypasses fatal assertion aborts on packet overflows
│   │   ├── character_select/    # Logout time elapsed formatting & deco animation trigger
│   │   ├── hud_customizer/      # In-game HUD promo/guide visibility toggles
│   │   ├── net_log/             # Comprehensive packet inspector, filter, and hex viewer
│   │   ├── quest/               # Fallback dummy definitions to prevent missing-quest crashes
│   │   ├── target_window/       # Special mob target window HP% overlays & rank icons
│   │   ├── title/               # Login screen layout, custom DDJ background, logo alignment
│   │   ├── version_check/       # Loading screen banner and camera controls
│   │   └── welcome_msg/         # Customizable notice messages
│   ├── render/                  # DirectX 9 ImGui integration, overlay render loop & menus
│   ├── sdk/                     # Strongly-typed reverse-engineered game engine views
│   │   ├── game/                # Game world, entities, controllers (CEntityManager, CICPlayer, etc.)
│   │   ├── net/                 # Packet streams & net engine (cmsg_stream_buffer, packet_builder)
│   │   ├── process/             # Game states (CProcess, CPSTitle, CPSCharacterSelect, CPSSilkroad)
│   │   ├── render/              # CGInterface and gfx rendering singletons
│   │   ├── runtime/             # RTTI helpers, class identification, memory validation
│   │   ├── types/               # POD structures, offsets, packed times, math structs
│   │   └── ui/                  # Native UI hierarchy (CGWnd, CIFWnd, CIFStatic, CIFGauge, etc.)
│   └── utils/                   # Memory offsets, STL helpers, logging, hooking macros
└── tests/                       # Unit and integration test suite (client_checks)
```

---

## 3. Native Engine Hierarchy & Object Model

The Silkroad client engine is written in C++ (MSVC 2003/2008 ABI) and makes extensive use of multiple inheritance and custom STL structures.

### UI Widget Hierarchy
All widgets derive from the engine's root UI classes:

```
CGWnd (Root base widget: bounds, visibility, children, message routing)
  └── CIFWnd (Interface widget base: UI resource map @ +0x1C4)
        ├── [CTextBoard] (Secondary base via Multiple Inheritance @ +0x84: textures, text rendering)
        └── CIFStatic (Label / static text control: text formatting, fonts, colors)
              ├── CIFGauge (Progress / HP / MP bar: fill ratio @ +0x398, target @ +0x39C, speed @ +0x3A0)
              ├── CIFButton (Clickable buttons)
              └── CIFDecoratedStatic (Icon-bearing labels, promo icons)
```

### Process / Game State Hierarchy
The client transitions between major game states via `CProcess`:

```
CGWnd
  └── CProcess (Game state manager: message queue, threads, network state)
        ├── CPSLogo (Intro logo playback)
        ├── CPSTitle (Login screen, server selection, credentials)
        ├── CPSVersionCheck (Patching, loading banner, intro camera)
        ├── CPSCharacterSelect (Character selection slots, 3D character preview entities)
        ├── CPSCharacterCreate (Character creation screens: China / Europe)
        └── CPSSilkroad (Main in-game world state: entities, world, rendering)
```

### Important Game Singletons
| Singleton | Pointer Address | Class Name | Responsibility |
| --- | --- | --- | --- |
| `CGInterface` | `0x013BAE3C` | `CGInterface` | Root in-game UI manager; controls all HUD children and promo guides |
| `CControler` | `0x013BAE38` | `CControler` | Active process/state controller (`get_active_child()`) |
| `CEntityManager` | `0x013BAE28` | `CEntityManager` | Global world entity registry |
| `CEntityManagerClient` | `0x013BAE24` | `CEntityManagerClient` | Client local player & spawned character cache |
| `CTextStringManager` | `0x0117EDA8` | `CTextStringManager` | Client string table lookup (`UIIT_*`, `UIOT_*`) |

---

## 4. Key Subsystems & Native Implementation Guidelines

### A. Promo & Guide Icons (HUD Customizer)
- **Engine Design**:
  - `CGInterface` manages UI children via child IDs.
  - Child ID `0x01` is `main_hud`.
  - Child ID `0x9D` is the right-hand **alarm strip** (`CAlramGuideMgrWnd`).
  - Child ID `0x9E` is the **guide host** (`CIFGuideHost`), which holds promo shortcut icons.
- **Rule - NEVER poke internal slot memory**:
  - Do NOT modify `alram_data[0..7]` slots or poke raw bytes at `0x584/0x58C/0x5AC`. Poking inactive slots without an item ID causes a null-pointer dereference in `sub_739A80`.
  - Do NOT hook `sub_7390B0` (`create_guide_icon`) to block creation by returning 0, as this corrupts widget unhiding.
- **Correct Native API**:
  Call the native `CGInterface` guide methods directly:
  - `show_magic_lamp_guide(bool)` (`0x008844C0`)
  - `show_daily_login_guide(bool)` (`0x008844F0`)
  - `show_facebook_guide(bool)` (`0x00884720`)
  - `show_web_item_alarm_guide(bool)` (`0x00883500`)
  - `show_macro_guide(bool)` (`0x00884170`)
  These native functions safely invoke widget allocation (`sub_7390B0`), positioning (`sub_738DF0`), or removal (`sub_7393D0`).

### B. Character Select & Animations
- **Logout Time Elapsed**:
  - The client natively parses the character's last logout time into `pcinfo_ui` at offset `+252` (`0xFC`), accessible via `pcinfo_ui::get_packed_time()`.
  - Do NOT intercept or manually deserialize network packet `0xB007`. Always read `character->get_packed_time()`.
- **Greeting / Emotion Animation**:
  - Character preview models are `cic_deco_character` entities.
  - Animations are triggered via `cic_deco_character::play_animation` (`0x00A90750`).
  - Animation `0x32` corresponds to `ANI_EMOTION01` (greeting wave).
  - Always introduce a brief delay (~20 frames via `EVENT_ON_CHAR_SELECT_UPDATE`) after slot selection before firing the animation to allow the 3D entity geometry and animation controller to settle.

### C. Target Window & HP Gauge
- **Structure**:
  - Root target window: `CIFTargetWindow` (child `0x10` of `CGInterface`).
  - Special mob content panel: child `0x388` (`CIFTargetWindowSpecialMob`).
  - Name label: `+0x378` (`cif_static*`).
  - HP gauge: `+0x37C` (`cif_gauge*`).
  - Level / Rank label: `+0x380` (`cif_static*`).
- **HP Fill Ratio**:
  - Read directly via `cif_gauge::get_current_percent()` (fill ratio float at `+0x398`).
  - Never use raw pointer offsets (`field_at<float>(gauge, 920)`) in plugin code.

---

## 5. Development Principles & Code Standards

1. **Native Engine Path Over Dirty Hacks**:
   - Always decompile the target game routine in IDA Pro via MCP before writing hooks or modifications.
   - If the game already provides a native function, singleton accessor, or structure field, use it directly.
2. **Encapsulation in `sdk/`**:
   - Plugins MUST NOT contain raw memory offsets (`field_at<T>`, `reinterpret_cast<void*>(ptr + 0x...)`).
   - All reverse-engineered game structures, member variables, and functions must be declared in `sdk/`.
3. **Hook Safety & Lifecycle**:
   - Every detour function MUST instantiate `ext_client::utils::hook_call_scope active_call;` as its first statement.
   - Avoid double-hooking high-level callers and low-level callees simultaneously.
4. **Project Inventory Consistency**:
   - Whenever adding a new `.cpp` or `.hpp` file, add it to both `ext_client.vcxproj` and `ext_client.vcxproj.filters`. The automated build system verifies that all project files match disk contents.
5. **Thread Safety**:
   - Engine callbacks and ImGui rendering run under the core event dispatch mutex.
   - For configuration, use `config::data()` under the mutex and `config::runtime()` for lockless cross-thread reading.
