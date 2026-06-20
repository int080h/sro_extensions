#pragma once

#include <cstddef>
#include <cstdint>

namespace ext_client::rtti {

  // Read the MSVC RTTI class name from a vtable pointer.
  // E.g. vtable for CIFStatic → "CIFStatic". Returns false if no RTTI is available.
  auto class_name(std::uint32_t vftable, char* dst, std::size_t dst_count) -> bool;

  // Cached version — returns a pointer to a static/internal string.
  // Falls back to "unknown" if RTTI is unavailable.
  auto class_name_cached(std::uint32_t vftable) -> const char*;

} // namespace ext_client::rtti
