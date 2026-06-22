#pragma once

#include "utils/memory.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ext_client::rtti {

  // Read the MSVC RTTI class name from a vtable pointer.
  // E.g. vtable for CIFStatic → "CIFStatic". Returns false if no RTTI is available.
  auto class_name(std::uint32_t vftable, char* dst, std::size_t dst_count) -> bool;

  // Cached version — returns a pointer to a static/internal string.
  // Falls back to "unknown" if RTTI is unavailable.
  auto class_name_cached(std::uint32_t vftable) -> const char*;

} // namespace ext_client::rtti

namespace ext_client::gfx_runtime {

  struct gfx_runtime_class {
    const char* m_lpszClassName;              // +0x00
    unsigned int m_wSchema;                   // +0x04
    int m_nObjectSize;                        // +0x08
    const gfx_runtime_class* m_pBaseClass;    // +0x0C
    int field_10;                             // +0x10
    int field_14;                             // +0x14
    void* m_pfnCreateObject;                  // +0x18
    void* m_pfnDeleteObject;                  // +0x1C
  };

  inline const gfx_runtime_class* get_runtime_class(const void* obj) noexcept {
    if (!obj || !ext_client::utils::memory::is_readable_ptr(obj)) {
      return nullptr;
    }
    const auto* vtable = *reinterpret_cast<const void* const*>(obj);
    if (!vtable || !ext_client::utils::memory::is_readable_ptr(vtable)) {
      return nullptr;
    }
    const auto* fn_ptr = reinterpret_cast<const void* const*>(vtable);
    if (!fn_ptr || !ext_client::utils::memory::is_code_ptr(*fn_ptr)) {
      return nullptr;
    }
    const auto fn = reinterpret_cast<const gfx_runtime_class* (__thiscall *)(const void*)>(*fn_ptr);
    return fn(obj);
  }

  inline bool is_class_name_match(const void* obj, const char* expected_name) noexcept {
    if (!obj || !expected_name) return false;
    const auto* rc = get_runtime_class(obj);
    if (!rc || !rc->m_lpszClassName) return false;
    return std::strcmp(rc->m_lpszClassName, expected_name) == 0;
  }

  inline const char* get_class_name(const void* obj) noexcept {
    if (!obj) return "none";
    const auto* rc = get_runtime_class(obj);
    if (!rc || !rc->m_lpszClassName) return "none";
    return rc->m_lpszClassName;
  }

} // namespace ext_client::gfx_runtime
