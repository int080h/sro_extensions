# Silkroad Online Client Extension SDK Guide

Welcome to the canonical SDK documentation for `sro_ext_client`. This guide outlines architectural standards, reverse-engineered engine contracts, memory layouts, and class design guidelines for all contributors and autonomous pair-programmers.

---

## 1. Directory Layout & Boundaries

The SDK is organized by engine functional subsystems:

```
sdk/
├── game/       # World simulation, entities, players, monsters, text string tables
├── net/        # Client network engine, sessions, packet builders, protocol definitions
├── process/    # Game state machine (CProcess, OuterInterface, Title, CharSelect, Silkroad)
├── render/     # DirectX 9 graphics pipeline, device applications, main UI interface
├── runtime/    # MSVC RTTI introspection, engine runtime class descriptors, safe casting
├── ui/         # In-game widgets (CGWnd, CIFWnd, CIFStatic, buttons, gauges, dialogs)
└── types/      # Pure Plain Old Data (POD) structs and enums ONLY (no .cpp files)
```

### Strict Rules for `sdk/types/`
- `sdk/types/` must contain **ONLY POD structs and enums** (e.g., `s_position`, `cif_text_color_state`).
- **NEVER** place class definitions, `.cpp` implementation files, or forwarding stubs in `sdk/types/`.
- If a helper struct or diagnostic output is used only by a single class, define it directly inside or alongside that class header (e.g. `ingame_res_lookup` in `cnif_sro_ingame_start.hpp`).

---

## 2. Canonical Class Architecture Skeleton

Every reverse-engineered game class in `sdk/` must follow the standardized structure:

```cpp
#pragma once

#include "sdk/.../base_class.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CClassName — Brief description of native engine role
// Native VTable: 0x01XXXXXX (N slots) | Singleton: 0x01XXXXXX | Class Size: 0xXXXX
// ---------------------------------------------------------------------------
class c_class_name : public c_base_class {
public:
  // --- 1. Compile-Time Metadata ---
  static constexpr std::uint32_t k_vtable_addr    = 0x01XXXXXX;
  static constexpr std::uint32_t k_singleton_addr = 0x01XXXXXX; // If singleton
  static constexpr std::size_t   k_class_size     = 0x0XXX;
  static constexpr std::size_t   vtable_slots     = NN;

  // --- 2. Singleton & Type Introspection ---
  static auto get() -> c_class_name*;
  static auto is_instance(const void* ptr) -> bool;

  // --- 3. Const-Correct Member Accessors (Trailing Return Types) ---
  auto get_field() -> int;
  auto get_field() const -> int;

  // --- 4. Subobject & Child Queries ---
  auto get_child_widget(int id) -> cgwnd*;

  // --- 5. High-Level Operations & Actions ---
  auto perform_action(int mode) -> void;
};
```

### Style & Coding Standards
1. **Trailing Return Types**: Use `auto method() -> return_type;` across all declarations and definitions.
2. **Const-Correctness**: Every accessor that does not mutate object state must be marked `const`. Provide both `const` and non-const overloads when returning pointers or references into internal buffers.
3. **No Phantom Declarations**: Never declare getters, setters, or virtual methods unless they are actually implemented in `.cpp` or backed by a confirmed engine offset. Unimplemented stub declarations cause symbol bloat and compilation failures.
4. **No Dirty Leftovers**: Stubs forwarding to nonexistent files, fake mockup classes, or copy-pasted unused methods are strictly forbidden.

---

## 3. Reverse-Engineering & Engine Memory Conventions

- **MSVC9 (VS2008) ABI Compatibility**: Silkroad Online (`isro_client.exe` / `sro_client.exe`) was compiled with MSVC 2008 (VC9). Native STL types (`std::string`, `std::vector`, `std::map`, `stdext::hash_map`) have distinct 32-bit layouts and alignment. Always use the helpers in `utils/msvc9_stl.hpp` (`ext_client::msvc9::*`).
- **Offset Verification**: Field offsets should be read using `ext_client::off::field_at<T>(this, offset)`.
- **Calling Conventions**:
  - Member methods in the client engine use `__thiscall` (register `ECX` holds `this`).
  - Free utilities and CRT functions use `__cdecl`.
  - Always use `ext_client::off::as_fn<signature>(address)` for invocation.
- **RTTI & Class Identification**:
  - `ext_client::gfx_runtime::get_class_name(obj)` reads the Silkroad custom `CRuntimeClass` table.
  - `ext_client::rtti::class_name(vftable, buf, len)` extracts native MSVC type descriptors.

---

## 4. Key Subsystem Singletons & Anchors

| Subsystem | Singleton / Anchor | Address | Primary Header |
|---|---|---|---|
| **World Simulation** | `sworld::get()` | `0x0113B8EC` | `sdk/game/sworld.hpp` |
| **Entity Manager** | `centity_manager_client::get()` | `0x0110F82C` | `sdk/game/centity_manager_client.hpp` |
| **Process Controller** | `ccontroler::get()` | `0x0117EBCC` | `sdk/game/ccontroler.hpp` |
| **Network Client** | `cclient_net::get()` | `0x0117EB48` | `sdk/net/cclient_net.hpp` |
| **Client Config** | `cclient_config::get()` | `0x004DE120` | `sdk/net/cclient_config.hpp` |
| **Direct3D Application**| `cd3d_application::get()` | `0x0117D234` | `sdk/render/cd3d_application.hpp` |
| **Graphics Engine** | `cgfx_video3d::get()` | `0x013BAE00` | `sdk/render/cgfx_video3d.hpp` |
| **Main UI Interface** | `cg_interface::get()` | `0x013BAE3C` | `sdk/render/cg_interface.hpp` |
| **Interface Resource Mgr**| `cirm_manager::get()` | `0x0117ED1C` | `sdk/ui/cirm_manager.hpp` |
| **2DT Interface Manager**| `cninterface_manager::get_instance()` | `0x01420408` | `sdk/ui/cninterface_manager.hpp` |
| **Global Data Manager** | `cglobal_data_manager` | `0x0117EE20` | `sdk/game/cglobal_data_manager.hpp` |
| **UI String Manager**   | `cui_string_manager`   | `0x0117EDA8` | `sdk/ui/cui_string_manager.hpp` |

---

## 5. Build, Verification & Deployment Workflow

The project uses an automated build and verification pipeline:

```powershell
# In sro_ext_client directory:
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Configuration Release -Check
```

### What `build.ps1` Validates:
1. **XML Project Inventory Check**: Recursively scans `20-Program\` on disk and verifies that every `.cpp` file matches `ext_client.vcxproj` 1:1. Any missing or extra file aborts the build.
2. **Native Toolchain Compilation**: Builds `ext_client.dll` in Win32 Release using the Visual Studio MSBuild toolchain.
3. **Automated Unit Checks (`-Check`)**: Builds and executes `client_checks.exe`, running regression tests across:
   - Configuration debouncing and persistence
   - Packet builders, ring buffers, and network log captures
   - Native detours and hook drains
   - Multithreaded event dispatch and shutdown barriers

### Deployment:
When building Release for in-game testing, copy the output artifacts to the game workspace:
```powershell
Copy-Item "..\Output\Release\ext_client.dll", "..\Output\Release\ext_client.pdb" -Destination "Z:\E\Workspace\Silkroad\RIGID_v234\" -Force
```
