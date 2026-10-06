# Silkroad GameServer Extension (`ext_gameserver`)

Modern C++20 extension DLL for the Silkroad Online GameServer (`SR_GameServer.exe`).
Designed with strict adherence to the native game engine architecture — type-safe object model, zero raw pointer guessing, and zero fragile memory hacks.

---

## 1. Architecture & Design Principles

The GameServer extension shares the architectural foundation established in `sro_ext_client`:

1. **Object Model Alignment**:
   - Direct representation of server-side game entities (world, regions, players, monsters, NPCs, items, inventory).
   - VTable address verification and exact binary size validation matching MSVC9 layout.
2. **Thread Safety & Concurrency**:
   - Double-buffered, immutable configuration snapshots (`config::runtime()`).
   - Thread-safe event dispatching with barrier-protected shutdown.
3. **Strict Plugin Decoupling**:
   - Feature implementations reside entirely in `plugins/` and communicate strictly through the core event manager and typed SDK APIs.
4. **Compile-Time Inventory Guards**:
   - `build.ps1` validates that all source files on disk match Visual Studio project files.

---

## 2. Directory Structure

```
sro_ext_gameserver/
├── 10-Library/                  # Third-party utilities (MinHook, detours, formatting)
├── 20-Program/
│   ├── core/                    # Extension core: plugin manager, config, event dispatch, detours
│   │   ├── hooks/               # Engine detours (server_hooks, network_hooks, tick_hooks)
│   │   ├── core_config.cpp/.hpp # Thread-safe configuration manager
│   │   ├── core_event_manager   # High-performance event pub/sub dispatcher
│   │   └── core_plugin_manager  # Dynamic plugin lifecycle registry
│   ├── plugins/                 # High-level server gameplay features and extensions
│   ├── sdk/                     # Reconstructed engine classes & structures
│   │   ├── entity/              # Server character, mob, item, object representations
│   │   ├── net/                 # Server network engine, packet builders, session managers
│   │   ├── world/               # Regions, world partitions, object cells
│   │   └── types/               # POD types, vectors, coordinates, packed time
│   └── utils/                   # Shared utilities (hooks, memory, msvc9_stl, string)
├── tests/                       # Unit and regression test harnesses
├── build.ps1                    # Automated compilation and validation script
└── ext_gameserver.vcxproj       # Visual Studio 2022 project file
```

---

## 3. Shared Utilities Migration Roadmap

Components designed for cross-project sharing across `ext_client`, `ext_gameserver`, and `ext_shardmanager`:
- `utils/msvc9_stl` (MSVC9 standard library container binary wrappers)
- `utils/hooks` and `utils/x86_lde` (x86 length disassembler and inline detour engine)
- `utils/memory` (page protection helpers, pattern scanners)
- `sdk/types/packed_time` (Silkroad bit-packed timestamp encoder and decoder)
- `sdk/net/msg_define` (Opcode registries and packet identifiers)
