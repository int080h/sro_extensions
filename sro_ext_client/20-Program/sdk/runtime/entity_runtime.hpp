#pragma once

#include "sdk/runtime/rtti.hpp"
#include "sdk/runtime/gfx_runtime.hpp"

#include <cstring>

namespace ext_client::entity_runtime {

  inline const char* get_class_name(const void* obj) noexcept {
    if (!obj) {
      return "none";
    }
    const auto* gfx_name = ext_client::gfx_runtime::get_class_name(obj);
    if (gfx_name && std::strcmp(gfx_name, "none") != 0) {
      return gfx_name;
    }
    const auto vftable = *reinterpret_cast<const std::uint32_t*>(obj);
    return ext_client::rtti::class_name_cached(vftable);
  }

  inline bool is_class_name_match(const void* obj, const char* expected_name) noexcept {
    if (!obj || !expected_name) {
      return false;
    }
    if (ext_client::gfx_runtime::is_class_name_match(obj, expected_name)) {
      return true;
    }
    const auto vftable = *reinterpret_cast<const std::uint32_t*>(obj);
    char buf[128]{};
    if (!ext_client::rtti::class_name(vftable, buf, sizeof(buf))) {
      return false;
    }
    return std::strcmp(buf, expected_name) == 0;
  }

  template<typename T>
  inline auto safe_cast(void* ptr) -> T* {
    if (!ptr || !T::is_instance(ptr)) {
      return nullptr;
    }
    return reinterpret_cast<T*>(ptr);
  }

  template<typename T>
  inline auto safe_cast(const void* ptr) -> const T* {
    return safe_cast<T>(const_cast<void*>(ptr));
  }

} // namespace ext_client::entity_runtime
