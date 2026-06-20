#include "rtti.hpp"

#include <cstring>
#include <excpt.h>
#include <string>
#include <unordered_map>

namespace ext_client::rtti {

  auto class_name(std::uint32_t vftable, char* dst, std::size_t dst_count) -> bool {
    if (!dst || dst_count < 2 || vftable < 0x10000u || (vftable & 3u) != 0) {
      return false;
    }

    bool result = false;
    __try {
      const auto* vt = reinterpret_cast<const void* const*>(static_cast<std::uintptr_t>(vftable));
      const void* col = vt[-1];
      if (!col) {
        return false;
      }

      const auto* col_bytes = reinterpret_cast<const std::uint8_t*>(col);
      const void* type_desc = *reinterpret_cast<const void* const*>(col_bytes + 12);
      if (!type_desc) {
        return false;
      }

      const char* mangled = reinterpret_cast<const char*>(reinterpret_cast<const std::uint8_t*>(type_desc) + 8);

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

} // namespace ext_client::rtti
