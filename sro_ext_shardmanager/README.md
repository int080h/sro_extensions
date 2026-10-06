# Silkroad ShardManager Extension (`ext_shardmanager`)

Modern C++20 extension DLL for the Silkroad Online ShardManager (`SR_ShardManager.exe`).
Coordinates inter-server routing, cross-shard transactions, user session tracking, and server cluster operations.

---

## 1. Architecture & Design Principles

1. **Deterministic State Synchronization**:
   - Zero raw pointer manipulation; typed wrappers for shard instances, machine connections, and cluster routing tables.
2. **Double-Buffered Configuration**:
   - Thread-safe configuration reloading without blocking active shard heartbeats or inter-server packet traffic.
3. **Structured Event System**:
   - Server lifecycle, shard registration, user verification, and inter-server messaging mapped to typed event handlers.
4. **Resilient Detours**:
   - Hot-patchable, relocatable x86 hooks with in-flight call tracking and graceful drain on shutdown.

---

## 2. Directory Structure

```
sro_ext_shardmanager/
├── 10-Library/                  # Third-party utilities (MinHook, detours, formatting)
├── 20-Program/
│   ├── core/                    # Extension core: plugin manager, config, event dispatch, detours
│   │   ├── hooks/               # Engine detours (shard_hooks, network_hooks, cluster_hooks)
│   │   ├── core_config.cpp/.hpp # Thread-safe configuration manager
│   │   ├── core_event_manager   # High-performance event pub/sub dispatcher
│   │   └── core_plugin_manager  # Dynamic plugin lifecycle registry
│   ├── plugins/                 # Shard-level plugins, security audits, metrics collectors
│   ├── sdk/                     # Reconstructed shard manager classes
│   │   ├── cluster/             # Machine connection nodes, shard info records
│   │   ├── net/                 # Gateway, gameserver, and billing communication protocols
│   │   └── types/               # POD types, network session IDs, packet headers
│   └── utils/                   # Shared utilities (hooks, memory, msvc9_stl, string)
├── tests/                       # Unit and regression test harnesses
├── build.ps1                    # Automated compilation and validation script
└── ext_shardmanager.vcxproj     # Visual Studio 2022 project file
```
