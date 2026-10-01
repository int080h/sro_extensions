# Refactor Game Process Manager to Use Factory Entry Identification

## Summary
The game identifies processes via `get_factory_entry()` (vtable slot 0) and `get_process_id()` (vtable slot 1) virtual methods, NOT by comparing vtable addresses. The current extension code compares vtable addresses directly. This refactor aligns the extension with the game's native identification mechanism.

## Key Findings from IDA Analysis
- `CProcess_SetChildProcess` takes a **factory entry pointer** (global data struct), not a vtable or process ID
- Each process type has a global factory entry struct (e.g., `dword_117EBA0` for CPSTitle)
- `get_factory_entry()` (vtable slot 0, `__cdecl`) returns a pointer to the factory entry struct
- `get_process_id()` (vtable slot 1, `__cdecl`) returns an integer ID
- The game never compares vtable addresses for process identification

## Factory Entry Addresses (from IDA)
| Process | Factory Entry | Process ID Global |
|---------|--------------|-------------------|
| CPSilkroad | 0x0117E794 | 0x0117E7A0 |
| CPSMission | 0x0117E7BC | 0x0117E7C8 |
| CPSQuit | 0x0117E7DC | (none) |
| CPSCharacterSelect | 0x0117E85C | 0x0117E868 |
| CPSTitle | 0x0117EBA0 | 0x0117EBAC |
| CPSVersionCheck | 0x0117EBC8 | 0x0117EBD4 |

## Changes

### 1. `offsets.hpp` — Add missing cps_silkroad globals
- Add `cps_silkroad::globals::factory_entry = 0x0117E794`
- Add `cps_silkroad::globals::process_id = 0x0117E7A0`

### 2. `process.hpp` — New API
- Add `active_child_factory_entry() -> void*` — calls vtable[0] (`get_factory_entry`) on active child
- Add `active_child_process_id() -> int` — calls vtable[1] (`get_process_id`) on active child
- Change `active_child_as<T>` to take `expected_factory_entry` (uint32) instead of `expected_vftable`
- Keep `active_child_vftable()` for logging/debugging only

### 3. `process.cpp` — Implement new API
- Implement `active_child_factory_entry()`: read vtable ptr from child, call slot 0 as `void* (__cdecl*)()`
- Implement `active_child_process_id()`: read vtable ptr from child, call slot 1 as `int (__cdecl*)()`

### 4. `cprocess_manager.hpp/cpp` — Use factory entry
- Replace `active_child_vftable()` with `active_child_factory_entry()`
- Update `is_ingame()`: compare factory entry against known pre-game factory entries
- Update `is_title()`, `is_character_select()`, `is_version_check()`: compare factory entry

### 5. `cps_title.cpp` — Use factory entry
- `is_live()`: call `get_factory_entry` through vtable, compare to `cps_title::globals::factory_entry`
- `resolve_live()`: use `active_child_as<cps_title>(cps_title::globals::factory_entry)`

### 6. `cps_character_select.cpp` — Use factory entry
- `is_live()`: call `get_factory_entry` through vtable, compare to `cps_character_select::globals::factory_entry`
- `resolve_live()`: use `active_child_as<cps_character_select>(cps_character_select::globals::factory_entry)`

### 7. `cps_mission.cpp` — Use factory entry
- `is_live()`: call `get_factory_entry` through vtable, compare to `cps_mission::globals::factory_entry`
- `resolve_live()`: use `active_child_as<cps_mission>(cps_mission::globals::factory_entry)`

### 8. `cps_version_check_hook.cpp` — Use factory entry
- `is_version_check_active_process()`: use `active_child_factory_entry()` instead of `active_child_vftable()`
- Logging: log factory entry instead of (or alongside) vtable address

## Implementation Order
1. offsets.hpp (add cps_silkroad globals)
2. process.hpp + process.cpp (new API)
3. cprocess_manager.hpp + cprocess_manager.cpp
4. cps_title.cpp, cps_character_select.cpp, cps_mission.cpp
5. cps_version_check_hook.cpp
