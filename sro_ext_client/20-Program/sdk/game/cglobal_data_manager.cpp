#include "pch.hpp"
#include "sdk/game/cglobal_data_manager.hpp"

#include "utils/memory.hpp"
#include "utils/msvc9_stl.hpp"

#include <cwchar>

namespace {

  inline auto manager() -> void* {
    auto* mgr = reinterpret_cast<void*>(cglobal_data_manager::k_singleton_addr);
    return ext_client::utils::memory::is_game_ptr(mgr) ? mgr : nullptr;
  }

  // Native game engine helper functions (using verified pointer validation)

  auto get_ref_item_impl(void* mgr, std::uint32_t ref_id) -> void* {
    if (!mgr || !ext_client::utils::memory::is_game_ptr(mgr)) return nullptr;
    constexpr std::uintptr_t k_get_ref_item_fn = 0x00A93E20;
    using fn_get_ref_item = void* (__thiscall*)(void* self, std::uint32_t id);
    return reinterpret_cast<fn_get_ref_item>(k_get_ref_item_fn)(mgr, ref_id);
  }

  auto get_ref_char_impl(void* mgr, std::uint32_t char_id) -> void* {
    if (!mgr || !ext_client::utils::memory::is_game_ptr(mgr)) return nullptr;
    constexpr std::uintptr_t k_get_ref_char_fn = 0x00A93CF0;
    using fn_get_ref_char = void* (__thiscall*)(void* self, std::uint32_t id);
    return reinterpret_cast<fn_get_ref_char>(k_get_ref_char_fn)(mgr, char_id);
  }

  auto find_option_info_impl(void* mgr, std::uint16_t opt_id) -> void* {
    if (!mgr || !ext_client::utils::memory::is_game_ptr(mgr)) return nullptr;
    constexpr std::uintptr_t k_find_option_fn = 0x00A757A0;
    constexpr std::uintptr_t k_get_info_fn    = 0x00AC8CF0;
    using fn_find = void* (__thiscall*)(void*, std::uint32_t);
    auto* node = reinterpret_cast<fn_find>(k_find_option_fn)(mgr, opt_id);
    if (!node || !ext_client::utils::memory::is_game_ptr(node)) return nullptr;

    using fn_info = void* (__thiscall*)(void*);
    return reinterpret_cast<fn_info>(k_get_info_fn)(node);
  }

  // MSVC9 (VS2008) std::wstring layout, only used as an out-parameter of sub_A80130
  struct msvc9_wstring_buf {
    std::uint32_t proxy{0};
    union {
      wchar_t  buf[8]{0};
      wchar_t* ptr;
    } bx{};
    std::uint32_t size{0};
    std::uint32_t res{7};
  };

  auto resolve_type_key_impl(void* mgr, std::uint32_t tid, const void* code_ptr,
                             wchar_t* out_key, std::size_t out_cap) -> bool {
    if (!mgr || !ext_client::utils::memory::is_game_ptr(mgr) || !out_key || out_cap == 0) return false;
    constexpr std::uintptr_t k_resolve_type_fn = 0x00A80130;
    constexpr std::uintptr_t k_tidy_fn         = 0x004086A0;

    msvc9_wstring_buf out_str{};
    using fn_resolve = int (__thiscall*)(void* self, void* out_wstr, std::uint32_t tid, const void* code_ptr);
    reinterpret_cast<fn_resolve>(k_resolve_type_fn)(mgr, &out_str, tid, code_ptr);

    bool ok = false;
    const wchar_t* k = (out_str.res >= 8) ? out_str.bx.ptr : out_str.bx.buf;
    if (k && *k != L'\0' && ext_client::utils::memory::is_readable_ptr(k)) {
      std::wcsncpy(out_key, k, out_cap - 1);
      out_key[out_cap - 1] = L'\0';
      ok = true;
    }

    using fn_tidy = void (__thiscall*)(void* str);
    reinterpret_cast<fn_tidy>(k_tidy_fn)(&out_str);
    return ok;
  }

