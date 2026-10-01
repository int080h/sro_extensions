#pragma once

#include <cstddef>
#include <cstdint>

namespace ext_client::off {

  inline auto vtable_slot(std::uintptr_t vtable, std::size_t slot) -> std::uintptr_t {
    return *reinterpret_cast<std::uintptr_t*>(vtable + slot * sizeof(void*));
  }

  template<typename T> inline auto as_fn(std::uint32_t address) -> T {
    return reinterpret_cast<T>(address);
  }

  template<typename T> inline auto global_at(std::uint32_t address) -> T& {
    return *reinterpret_cast<T*>(address);
  }

  template<typename T> inline auto field_at(const void* self, std::size_t offset) -> const T& {
    return *reinterpret_cast<const T*>(reinterpret_cast<const std::uint8_t*>(self) + offset);
  }

  template<typename T> inline auto field_at(void* self, std::size_t offset) -> T& {
    return *reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(self) + offset);
  }

  inline auto raw_vftable(const void* self) -> const std::uintptr_t* {
    return *reinterpret_cast<const std::uintptr_t* const*>(self);
  }

  inline auto raw_vftable(void* self) -> std::uintptr_t* {
    return *reinterpret_cast<std::uintptr_t* const*>(self);
  }

} // namespace ext_client::off
