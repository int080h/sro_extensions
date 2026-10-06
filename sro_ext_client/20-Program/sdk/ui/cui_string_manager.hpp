#pragma once

#include "sdk/game/ctext_string_manager.hpp"
#include "utils/memory.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/string.hpp"

#include <cstdint>
#include <string>

// ---------------------------------------------------------------------------
// CUIStringManager — Localized UI text table (UIIT_*, UIO_*, PARAM_*, SN_* keys)
// Singleton: 0x0117EDA8 | GetString: sub_9E4F00 (__thiscall, returns native wstring)
//
// Every user-visible text is resolved through this manager so the overlay follows the
// language the client was started with. No translated text is ever hardcoded on our side.
// ---------------------------------------------------------------------------
namespace ext_client::sdk::ui {

  // Returns the translation of `key`, or an empty string when the key is unknown
  inline auto get_string(const wchar_t* key) -> std::wstring {
    if (!key || *key == L'\0') return L"";

    auto* mgr = ctext_string_manager::get();
    if (!mgr) return L"";

    const auto ref = mgr->get_text(key);
    const auto* d = ref.data();
    if (d && *d != L'\0') {
      return std::wstring(d, ref.length());
    }

    return L"";
  }

  // `fallback` is only meant for overlay widgets that must have a non-empty label (ImGui IDs)
  // before the client's textdata is available. Tooltip/body text never passes one.
  inline auto get_string_utf8(const wchar_t* key, const char* fallback = "") -> std::string {
    const auto w = get_string(key);
    if (!w.empty()) {
      return ext_client::utils::string::to_utf8(w.c_str());
    }
    return (fallback && *fallback != '\0') ? std::string(fallback) : std::string{};
  }

} // namespace ext_client::sdk::ui

// Canonical class facade mirroring CUIStringManager
class cui_string_manager {
public:
  static constexpr std::uintptr_t k_singleton_addr = 0x0117EDA8;

  static auto get_string(const wchar_t* key) -> std::wstring {
    return ext_client::sdk::ui::get_string(key);
  }

  static auto get_string_utf8(const wchar_t* key, const char* fallback = "") -> std::string {
    return ext_client::sdk::ui::get_string_utf8(key, fallback);
  }
};