  auto query_devil_entries_impl(void* mgr, std::uint32_t ref_id, std::uint32_t opt_level,
                                cglobal_data_manager::devil_option_entry* out, int cap) -> int {
    if (!mgr || !ext_client::utils::memory::is_game_ptr(mgr) || !out || cap <= 0) return 0;
    constexpr std::uintptr_t k_find_group_fn = 0x00A742A0;
    constexpr std::uintptr_t k_level_slot_fn = 0x009DAEE0;
    constexpr std::uintptr_t k_find_list_fn  = 0x00A74300;

    int count = 0;
    using fn_lookup = std::uintptr_t (__thiscall*)(void* self, std::uint32_t arg);
    const auto group = reinterpret_cast<fn_lookup>(k_find_group_fn)(mgr, ref_id);
    if (group != 0 && ext_client::utils::memory::is_game_ptr(reinterpret_cast<void*>(group))) {
      const auto slot = reinterpret_cast<fn_lookup>(k_level_slot_fn)(reinterpret_cast<void*>(group), opt_level);
      if (slot != 0 && ext_client::utils::memory::is_game_ptr(reinterpret_cast<void*>(slot))) {
        const auto list_key = *reinterpret_cast<const std::uint32_t*>(slot + 4);
        const auto list = reinterpret_cast<fn_lookup>(k_find_list_fn)(mgr, list_key);
        if (list != 0 && ext_client::utils::memory::is_game_ptr(reinterpret_cast<void*>(list))) {
          const auto begin = *reinterpret_cast<const std::uintptr_t*>(list + 0x14);
          const auto end   = *reinterpret_cast<const std::uintptr_t*>(list + 0x18);
          if (begin != 0 && end >= begin && ext_client::utils::memory::is_game_ptr(reinterpret_cast<void*>(begin))) {
            const auto total = static_cast<std::size_t>((end - begin) / 36);
            for (std::size_t i = 0; i < total && count < cap; ++i) {
              const auto entry = begin + i * 36;
              auto& e = out[count];
              e.opt_id = *reinterpret_cast<const std::uint16_t*>(entry + 0);
              e.value  = *reinterpret_cast<const std::uint16_t*>(entry + 2);
              e.group  = *reinterpret_cast<const std::uint8_t*>(entry + 4);

              // MSVC9 std::wstring at entry + 8: _Buf/_Ptr @ +4, _Mysize @ +0x14, _Myres @ +0x18
              const auto wstr = entry + 8;
              const auto size = *reinterpret_cast<const std::uint32_t*>(wstr + 0x14);
              const auto res  = *reinterpret_cast<const std::uint32_t*>(wstr + 0x18);
              const wchar_t* text = (res >= 8)
                ? *reinterpret_cast<const wchar_t* const*>(wstr + 4)
                : reinterpret_cast<const wchar_t*>(wstr + 4);
              if (text && ext_client::utils::memory::is_readable_ptr(text)) {
                const std::size_t n = size < 63 ? size : 63;
                for (std::size_t c = 0; c < n; ++c) e.key[c] = text[c];
                e.key[n] = L'\0';
                e.generic = (size == 3 && text[0] == L'x' && text[1] == L'x' && text[2] == L'x');
                ++count;
              }
            }
          }
        }
      }
    }
    return count;
  }

} // namespace

auto cglobal_data_manager::get_ref_item(std::uint32_t ref_id) -> void* {
  if (ref_id == 0) return nullptr;
  auto* mgr = manager();
  return mgr ? get_ref_item_impl(mgr, ref_id) : nullptr;
}

auto cglobal_data_manager::get_ref_char(std::uint32_t char_id) -> void* {
  if (char_id == 0) return nullptr;
  auto* mgr = manager();
  return mgr ? get_ref_char_impl(mgr, char_id) : nullptr;
}

auto cglobal_data_manager::find_magic_option_name(std::uint16_t opt_id) -> std::wstring {
  auto* mgr = manager();
  if (!mgr) return L"";

  auto* info = find_option_info_impl(mgr, opt_id);
  if (!info || !ext_client::utils::memory::is_game_ptr(info)) return L"";

  // std::wstring at info + 4
  const auto* str_obj = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(info) + 4);
  const auto ref = ext_client::msvc9::wstring_ref::from(str_obj);
  const auto* d = ref.data();
  if (d && *d != L'\0') {
    return std::wstring(d, ref.length());
  }
  return L"";
}

auto cglobal_data_manager::resolve_item_type_key(std::uint32_t type_id, const void* code_name_wstring,
                                                 wchar_t* out_key, std::size_t out_capacity) -> bool {
  if (!code_name_wstring || !out_key || out_capacity == 0) return false;
  out_key[0] = L'\0';

  auto* mgr = manager();
  return mgr && resolve_type_key_impl(mgr, type_id, code_name_wstring, out_key, out_capacity);
}

auto cglobal_data_manager::query_devil_options(std::uint32_t ref_id, std::uint32_t opt_level,
                                               devil_option_entry* out, int capacity) -> int {
  if (!out || capacity <= 0) return 0;
  auto* mgr = manager();
  return mgr ? query_devil_entries_impl(mgr, ref_id, opt_level, out, capacity) : 0;
}
