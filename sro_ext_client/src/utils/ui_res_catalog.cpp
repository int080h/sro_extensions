#include "utils/ui_res_catalog.hpp"
#include "utils/string.hpp"

#include "utils/process.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <Windows.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace ext_client::utils::ui_res_catalog {

  namespace string_utils = ::ext_client::utils::string;

  namespace {

    struct catalog_state {
      bool loaded = false;
      bool file_loaded = false;
      bool memory_loaded = false;
      const void* memory_document = nullptr;
      std::unordered_map<int, entry> by_id{};
      std::unordered_map<std::string, entry> by_name{};
      std::recursive_mutex mutex;
    };

    catalog_state g_pstitle{};
    catalog_state g_character_select{};

    auto safe_strncpy(char* dst, const char* src, std::size_t dst_count) -> void;

    // Loose pstitle.txt is often missing (packed in Media.pk2). Keep common login widgets here.
    constexpr entry k_pstitle_fallbacks[] = {
      {"GDR_STA_BIGLOGO", "CIFStatic", 7, "interface\\outer\\logo-big.ddj", 360, 448, 896, 300},
      {"GDR_STA_LOGO",    "CIFStatic", 8, "interface\\outer\\logo.ddj",     540, 403, 508, 172},
    };

    auto parse_rect(std::string_view value, entry& out) -> void {
      int x = 0;
      int y = 0;
      int w = 0;
      int h = 0;
      if (std::sscanf(value.data(), "%d,%d,%d,%d", &x, &y, &w, &h) == 4) {
        out.rect_x = x;
        out.rect_y = y;
        out.rect_w = w;
        out.rect_h = h;
      }
    }

    auto parse_header_line(std::string_view line, entry& out) -> bool {
      const auto colon = line.find(':');
      if (colon == std::string_view::npos) {
        return false;
      }
      const auto name = string_utils::trim(line.substr(0, colon));
      const auto type = string_utils::trim(line.substr(colon + 1));
      if (name.empty() || name.size() >= sizeof(out.name) || type.size() >= sizeof(out.type)) {
        return false;
      }
      std::memcpy(out.name, name.data(), name.size());
      out.name[name.size()] = '\0';
      std::memcpy(out.type, type.data(), type.size());
      out.type[type.size()] = '\0';
      return true;
    }

    auto parse_property_line(std::string_view line, entry& out) -> void {
      const auto eq = line.find('=');
      if (eq == std::string_view::npos) {
        return;
      }
      const auto key = string_utils::trim(line.substr(0, eq));
      const auto rest = line.substr(eq + 1);

      const auto comma = rest.find(',');
      if (comma == std::string_view::npos) {
        return;
      }

      const auto quoted_part = rest.substr(comma + 1);
      const auto open_quote = quoted_part.find('"');
      const auto close_quote = quoted_part.find('"', open_quote + 1);
      if (open_quote == std::string_view::npos || close_quote == std::string_view::npos) {
        return;
      }

      const auto value = quoted_part.substr(open_quote + 1, close_quote - open_quote - 1);
      if (key == "ID") {
        out.id = std::atoi(std::string(value).c_str());
      } else if (key == "DDJ") {
        if (value.size() < sizeof(out.ddj)) {
          std::memcpy(out.ddj, value.data(), value.size());
          out.ddj[value.size()] = '\0';
        }
      } else if (key == "Rect") {
        parse_rect(value, out);
      }
    }

    auto try_load_path(const char* path, catalog_state& out) -> bool {
      std::ifstream file(path);
      if (!file.is_open()) {
        return false;
      }

      entry current{};
      bool in_block = false;
      std::string line;
      while (std::getline(file, line)) {
        std::string_view trimmed = string_utils::trim(line);
        if (trimmed.empty()) {
          continue;
        }
        if (trimmed[0] == '}') {
          if (in_block && current.id >= 0 && current.name[0] != '\0') {
            out.by_id[current.id] = current;
            out.by_name[current.name] = current;
          }
          current = entry{};
          in_block = false;
          continue;
        }
        if (trimmed[0] == '{') {
          in_block = true;
          continue;
        }
        if (!in_block) {
          current = entry{};
          if (parse_header_line(trimmed, current)) {
            in_block = true;
          }
          continue;
        }
        parse_property_line(trimmed, current);
      }

      return !out.by_id.empty();
    }

    auto merge_fallbacks(catalog_state& out, const entry* items, std::size_t count) -> void {
      for (std::size_t i = 0; i < count; ++i) {
        const auto& item = items[i];
        if (item.id < 0) {
          continue;
        }
        if (out.by_id.find(item.id) == out.by_id.end()) {
          out.by_id[item.id] = item;
        }
        if (item.name[0] != '\0' && out.by_name.find(item.name) == out.by_name.end()) {
          out.by_name[item.name] = item;
        }
      }
    }

    auto merge_fallback_names(catalog_state& out, const entry* items, std::size_t count) -> void {
      for (std::size_t i = 0; i < count; ++i) {
        const auto& item = items[i];
        if (item.name[0] == '\0') {
          continue;
        }
        if (item.id >= 0) {
          const auto it = out.by_id.find(item.id);
          if (it != out.by_id.end()) {
            if (it->second.name[0] == '\0') {
              safe_strncpy(it->second.name, item.name, sizeof(it->second.name));
            }
            if (it->second.ddj[0] == '\0' && item.ddj[0] != '\0') {
              safe_strncpy(it->second.ddj, item.ddj, sizeof(it->second.ddj));
            }
          }
        }
        if (out.by_name.find(item.name) == out.by_name.end()) {
          out.by_name[item.name] = item;
        }
      }
    }

    auto load_catalog(catalog_state& out, const char* relative_path, const entry* fallbacks, std::size_t fallback_count) -> void {
      if (out.loaded) {
        return;
      }
      out.loaded = true;

      char base[MAX_PATH]{};
      if (ext_client::utils::process::exe_directory(base, sizeof(base))) {
        char path[MAX_PATH]{};
        snprintf(path, sizeof(path), "%s%s", base, relative_path);
        if (try_load_path(path, out)) {
          out.file_loaded = true;
        } else {
          snprintf(path, sizeof(path), "%sMedia\\%s", base, relative_path);
          if (try_load_path(path, out)) {
            out.file_loaded = true;
          }
        }
      }

      if (fallbacks && fallback_count > 0) {
        merge_fallbacks(out, fallbacks, fallback_count);
      }
    }

    auto catalog_for(screen which) -> catalog_state& {
      switch (which) {
        case screen::character_select:
          return g_character_select;
        default:
          return g_pstitle;
      }
    }

    auto wide_to_narrow(const wchar_t* text, char* dst, std::size_t dst_count) -> void {
      if (!dst || dst_count == 0) {
        return;
      }
      dst[0] = '\0';
      if (!text || text[0] == L'\0') {
        return;
      }
      const int written = WideCharToMultiByte(CP_UTF8, 0, text, -1, dst, static_cast<int>(dst_count), nullptr, nullptr);
      if (written <= 0) {
        dst[0] = '\0';
      }
    }

    auto safe_strncpy(char* dst, const char* src, std::size_t dst_count) -> void {
      if (!dst || dst_count == 0) {
        return;
      }
      dst[0] = '\0';
      if (!src) {
        return;
      }
      std::strncpy(dst, src, dst_count - 1);
      dst[dst_count - 1] = '\0';
    }

    auto read_i32(const void* base, std::size_t offset, int* out) -> bool {
      if (!out || !base) {
        return false;
      }
      const auto value = *reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(base) + offset);
      *out = static_cast<int>(value);
      return true;
    }

    auto read_ptr_at(const void* base, std::size_t offset) -> const void* {
      if (!base) {
        return nullptr;
      }
      return *reinterpret_cast<void* const*>(static_cast<const std::uint8_t*>(base) + offset);
    }

    auto read_descriptor_ddj(const std::uint8_t* desc, char* dst, std::size_t dst_count) -> void {
      if (!desc || !dst || dst_count == 0) {
        return;
      }
      dst[0] = '\0';

      const auto* vector_obj = read_ptr_at(desc, ext_client::offsets::ui_res_document::descriptor::ddj_vector);
      if (!vector_obj) {
        return;
      }

      const auto* begin = read_ptr_at(vector_obj, ext_client::offsets::ui_res_document::string_vector::begin);
      if (!begin) {
        return;
      }

      const auto* path_ptr = read_ptr_at(begin, 0);
      if (!path_ptr) {
        return;
      }

      if (ext_client::msvc9::string_ref::from(path_ptr).copy_to(dst, dst_count) && dst[0] != '\0') {
        return;
      }

      if (path_ptr) {
        std::strncpy(dst, static_cast<const char*>(path_ptr), dst_count - 1);
        dst[dst_count - 1] = '\0';
      }
    }

    auto read_descriptor_type(const std::uint8_t* desc, char* dst, std::size_t dst_count) -> void {
      if (!desc || !dst || dst_count == 0) {
        return;
      }
      dst[0] = '\0';

      wchar_t wide[64]{};
      if (!ext_client::msvc9::wstring_ref::from(desc + ext_client::offsets::ui_res_document::descriptor::type_wstr)
             .copy_to(wide, std::size(wide))) {
        return;
      }
      wide_to_narrow(wide, dst, dst_count);
    }

    auto upsert_entry(catalog_state& catalog, const entry& item) -> void {
      if (item.id < 0) {
        return;
      }
      const auto existing = catalog.by_id.find(item.id);
      if (existing == catalog.by_id.end()) {
        catalog.by_id[item.id] = item;
      } else {
        auto& slot = existing->second;
        if (slot.ddj[0] == '\0' && item.ddj[0] != '\0') {
          safe_strncpy(slot.ddj, item.ddj, sizeof(slot.ddj));
        }
        if (slot.type[0] == '\0' && item.type[0] != '\0') {
          safe_strncpy(slot.type, item.type, sizeof(slot.type));
        }
        if (slot.rect_w == 0 && item.rect_w != 0) {
          slot.rect_x = item.rect_x;
          slot.rect_y = item.rect_y;
          slot.rect_w = item.rect_w;
          slot.rect_h = item.rect_h;
        }
        if (std::strncmp(slot.name, "GDR_", 4) != 0 && std::strncmp(item.name, "GDR_", 4) == 0) {
          safe_strncpy(slot.name, item.name, sizeof(slot.name));
        }
      }

      if (item.name[0] != '\0') {
        const auto existing_name = catalog.by_name.find(item.name);
        if (existing_name == catalog.by_name.end()) {
          catalog.by_name[item.name] = item;
        } else {
          auto& slot = existing_name->second;
          if (slot.id < 0 && item.id >= 0) {
            slot.id = item.id;
          }
          if (slot.ddj[0] == '\0' && item.ddj[0] != '\0') {
            safe_strncpy(slot.ddj, item.ddj, sizeof(slot.ddj));
          }
          if (slot.type[0] == '\0' && item.type[0] != '\0') {
            safe_strncpy(slot.type, item.type, sizeof(slot.type));
          }
          if (slot.rect_w == 0 && item.rect_w != 0) {
            slot.rect_x = item.rect_x;
            slot.rect_y = item.rect_y;
            slot.rect_w = item.rect_w;
            slot.rect_h = item.rect_h;
          }
        }
      }
    }

    auto entry_from_descriptor(const char* gdr_name, const std::uint8_t* desc) -> entry {
      entry out{};
      if (gdr_name) {
        safe_strncpy(out.name, gdr_name, sizeof(out.name));
      }
      read_descriptor_type(desc, out.type, sizeof(out.type));
      read_descriptor_ddj(desc, out.ddj, sizeof(out.ddj));
      read_i32(desc, ext_client::offsets::ui_res_document::descriptor::res_id, &out.id);
      read_i32(desc, ext_client::offsets::ui_res_document::descriptor::rect_x, &out.rect_x);
      read_i32(desc, ext_client::offsets::ui_res_document::descriptor::rect_y, &out.rect_y);
      read_i32(desc, ext_client::offsets::ui_res_document::descriptor::rect_w, &out.rect_w);
      read_i32(desc, ext_client::offsets::ui_res_document::descriptor::rect_h, &out.rect_h);
      return out;
    }

    auto walk_descriptor_list(catalog_state& catalog, const char* section_name, const void* section, int depth) -> void {
      if (!section || depth > 8) {
        return;
      }

      const auto* sentinel = read_ptr_at(section, ext_client::offsets::ui_res_document::section::child_list);
      if (!sentinel) {
        return;
      }

      ext_client::msvc9::list<void*>::from(sentinel).for_each([&](void* value_slot) {
        const auto* desc = read_ptr_at(value_slot, 0);
        if (!desc) {
          return;
        }

        const auto* desc_bytes = static_cast<const std::uint8_t*>(desc);
        const char* name = "";
        if (section_name && std::strncmp(section_name, "GDR_", 4) == 0) {
          name = section_name;
        }
        upsert_entry(catalog, entry_from_descriptor(name, desc_bytes));
      });
    }

    auto harvest_from_document(catalog_state& catalog, const void* document) -> bool {
      if (!document) {
        return false;
      }

      ext_client::msvc9::stdext_hash_map_ref::from(document).for_each_section(
        [&](const char* section_name, const void* section) { walk_descriptor_list(catalog, section_name, section, 0); });

      return !catalog.by_id.empty();
    }

    auto harvest_from_document_safe(catalog_state& catalog, const void* document) -> bool {
#if defined(_MSC_VER)
      __try {
        return harvest_from_document(catalog, document);
      } __except (EXCEPTION_EXECUTE_HANDLER) {
        catalog.by_id.clear();
        catalog.by_name.clear();
        return false;
      }
#else
      return harvest_from_document(catalog, document);
#endif
    }

    auto try_harvest_memory(catalog_state& catalog, void* res_ui_root) -> bool {
      if (!res_ui_root) {
        return false;
      }

      const auto* document = read_ptr_at(res_ui_root, ext_client::offsets::ui_res_document::res_ui_manager::parsed_document);
      if (!document) {
        catalog.memory_loaded = false;
        catalog.memory_document = nullptr;
        return false;
      }

      if (catalog.memory_loaded && catalog.memory_document == document && !catalog.by_id.empty()) {
        return true;
      }

      catalog.by_id.clear();
      catalog.by_name.clear();
      catalog.memory_loaded = false;
      catalog.memory_document = nullptr;

      if (!harvest_from_document_safe(catalog, document)) {
        return false;
      }

      catalog.memory_loaded = true;
      catalog.memory_document = document;
      return true;
    }

    auto apply_ddj_name_hints(catalog_state& catalog, const entry* items, std::size_t count) -> void {
      for (std::size_t i = 0; i < count; ++i) {
        const auto& item = items[i];
        if (item.name[0] == '\0' || item.ddj[0] == '\0') {
          continue;
        }
        for (auto& [id, slot] : catalog.by_id) {
          (void)id;
          if (slot.name[0] != '\0' || slot.ddj[0] == '\0') {
            continue;
          }
          if (string_utils::paths_match(slot.ddj, item.ddj) ||
              string_utils::paths_match(string_utils::basename(slot.ddj), string_utils::basename(item.ddj))) {
            safe_strncpy(slot.name, item.name, sizeof(slot.name));
          }
        }
      }
    }

  } // namespace

  auto layout_relative_path(screen which) -> const char* {
    switch (which) {
      case screen::character_select:
        return "resinfo\\pscharacterselect.txt";
      default:
        return "resinfo\\pstitle.txt";
    }
  }

  auto ensure_loaded(screen which) -> void {
    auto& catalog = catalog_for(which);
    std::lock_guard<std::recursive_mutex> lock{catalog.mutex};
    switch (which) {
      case screen::character_select:
        load_catalog(g_character_select, layout_relative_path(which), nullptr, 0);
        break;
      default:
        load_catalog(g_pstitle, layout_relative_path(which), k_pstitle_fallbacks, std::size(k_pstitle_fallbacks));
        break;
    }
  }

  auto file_loaded(screen which) -> bool {
    auto& catalog = catalog_for(which);
    std::lock_guard<std::recursive_mutex> lock{catalog.mutex};
    ensure_loaded(which);
    return catalog.file_loaded;
  }

  auto memory_loaded(screen which) -> bool {
    auto& catalog = catalog_for(which);
    std::lock_guard<std::recursive_mutex> lock{catalog.mutex};
    return catalog.memory_loaded;
  }

  auto catalog_source_for(screen which) -> catalog_source {
    auto& catalog = catalog_for(which);
    std::lock_guard<std::recursive_mutex> lock{catalog.mutex};
    if (catalog.memory_loaded) {
      return catalog_source::memory;
    }
    if (catalog.file_loaded) {
      return catalog_source::file;
    }
    if (!catalog.by_id.empty()) {
      return catalog_source::fallback;
    }
    return catalog_source::none;
  }

  auto sync_from_game(void* res_ui_root, screen which) -> void {
    auto& catalog = catalog_for(which);
    std::lock_guard<std::recursive_mutex> lock{catalog.mutex};
    catalog.loaded = true;
    if (try_harvest_memory(catalog, res_ui_root)) {
      if (which == screen::pstitle) {
        merge_fallback_names(catalog, k_pstitle_fallbacks, std::size(k_pstitle_fallbacks));
        apply_ddj_name_hints(catalog, k_pstitle_fallbacks, std::size(k_pstitle_fallbacks));
      }
      return;
    }

    if (!catalog.file_loaded) {
      switch (which) {
        case screen::character_select:
          load_catalog(catalog, layout_relative_path(which), nullptr, 0);
          break;
        default:
          load_catalog(catalog, layout_relative_path(which), k_pstitle_fallbacks, std::size(k_pstitle_fallbacks));
          break;
      }
    }
  }

  auto find(screen which, int res_id) -> const entry* {
    if (res_id < 0) {
      return nullptr;
    }
    auto& catalog = catalog_for(which);
    std::lock_guard<std::recursive_mutex> lock{catalog.mutex};
    ensure_loaded(which);
    const auto it = catalog.by_id.find(res_id);
    if (it == catalog.by_id.end()) {
      return nullptr;
    }
    return &it->second;
  }

  auto find_by_ddj(screen which, const char* ddj_path) -> const entry* {
    if (!ddj_path || ddj_path[0] == '\0') {
      return nullptr;
    }
    auto& catalog = catalog_for(which);
    std::lock_guard<std::recursive_mutex> lock{catalog.mutex};
    ensure_loaded(which);
    std::string_view leaf = string_utils::basename(ddj_path);
    const entry* basename_match = nullptr;
    for (const auto& [id, item] : catalog.by_id) {
      (void)id;
      if (item.ddj[0] == '\0') {
        continue;
      }
      if (string_utils::paths_match(item.ddj, ddj_path)) {
        return &item;
      }
      if (string_utils::paths_match(string_utils::basename(item.ddj), leaf)) {
        basename_match = &item;
      }
    }
    return basename_match;
  }

  auto find_by_name(screen which, const char* gdr_name) -> const entry* {
    if (!gdr_name || gdr_name[0] == '\0') {
      return nullptr;
    }
    auto& catalog = catalog_for(which);
    std::lock_guard<std::recursive_mutex> lock{catalog.mutex};
    ensure_loaded(which);
    const auto named = catalog.by_name.find(gdr_name);
    if (named != catalog.by_name.end()) {
      return &named->second;
    }
    for (const auto& [id, item] : catalog.by_id) {
      (void)id;
      if (std::strcmp(item.name, gdr_name) == 0) {
        return &item;
      }
    }
    return nullptr;
  }

} // namespace ext_client::utils::ui_res_catalog
