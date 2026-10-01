# Refactor sro_ext_client — Reduce Redundancy & Repetitiveness

## Analysis Summary

After reading all source files in `sro_ext_client/src/`, the following redundancy patterns were identified:

### 1. `core_config.cpp` — Massive if/else parse/save chains (HIGH)
- `apply_key()` is ~315 lines of repetitive `if (strcmp(key, "x") == 0) { field = parse_bool(value, field); }` blocks
- `save()` is ~177 lines of repetitive `file << "key=" << (field ? 1 : 0) << "\n";` lines
- Every field follows one of 6 patterns: bool, int, string (char[]), float, hex16, hex32
- **Fix**: Table-driven approach with a `field_desc` array containing `{section, key, ptr, type, size}`. A single `apply_field()` and `save_field()` handle all types via switch. Reduces ~500 lines to ~120.

### 2. `net_log_plugin.cpp` — Duplicated dispatch/record functions (HIGH)
- 4 nearly identical `dispatch_incoming/outgoing` overloads (for `cmsg_stream_buffer*` vs `cmsg*`)
- 4 nearly identical `record_*` functions (record_incoming_cmsg, record_incoming_stream, record_outgoing_cmsg, record_outgoing_stream)
- `should_capture` logic duplicated in `push_entry`
- **Fix**: Template the dispatch functions on the message type. Consolidate record functions into a single `record_packet` with a direction + type enum. Extract `should_capture` as a standalone helper.

### 3. `title_plugin.cpp` — 7 identical DragFloat2 blocks (MEDIUM)
- 7 consecutive `ImGui::DragFloat2` blocks for EU login frame adjustments, each with the same pattern: label, pointer to vector2f, speed, min, max, and `mark_dirty()` call
- **Fix**: Replace with a loop over a constexpr array of `{label, ptr}` pairs.

### 4. `render_system.cpp` — Duplicated input install lambdas (MEDIUM)
- Lines 94-98 and 136-139: identical `g_input.install(...)` lambda calls
- **Fix**: Extract into a `install_input_hooks()` helper method.

### 5. All plugin menu handlers — `Checkbox + mark_dirty` boilerplate (MEDIUM)
- Pattern `if (ImGui::Checkbox("label", &field)) { core::config::mark_dirty(); }` repeated dozens of times across title_plugin, version_check_plugin, net_log_plugin, hud_customizer_plugin, target_window_plugin, welcome_msg_plugin
- **Fix**: Add a tiny helper `inline auto dirty_checkbox(const char* label, bool* v) -> bool` in a new `utils/imgui_helpers.hpp` that does the check + mark_dirty in one call.

### 6. `character_select_plugin.cpp` — Binary read helpers (LOW)
- `read_u8_at`, `read_u16_at`, `read_u32_at`, `skip_bytes`, `skip_string_at` are generic packet parsing utilities trapped inside a single plugin
- **Fix**: Move to a shared `utils/packet_reader.hpp` for reuse across plugins.

---

## Implementation Plan

### Step 1: Refactor `core_config.cpp` (HIGH)
- Add `field_type` enum, `field_desc` struct, and `g_fields[]` table
- Add `apply_field()` and `save_field()` helper functions
- Replace `apply_key()` body with table lookup loop + special cases for `interface_manager` dynamic hides and `plugins` section
- Replace `save()` body with table-driven iteration, appending dynamic `interface_manager` hide entries and `plugins` section after
- Reorder table so `interface_manager` is last (before special sections) to allow dynamic entries to be appended naturally
- **Expected reduction**: ~500 lines → ~120 lines

### Step 2: Refactor `net_log_plugin.cpp` (HIGH)
- Template `dispatch_incoming`/`dispatch_outgoing` on the message type parameter
- Consolidate `record_*` functions into a single `record_packet(direction, type, opcode, data, size)` 
- Extract `should_capture()` as a standalone function used by both dispatch and push_entry
- **Expected reduction**: ~80 lines saved

### Step 3: Refactor `title_plugin.cpp` DragFloat2 blocks (MEDIUM)
- Define a constexpr array of `{label, vector2f*}` pairs for the 7 EU login adjustments
- Replace 7 blocks with a single loop
- **Expected reduction**: ~40 lines saved

### Step 4: Refactor `render_system.cpp` input hooks (MEDIUM)
- Extract the duplicated `g_input.install(...)` calls into a helper method
- **Expected reduction**: ~10 lines saved

### Step 5: Add `utils/imgui_helpers.hpp` (MEDIUM)
- Create `dirty_checkbox()`, `dirty_slider_int()`, `dirty_input_text()` helpers
- Update plugin menu handlers to use these helpers
- **Expected reduction**: ~50 lines saved across plugins

### Step 6: Extract `utils/packet_reader.hpp` (LOW)
- Move binary read helpers from `character_select_plugin.cpp` into a shared header
- **Expected reduction**: ~30 lines, improved reusability

---

## Risk Assessment
- All changes are internal refactors with no behavior changes
- Config INI file format remains identical (same keys, same values, same section order modulo interface_manager/widgets swap)
- No API changes to plugin interfaces or event system
- Build system (CMake) does not need changes for steps 1-4
- Step 5 adds a new header file (may need CMake update if not globbing)
- Step 6 adds a new header file

## Verification
- Build the project using existing CMake build system
- Verify INI file loads/saves correctly (diff old vs new output)
- Verify plugins still register and function
