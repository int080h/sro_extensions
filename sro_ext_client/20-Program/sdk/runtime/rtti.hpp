#pragma once

#include "sdk/runtime/gfx_runtime.hpp"

#include <cstddef>
#include <cstdint>

namespace ext_client::rtti {

  // Read the MSVC RTTI class name from a vtable pointer.
  // E.g. vtable for CIFStatic → "CIFStatic". Returns false if no RTTI is available.
  auto class_name(std::uint32_t vftable, char* dst, std::size_t dst_count) -> bool;

  // Cached version — returns a pointer to a static/internal string.
  // Falls back to "unknown" if RTTI is unavailable.
  auto class_name_cached(std::uint32_t vftable) -> const char*;

  // Check if an object derives from or is of class `expected_class_name`
  // using MSVC RTTI ClassHierarchyDescriptor. Falls back to gfx_runtime.
  auto is_kind_of(const void* obj, const char* expected_class_name) -> bool;
} // namespace ext_client::rtti
