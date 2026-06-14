#pragma once

#include <cstddef>
#include <cstdint>

namespace ext_client::msvc9 {
  auto is_readable_ptr(const void* ptr, std::size_t bytes) -> bool;
}

#define PAD_BYTES(name, count) std::uint8_t name[count]

#define PAD_FROM_TO(name, from, to) PAD_BYTES(name, (to) - (from))

#define PAD_TAIL(name, struct_size) PAD_FROM_TO(name, sizeof(void*), struct_size)

#define VFN_THISCALL(name, ret, ...) auto(__thiscall * name)(__VA_ARGS__)->ret

#define VFN_STDCALL(name, ret, ...) auto(__stdcall * name)(__VA_ARGS__)->ret

#define VFN_CDECL(name, ret, ...) auto(__cdecl * name)(__VA_ARGS__)->ret

// ---------------------------------------------------------------------------
// Anonymous unique padding
// ---------------------------------------------------------------------------
#define CONCAT_INNER(a, b) a##b
#define CONCAT(a, b) CONCAT_INNER(a, b)

#define PAD(count) std::uint8_t CONCAT(pad_, __COUNTER__)[count]

#define PAD_TO(from, to) PAD((to) - (from))

// ---------------------------------------------------------------------------
// SDK casting and helper generation macros
// ---------------------------------------------------------------------------
#define DECLARE_SDK_WND_CAST(ClassName)                         \
  template<typename T> static auto from(T* wnd) -> ClassName* { \
    return reinterpret_cast<ClassName*>(wnd);                   \
  }                                                             \
  static auto from(void* wnd) -> ClassName* {                   \
    return reinterpret_cast<ClassName*>(wnd);                   \
  }

#define DECLARE_SDK_WND_HELPERS(ClassName, VTableAddress)                  \
  DECLARE_SDK_WND_CAST(ClassName)                                          \
  static auto is_instance(const void* wnd) -> bool {                       \
    if (!ext_client::msvc9::is_readable_ptr(wnd, sizeof(std::uint32_t))) { \
      return false;                                                        \
    }                                                                      \
    const auto vtable = *reinterpret_cast<const std::uint32_t*>(wnd);      \
    return vtable == (VTableAddress);                                      \
  }

#define DECLARE_SDK_VTABLE(VTableType, GetterName)       \
  auto GetterName() -> VTableType* {                     \
    return reinterpret_cast<VTableType*>(vftable);       \
  }                                                      \
  auto GetterName() const -> const VTableType* {         \
    return reinterpret_cast<const VTableType*>(vftable); \
  }

#define DECLARE_SDK_CAST(TargetType, MethodName)      \
  auto MethodName() -> TargetType* {                  \
    return reinterpret_cast<TargetType*>(this);       \
  }                                                   \
  auto MethodName() const -> const TargetType* {      \
    return reinterpret_cast<const TargetType*>(this); \
  }

#define DECLARE_SDK_OFFSET_CAST(TargetType, MethodName, Offset)                                         \
  auto MethodName() -> TargetType* {                                                                    \
    return reinterpret_cast<TargetType*>(reinterpret_cast<std::uint8_t*>(this) + (Offset));             \
  }                                                                                                     \
  auto MethodName() const -> const TargetType* {                                                        \
    return reinterpret_cast<const TargetType*>(reinterpret_cast<const std::uint8_t*>(this) + (Offset)); \
  }

namespace ext_client::offsets {

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

} // namespace ext_client::offsets
