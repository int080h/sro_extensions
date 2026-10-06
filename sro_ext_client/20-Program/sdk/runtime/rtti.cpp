#include "pch.hpp"
#include "sdk/runtime/rtti.hpp"
#include "utils/memory.hpp"

#include <cstring>
#include <excpt.h>
#include <string>
#include <unordered_map>

namespace ext_client::rtti {

  auto class_name(std::uint32_t vftable, char* dst, std::size_t dst_count) -> bool {
    if (!dst || dst_count < 2 || vftable < 0x10000u || (vftable & 3u) != 0) {
      return false;
    }

    // Safety checks: vftable - 4 (the complete object locator pointer pointer) must be readable
    const auto col_ptr_addr = static_cast<std::uintptr_t>(vftable) - sizeof(void*);
    if (!ext_client::utils::memory::is_readable_ptr(reinterpret_cast<const void*>(col_ptr_addr))) {
      return false;
    }

    bool result = false;
    __try {
      const auto* vt = reinterpret_cast<const void* const*>(static_cast<std::uintptr_t>(vftable));
      const void* col = vt[-1];
      if (!col || !ext_client::utils::memory::is_readable_ptr(col)) {
        return false;
      }

      const auto* col_bytes = reinterpret_cast<const std::uint8_t*>(col);
      // RTTI Complete Object Locator has TypeDescriptor* at offset 12
      if (!ext_client::utils::memory::is_readable_ptr(col_bytes + 12)) {
        return false;
      }
      const void* type_desc = *reinterpret_cast<const void* const*>(col_bytes + 12);
      if (!type_desc || !ext_client::utils::memory::is_readable_ptr(type_desc)) {
        return false;
      }

      // TypeDescriptor has mangled name at offset 8 (null terminated)
      const auto* mangled_bytes = reinterpret_cast<const std::uint8_t*>(type_desc) + 8;
      if (!ext_client::utils::memory::is_readable_ptr(mangled_bytes)) {
        return false;
      }
      const char* mangled = reinterpret_cast<const char*>(mangled_bytes);

      const char* class_start = nullptr;
      if (std::strncmp(mangled, ".?AV", 4) == 0) {
        class_start = mangled + 4;
      } else if (std::strncmp(mangled, ".?AU", 4) == 0) {
        class_start = mangled + 4;
      } else {
        return false;
      }

      const char* class_end = std::strstr(class_start, "@@");
      const std::size_t len = class_end != nullptr ? static_cast<std::size_t>(class_end - class_start) : std::strlen(class_start);
      if (len == 0 || len + 1 > dst_count) {
        return false;
      }

      std::memcpy(dst, class_start, len);
      dst[len] = '\0';
      result = true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      result = false;
    }

    return result;
  }

  auto class_name_cached(std::uint32_t vftable) -> const char* {
    static std::unordered_map<std::uint32_t, std::string> cache;

    if (auto found = cache.find(vftable); found != cache.end()) {
      return found->second.c_str();
    }

    char parsed[64]{};
    if (!class_name(vftable, parsed, sizeof(parsed))) {
      return "unknown";
    }

    auto [it, _] = cache.emplace(vftable, parsed);
    return it->second.c_str();
  }

namespace {
  auto raw_rtti_is_kind_of(const void* obj, const char* expected_class_name, std::uintptr_t& out_vftable) -> bool {
    out_vftable = 0;
    bool matched = false;
    __try {
      if (!ext_client::utils::memory::is_readable_ptr(obj)) {
        return false;
      }
      const auto* vtable_ptr = *reinterpret_cast<const void* const*>(obj);
      if (!vtable_ptr || !ext_client::utils::memory::is_readable_ptr(vtable_ptr)) {
        return false;
      }
      const auto vftable = reinterpret_cast<std::uintptr_t>(vtable_ptr);
      if (vftable < 0x10000u || (vftable & 3u) != 0) {
        return false;
      }
      out_vftable = vftable;

      const auto* vt = reinterpret_cast<const void* const*>(vftable);
      const void* col = vt[-1];
      if (col && ext_client::utils::memory::is_readable_ptr(col)) {
        const auto* col_bytes = reinterpret_cast<const std::uint8_t*>(col);
        if (ext_client::utils::memory::is_readable_ptr(col_bytes + 16)) {
          const auto* chd = *reinterpret_cast<const std::uint8_t* const*>(col_bytes + 16);
          if (chd && ext_client::utils::memory::is_readable_ptr(chd)) {
            const auto num_bases = *reinterpret_cast<const std::uint32_t*>(chd + 8);
            const auto* bca = *reinterpret_cast<const void* const* const*>(chd + 12);
            if (bca && ext_client::utils::memory::is_readable_ptr(bca) && num_bases > 0 && num_bases < 64) {
              const std::size_t expected_len = std::strlen(expected_class_name);
              for (std::uint32_t i = 0; i < num_bases; ++i) {
                const auto* bcd = reinterpret_cast<const std::uint8_t*>(bca[i]);
                if (!bcd || !ext_client::utils::memory::is_readable_ptr(bcd)) {
                  continue;
                }
                const auto* td = *reinterpret_cast<const std::uint8_t* const*>(bcd);
                if (!td || !ext_client::utils::memory::is_readable_ptr(td)) {
                  continue;
                }
                const char* mangled = reinterpret_cast<const char*>(td + 8);
                if (!ext_client::utils::memory::is_readable_ptr(mangled)) {
                  continue;
                }
                const char* start = (std::strncmp(mangled, ".?AV", 4) == 0 || std::strncmp(mangled, ".?AU", 4) == 0)
                                      ? mangled + 4
                                      : mangled;
                const char* end = std::strstr(start, "@@");
                const std::size_t len = end ? static_cast<std::size_t>(end - start) : std::strlen(start);
                if (len == expected_len && std::strncmp(start, expected_class_name, len) == 0) {
                  matched = true;
                  break;
                }
              }
            }
          }
        }
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      matched = false;
    }
    return matched;
  }
} // namespace

  auto is_kind_of(const void* obj, const char* expected_class_name) -> bool {
    if (!obj || !expected_class_name || !ext_client::utils::memory::is_game_ptr(obj)) {
      return false;
    }

    if (!ext_client::utils::memory::is_readable_ptr(obj)) {
      return false;
    }

    const auto* vtable_ptr = *reinterpret_cast<const void* const*>(obj);
    if (!vtable_ptr || !ext_client::utils::memory::is_readable_ptr(vtable_ptr)) {
      return false;
    }

    const auto vftable = reinterpret_cast<std::uintptr_t>(vtable_ptr);
    if (vftable < 0x10000u || (vftable & 3u) != 0) {
      return false;
    }

    static std::unordered_map<std::uint64_t, bool> s_kind_cache;
    const std::uint64_t name_hash = std::hash<std::string_view>{}(expected_class_name);
    const std::uint64_t cache_key = (static_cast<std::uint64_t>(vftable) << 32) ^ name_hash;

    if (auto it = s_kind_cache.find(cache_key); it != s_kind_cache.end()) {
      return it->second;
    }

    std::uintptr_t parsed_vftable = 0;
    bool matched = raw_rtti_is_kind_of(obj, expected_class_name, parsed_vftable);
    if (!matched) {
      matched = ext_client::gfx_runtime::is_kind_of(obj, expected_class_name);
    }

    s_kind_cache[cache_key] = matched;
    return matched;
  }
} // namespace ext_client::rtti
