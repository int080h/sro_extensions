#include "ext_client.hpp"

#include "calarm_guide_mgr_wnd.hpp"

#include "calarm_data.hpp"
#include "calarm_entry.hpp"
#include "calarm_store.hpp"
#include "cif_decorated_static.hpp"
#include "cif_alarms.hpp"
#include "cif_manager.hpp"
#include "cg_interface.hpp"
#include "cgwnd.hpp"
#include "cps_character_select.hpp"
#include "cps_title.hpp"
#include "live_instance.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cctype>
#include <cstring>
#include <vector>

namespace cif_manager = ext_client::cif_manager;

namespace {

  calarm_guide_mgr_wnd* g_cached_resolve = nullptr;

  constexpr std::size_t k_effect_widget_count = 3;
  constexpr std::size_t k_path_buf_size = 260;
  constexpr std::size_t k_rigid_promo_slot_first = 6;
  constexpr std::size_t k_rigid_promo_slot_last = 7;
  constexpr unsigned char k_promo_effect_first = 2;
  constexpr unsigned char k_promo_effect_last = 3;

  constexpr const char* k_facebook_icon_tokens[] = {
    "facebook_1.ddj",
    "facebook_2.ddj",
  };

  constexpr const char* k_magic_lamp_icon_tokens[] = {
    "webgacha2_0.ddj",
  };

  constexpr const char* k_daily_login_icon_tokens[] = {
    "dailylogin_0.ddj",
  };

  constexpr int k_facebook_child_ids[] = {6, 400};
  constexpr int k_magic_lamp_child_ids[] = {7, 401, 402};

  using promo_target = calarm_guide_mgr_wnd::promo_target;

  auto is_ui_widget(const void* widget) -> bool;
  auto safe_vftable(const void* widget) -> std::uint32_t;
  auto is_safe_alarm_widget(const void* widget) -> bool;
  auto static_subobject(void* widget) -> void*;
  auto entry_for_slot(calarm_data* data, std::size_t slot_index) -> const calarm_entry*;
  auto is_valid_entry(const calarm_entry* entry) -> bool;

  auto path_has_token(const char* path, const char* token) -> bool {
    if (!path || !token || token[0] == '\0') {
      return false;
    }

    const auto token_len = std::strlen(token);
    for (const char* cursor = path; *cursor != '\0'; ++cursor) {
      if (!ext_client::msvc9::is_readable_ptr(cursor, 1)) {
        return false;
      }

      bool match = true;
      for (std::size_t i = 0; i < token_len; ++i) {
        if (!ext_client::msvc9::is_readable_ptr(cursor + i, 1)) {
          return false;
        }
        const auto left = static_cast<unsigned char>(cursor[i]);
        const auto right = static_cast<unsigned char>(token[i]);
        if (std::tolower(left) != std::tolower(right)) {
          match = false;
          break;
        }
      }
      if (match) {
        return true;
      }
    }
    return false;
  }

  auto path_has_any_token(const char* path, const char* const* tokens, std::size_t count) -> bool {
    if (!path || path[0] == '\0') {
      return false;
    }
    for (std::size_t i = 0; i < count; ++i) {
      if (path_has_token(path, tokens[i])) {
        return true;
      }
    }
    return false;
  }

  auto path_is_facebook_icon(const char* path) -> bool {
    return path_has_any_token(path, k_facebook_icon_tokens, std::size(k_facebook_icon_tokens));
  }

  auto path_is_magic_lamp_icon(const char* path) -> bool {
    return path_has_any_token(path, k_magic_lamp_icon_tokens, std::size(k_magic_lamp_icon_tokens));
  }

  auto path_is_daily_login_icon(const char* path) -> bool {
    return path_has_any_token(path, k_daily_login_icon_tokens, std::size(k_daily_login_icon_tokens));
  }

  auto path_is_promo_icon(const char* path) -> bool {
    return path_is_facebook_icon(path) || path_is_magic_lamp_icon(path) || path_is_daily_login_icon(path);
  }

  auto read_mgr_path(const calarm_guide_mgr_wnd* mgr, std::size_t byte_offset, char* dst, std::size_t dst_count) -> bool {
    if (!mgr || !dst || dst_count == 0) {
      return false;
    }

    dst[0] = '\0';
    if (byte_offset + ext_client::msvc9::string_object_size > ext_client::offsets::calarm_guide_mgr_wnd::size) {
      return false;
    }

    const auto* field = reinterpret_cast<const std::uint8_t*>(mgr) + byte_offset;
    if (!ext_client::msvc9::is_readable_ptr(field, ext_client::msvc9::string_object_size)) {
      return false;
    }

    return ext_client::msvc9::string_ref::from(field).copy_to(dst, dst_count);
  }

  auto slot_path_offset(std::size_t slot_index, bool yellow) -> std::size_t {
    const auto base = yellow ? ext_client::offsets::calarm_guide_mgr_wnd::fields::icon_paths_yellow
                             : ext_client::offsets::calarm_guide_mgr_wnd::fields::icon_paths_red;
    return base + slot_index * ext_client::offsets::calarm_guide_mgr_wnd::icon_path_stride;
  }

  auto read_widget_texture_path(const void* widget, std::size_t byte_offset, char* dst, std::size_t dst_count) -> bool {
    if (!widget || !dst || dst_count == 0) {
      return false;
    }
    dst[0] = '\0';
    const auto* field = reinterpret_cast<const std::uint8_t*>(widget) + byte_offset;
    if (!ext_client::msvc9::is_readable_ptr(field, ext_client::msvc9::string_object_size)) {
      return false;
    }
    return ext_client::msvc9::string_ref::from(field).copy_to(dst, dst_count);
  }

  auto paths_on_object_are_promo(const void* object) -> bool {
    if (!object) {
      return false;
    }

    char path[k_path_buf_size]{};
    if (read_widget_texture_path(object, ext_client::offsets::cif_decorated_static::fields::texture_path_alt, path, sizeof(path)) &&
        path_is_promo_icon(path)) {
      return true;
    }

    path[0] = '\0';
    return read_widget_texture_path(object, ext_client::offsets::cif_decorated_static::fields::texture_path, path, sizeof(path)) &&
           path_is_promo_icon(path);
  }

  auto widget_path_is_promo(const void* widget) -> bool {
    if (!is_ui_widget(widget)) {
      return false;
    }

    if (cif_facebook_link_alarm::is_instance(widget) || cif_magic_lamp_alarm::is_instance(widget) ||
        cif_daily_login_alarm::is_instance(widget)) {
      return true;
    }

    if (!cif_decorated_static::is_instance(widget)) {
      return false;
    }

    if (paths_on_object_are_promo(widget)) {
      return true;
    }

    return paths_on_object_are_promo(static_subobject(const_cast<void*>(widget)));
  }

  auto static_subobject(void* widget) -> void* {
    if (!widget) {
      return nullptr;
    }
    return reinterpret_cast<std::uint8_t*>(widget) + ext_client::offsets::cif_static::subobject_offset;
  }

  auto read_ptr_at(const void* object, std::size_t byte_offset) -> void* {
    const auto* field = reinterpret_cast<const std::uint8_t*>(object) + byte_offset;
    if (!ext_client::msvc9::is_readable_ptr(field, sizeof(void*))) {
      return nullptr;
    }
    return *reinterpret_cast<void* const*>(field);
  }

  auto is_strip_widget_ptr(const void* widget) -> bool {
    return cif_decorated_static::is_instance(widget);
  }

  auto is_loose_ui_widget(const void* widget) -> bool {
    return ext_client::msvc9::is_game_ptr(widget) && is_ui_widget(widget);
  }

  auto raw_effect_ptr(const calarm_guide_mgr_wnd* mgr, unsigned char effect_index_1based) -> void* {
    if (!mgr || effect_index_1based < 1 || effect_index_1based > k_effect_widget_count) {
      return nullptr;
    }
    // sub_7394A0: mov ecx, [ecx+eax*4+574h]
    const auto byte_off = 0x574u + static_cast<std::size_t>(effect_index_1based) * sizeof(void*);
    auto* ptr = read_ptr_at(mgr, byte_off);
    return is_strip_widget_ptr(ptr) ? ptr : nullptr;
  }

  auto loose_effect_ptr(const calarm_guide_mgr_wnd* mgr, unsigned char effect_index_1based) -> void* {
    if (!mgr || effect_index_1based < 1 || effect_index_1based > k_effect_widget_count) {
      return nullptr;
    }
    const auto byte_off = 0x574u + static_cast<std::size_t>(effect_index_1based) * sizeof(void*);
    auto* ptr = read_ptr_at(mgr, byte_off);
    return is_loose_ui_widget(ptr) ? ptr : nullptr;
  }

  auto loose_alarm_slot_ptr(const calarm_guide_mgr_wnd* mgr, std::size_t slot_index) -> void* {
    if (!mgr || slot_index >= ext_client::offsets::calarm_guide_mgr_wnd::slot_count) {
      return nullptr;
    }
    auto* ptr = read_ptr_at(mgr, ext_client::offsets::calarm_guide_mgr_wnd::fields::alarm_slots + slot_index * sizeof(void*));
    return is_loose_ui_widget(ptr) ? ptr : nullptr;
  }

  auto find_mgr_child_by_id(calarm_guide_mgr_wnd* mgr, int control_id) -> cgwnd* {
    if (!mgr) {
      return nullptr;
    }

    const auto* map_obj = reinterpret_cast<const std::uint8_t*>(mgr) + ext_client::offsets::cif_wnd::fields::ui_res_map;
    if (!ext_client::msvc9::is_readable_ptr(map_obj, ext_client::msvc9::ui_res_map_size)) {
      return nullptr;
    }

    const auto* map = reinterpret_cast<const ext_client::msvc9::ui_res_map*>(map_obj);
    auto* child = map->find(control_id, false);
    return is_loose_ui_widget(child) ? static_cast<cgwnd*>(child) : nullptr;
  }

  auto set_promo_effect_visible(calarm_guide_mgr_wnd* mgr, unsigned char effect_index_1based, bool visible) -> void {
    // sub_7394A0 dereferences m_effect_widgets[a2-1] with no null check (crash @ +0x18 if null).
    if (!raw_effect_ptr(mgr, effect_index_1based)) {
      return;
    }

    calarm_guide_mgr_wnd::set_effect_state(mgr, effect_index_1based, visible ? 1 : 0);
  }

  auto hide_widget_visible_only(cgwnd* widget) -> void {
    if (!is_loose_ui_widget(widget)) {
      return;
    }
    cgwnd::set_visible(widget, false);
  }

  auto show_widget_visible_only(cgwnd* widget) -> void {
    if (!is_loose_ui_widget(widget)) {
      return;
    }
    cgwnd::set_visible(widget, true);
  }

  auto hide_widget_loose(cgwnd* widget) -> void {
    if (!is_loose_ui_widget(widget)) {
      return;
    }
    calarm_guide_mgr_wnd::hide_promo_widget(widget);
  }

  auto rigid_slot_target(std::size_t slot_index) -> promo_target {
    if (slot_index == k_rigid_promo_slot_first) {
      return promo_target::facebook;
    }
    if (slot_index == k_rigid_promo_slot_last) {
      return promo_target::magic_lamp;
    }
    return promo_target::none;
  }

  auto entry_matches_target(const calarm_entry* entry, promo_target targets) -> bool {
    if (!is_valid_entry(entry) || static_cast<unsigned>(targets) == 0) {
      return false;
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::facebook) && entry->is_facebook()) {
      return true;
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::magic_lamp) && entry->is_magic_lamp()) {
      return true;
    }
    return calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::daily_login) && entry->is_daily_login();
  }

  auto slot_matches_targets(calarm_data* data, std::size_t index, promo_target targets) -> bool {
    if (static_cast<unsigned>(targets) == 0) {
      return false;
    }
    const auto rigid = rigid_slot_target(index);
    if (rigid != promo_target::none && calarm_guide_mgr_wnd::has_promo_target(targets, rigid)) {
      return true;
    }
    if (const auto* entry = entry_for_slot(data, index)) {
      return entry_matches_target(entry, targets);
    }
    return false;
  }

  auto widget_matches_targets(const void* widget, promo_target targets) -> bool {
    if (!widget || static_cast<unsigned>(targets) == 0 || !is_safe_alarm_widget(widget)) {
      return false;
    }
    const char* type_name = cif_manager::ui_type_name(widget);
    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::web_item_alarm) && std::strcmp(type_name, "CIFWebItemAlram") == 0) {
      return true;
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::macro_guide) && std::strcmp(type_name, "CIFMacroSystemGuide") == 0) {
      return true;
    }
    if (cif_facebook_link_alarm::is_instance(widget)) {
      return calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::facebook);
    }
    if (cif_magic_lamp_alarm::is_instance(widget)) {
      return calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::magic_lamp);
    }
    if (cif_daily_login_alarm::is_instance(widget)) {
      return calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::daily_login);
    }
    char path[k_path_buf_size]{};
    if (cif_decorated_static::is_instance(widget)) {
      if (read_widget_texture_path(widget, ext_client::offsets::cif_decorated_static::fields::texture_path_alt, path, sizeof(path)) ||
          read_widget_texture_path(widget, ext_client::offsets::cif_decorated_static::fields::texture_path, path, sizeof(path))) {
        if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::facebook) && path_is_facebook_icon(path)) {
          return true;
        }
        if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::magic_lamp) && path_is_magic_lamp_icon(path)) {
          return true;
        }
        if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::daily_login) && path_is_daily_login_icon(path)) {
          return true;
        }
      }
    }
    return false;
  }

  auto guide_target_for_widget(const cgwnd* widget) -> promo_target {
    if (!is_loose_ui_widget(widget)) {
      return promo_target::none;
    }
    if (cif_facebook_link_alarm::is_instance(widget)) {
      return promo_target::facebook;
    }
    if (cif_magic_lamp_alarm::is_instance(widget)) {
      return promo_target::magic_lamp;
    }
    if (cif_daily_login_alarm::is_instance(widget)) {
      return promo_target::daily_login;
    }
    const int id = cif_manager::unique_id(widget);
    if (id == ext_client::offsets::calarm_guide_mgr_wnd::guide_ids::facebook) {
      return promo_target::facebook;
    }
    if (id == ext_client::offsets::calarm_guide_mgr_wnd::guide_ids::magic_lamp) {
      return promo_target::magic_lamp;
    }
    if (id == ext_client::offsets::calarm_guide_mgr_wnd::guide_ids::daily_login) {
      return promo_target::daily_login;
    }
    if (id == static_cast<int>(ext_client::offsets::cg_interface::guide_ids::web_item_alarm)) {
      return promo_target::web_item_alarm;
    }
    if (id == static_cast<int>(ext_client::offsets::calarm_guide_mgr_wnd::guide_ids::macro)) {
      return promo_target::macro_guide;
    }
    const char* type_name = cif_manager::ui_type_name(widget);
    if (std::strcmp(type_name, "CIFWebItemAlram") == 0) {
      return promo_target::web_item_alarm;
    }
    if (std::strcmp(type_name, "CIFMacroSystemGuide") == 0) {
      return promo_target::macro_guide;
    }
    return promo_target::none;
  }

  auto hide_promo_control_children(calarm_guide_mgr_wnd* mgr, promo_target targets, bool soft) -> void {
    if (!mgr || static_cast<unsigned>(targets) == 0 || !calarm_guide_mgr_wnd::is_attached_to_iface(mgr)) {
      return;
    }

    const auto hide_child = soft ? hide_widget_visible_only : hide_widget_loose;
    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::facebook)) {
      for (const int control_id : k_facebook_child_ids) {
        hide_child(find_mgr_child_by_id(mgr, control_id));
      }
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::magic_lamp)) {
      for (const int control_id : k_magic_lamp_child_ids) {
        hide_child(find_mgr_child_by_id(mgr, control_id));
      }
    }
  }

  auto show_promo_control_children(calarm_guide_mgr_wnd* mgr, promo_target targets) -> void {
    if (!mgr || static_cast<unsigned>(targets) == 0) {
      return;
    }

    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::facebook)) {
      for (const int control_id : k_facebook_child_ids) {
        show_widget_visible_only(find_mgr_child_by_id(mgr, control_id));
      }
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::magic_lamp)) {
      for (const int control_id : k_magic_lamp_child_ids) {
        show_widget_visible_only(find_mgr_child_by_id(mgr, control_id));
      }
    }
  }

  auto hide_slot_effect(calarm_guide_mgr_wnd* mgr, std::size_t slot_index, unsigned char fx_index, bool soft) -> void {
    const auto hide_widget = soft ? hide_widget_visible_only : hide_widget_loose;
    hide_widget(static_cast<cgwnd*>(loose_alarm_slot_ptr(mgr, slot_index)));
    if (auto* effect = loose_effect_ptr(mgr, fx_index)) {
      hide_widget(static_cast<cgwnd*>(effect));
    }
    set_promo_effect_visible(mgr, fx_index, false);
  }

  auto show_slot_effect(calarm_guide_mgr_wnd* mgr, std::size_t slot_index, unsigned char fx_index) -> void {
    show_widget_visible_only(static_cast<cgwnd*>(loose_alarm_slot_ptr(mgr, slot_index)));
    if (auto* effect = loose_effect_ptr(mgr, fx_index)) {
      show_widget_visible_only(static_cast<cgwnd*>(effect));
    }
    set_promo_effect_visible(mgr, fx_index, true);
  }

  auto hide_promo_slots_visible_only(calarm_guide_mgr_wnd* mgr, promo_target targets) -> void {
    if (!mgr || static_cast<unsigned>(targets) == 0 || !calarm_guide_mgr_wnd::is_attached_to_iface(mgr)) {
      return;
    }

    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::facebook)) {
      hide_widget_visible_only(static_cast<cgwnd*>(loose_alarm_slot_ptr(mgr, k_rigid_promo_slot_first)));
      hide_widget_visible_only(static_cast<cgwnd*>(loose_effect_ptr(mgr, k_promo_effect_first)));
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::magic_lamp)) {
      hide_widget_visible_only(static_cast<cgwnd*>(loose_alarm_slot_ptr(mgr, k_rigid_promo_slot_last)));
      hide_widget_visible_only(static_cast<cgwnd*>(loose_effect_ptr(mgr, k_promo_effect_last)));
    }
  }

  auto hide_promo_strip_targets(calarm_guide_mgr_wnd* mgr, promo_target targets, bool soft = false) -> void {
    if (!mgr || static_cast<unsigned>(targets) == 0 || !calarm_guide_mgr_wnd::is_attached_to_iface(mgr)) {
      return;
    }

    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::facebook)) {
      hide_slot_effect(mgr, k_rigid_promo_slot_first, k_promo_effect_first, soft);
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::magic_lamp)) {
      hide_slot_effect(mgr, k_rigid_promo_slot_last, k_promo_effect_last, soft);
    }

    hide_promo_control_children(mgr, targets, soft);
  }

  auto show_promo_strip_targets(calarm_guide_mgr_wnd* mgr, promo_target targets) -> void {
    if (!mgr || static_cast<unsigned>(targets) == 0) {
      return;
    }

    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::facebook)) {
      show_slot_effect(mgr, k_rigid_promo_slot_first, k_promo_effect_first);
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::magic_lamp)) {
      show_slot_effect(mgr, k_rigid_promo_slot_last, k_promo_effect_last);
    }

    show_promo_control_children(mgr, targets);
  }

  auto slot_has_promo_texture(const calarm_guide_mgr_wnd* mgr, std::size_t slot_index) -> bool {
    if (!mgr || slot_index >= ext_client::offsets::calarm_guide_mgr_wnd::slot_count) {
      return false;
    }
    if (!ext_client::msvc9::is_readable_ptr(mgr, ext_client::offsets::calarm_guide_mgr_wnd::size)) {
      return false;
    }

    char path[k_path_buf_size]{};
    if (read_mgr_path(mgr, slot_path_offset(slot_index, true), path, sizeof(path)) && path_is_promo_icon(path)) {
      return true;
    }

    path[0] = '\0';
    if (read_mgr_path(mgr, slot_path_offset(slot_index, false), path, sizeof(path)) && path_is_promo_icon(path)) {
      return true;
    }

    return false;
  }

  auto is_ui_widget(const void* widget) -> bool {
    if (!ext_client::msvc9::is_game_ptr(widget)) {
      return false;
    }

    std::uint32_t vftable = 0;
    if (!ext_client::msvc9::try_read_u32(widget, &vftable)) {
      return false;
    }

    return ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(static_cast<std::uintptr_t>(vftable)), sizeof(void*));
  }

  auto safe_vftable(const void* widget) -> std::uint32_t {
    if (!widget) {
      return 0;
    }

    std::uint32_t vftable = 0;
    if (!ext_client::msvc9::try_read_u32(widget, &vftable)) {
      return 0;
    }
    return vftable;
  }

  auto is_safe_alarm_widget(const void* widget) -> bool {
    if (!is_ui_widget(widget)) {
      return false;
    }

    const auto vftable = safe_vftable(widget);
    if (vftable == ext_client::offsets::cif_decorated_static::vtable::address ||
        vftable == ext_client::offsets::cif_facebook_link_alram::vtable::address ||
        vftable == ext_client::offsets::cif_magic_lamp_alram::vtable::address ||
        vftable == ext_client::offsets::cif_daily_login_alram::vtable::address ||
        vftable == ext_client::offsets::cif_static::vtable::address || vftable == ext_client::offsets::cif_static::vtable::secondary) {
      return true;
    }
    const char* type_name = cif_manager::ui_type_name(widget);
    return std::strcmp(type_name, "CIFWebItemAlram") == 0 || std::strcmp(type_name, "CIFMacroSystemGuide") == 0;
  }

  auto is_valid_inner_static(const void* widget) -> bool {
    if (!widget) {
      return false;
    }
    const auto* inner = static_cast<const std::uint8_t*>(widget) + ext_client::offsets::cif_static::subobject_offset;
    if (!ext_client::msvc9::is_readable_ptr(inner, sizeof(void*) * 4)) {
      return false;
    }
    const auto vtable = safe_vftable(inner);
    return vtable == ext_client::offsets::cif_static::vtable::address || vtable == ext_client::offsets::cif_static::vtable::secondary ||
           vtable == ext_client::offsets::cif_facebook_link_alram::vtable::secondary ||
           vtable == ext_client::offsets::cif_magic_lamp_alram::vtable::secondary ||
           vtable == ext_client::offsets::cif_daily_login_alram::vtable::secondary;
  }

  auto clear_static_subobject_texture(void* widget) -> void {
    if (!is_valid_inner_static(widget)) {
      return;
    }

    auto* static_sub = static_subobject(widget);
    using set_texture_fn = void(__thiscall*)(void*, int, int*, int, int, int, int, unsigned int, int, int);
    const auto fn = reinterpret_cast<set_texture_fn>(ext_client::offsets::cif_static::functions::set_texture);
    int block = 0;
    fn(static_sub, 0, &block, 0, 0, 0, 0, 0, 0, 0);
  }

  auto clear_outer_decorated_texture(void* widget) -> void {
    if (!is_strip_widget_ptr(widget)) {
      return;
    }

    using set_texture_fn = void(__thiscall*)(void*, int, int*, int, int, int, int, unsigned int);
    const auto fn = reinterpret_cast<set_texture_fn>(ext_client::offsets::cif_decorated_static::functions::set_texture);
    int block = 0;
    fn(widget, 0, &block, 0, 0, 0, 0, 0);
  }

  auto clear_decorated_texture(void* widget) -> void {
    if (!is_strip_widget_ptr(widget) || !is_safe_alarm_widget(widget)) {
      return;
    }

    clear_outer_decorated_texture(widget);
    clear_static_subobject_texture(widget);
  }

  auto is_promo_alarm_widget(const void* widget) -> bool {
    return cif_facebook_link_alarm::is_instance(widget) || cif_magic_lamp_alarm::is_instance(widget) ||
           cif_daily_login_alarm::is_instance(widget);
  }

  auto alarm_store_ptr() -> calarm_store* {
    auto* iface = cg_interface::get();
    if (!iface || !cg_interface::is_instance(iface)) {
      return nullptr;
    }

    return iface->alarm_store();
  }

  auto is_valid_alarm_data(const calarm_data* data) -> bool {
    if (!data) {
      return false;
    }

    const auto needed = ext_client::offsets::calarm_data::entries_begin +
                        ext_client::offsets::calarm_guide_mgr_wnd::slot_count * ext_client::offsets::calarm_entry::stride;
    return ext_client::msvc9::is_readable_ptr(data, needed);
  }

  auto alarm_data_ptr() -> calarm_data* {
    auto* store = alarm_store_ptr();
    if (!store || !ext_client::msvc9::is_readable_ptr(store, ext_client::offsets::calarm_store::fields::alarm_data + sizeof(void*))) {
      return nullptr;
    }

    auto* data = store->alarm_data();
    return is_valid_alarm_data(data) ? data : nullptr;
  }

  auto is_valid_entry(const calarm_entry* entry) -> bool {
    return entry && ext_client::msvc9::is_readable_ptr(entry, ext_client::offsets::calarm_entry::size);
  }

  auto neutralize_promo_entry(calarm_entry* entry) -> void {
    if (!is_valid_entry(entry)) {
      return;
    }
    entry->set_active(false);
    entry->set_type(0);
  }

  auto entry_ref_icon_path(const calarm_entry* entry, char* dst, std::size_t dst_count) -> bool {
    if (!is_valid_entry(entry) || !dst || dst_count == 0) {
      return false;
    }

    dst[0] = '\0';
    auto* ref = static_cast<const std::uint8_t*>(const_cast<calarm_entry*>(entry)->ref_data_ptr());
    const auto path_offset = ext_client::offsets::calarm_ref_data::icon_path;
    if (!ref || !ext_client::msvc9::is_readable_ptr(ref, path_offset + ext_client::msvc9::string_object_size)) {
      return false;
    }

    return ext_client::msvc9::string_ref::from(ref + path_offset).copy_to(dst, dst_count);
  }

  auto clear_mgr_path(calarm_guide_mgr_wnd* mgr, std::size_t byte_offset) -> void {
    if (!mgr || !ext_client::msvc9::is_readable_ptr(mgr, ext_client::offsets::calarm_guide_mgr_wnd::size)) {
      return;
    }
    auto* field = reinterpret_cast<std::uint8_t*>(mgr) + byte_offset;
    if (ext_client::msvc9::is_readable_ptr(field, ext_client::msvc9::string_object_size)) {
      std::memset(field, 0, ext_client::msvc9::string_object_size);
    }
  }

  auto clear_promo_slot_paths(calarm_guide_mgr_wnd* mgr, std::size_t slot_index) -> void {
    clear_mgr_path(mgr, slot_path_offset(slot_index, true));
    clear_mgr_path(mgr, slot_path_offset(slot_index, false));
  }

  auto promo_effect_index(std::size_t slot_index) -> std::size_t {
    if (slot_index == k_rigid_promo_slot_first) {
      return 1;
    }
    if (slot_index == k_rigid_promo_slot_last) {
      return 2;
    }
    return static_cast<std::size_t>(-1);
  }

  auto entry_for_slot(calarm_data* data, std::size_t slot_index) -> const calarm_entry* {
    if (!is_valid_alarm_data(data) || slot_index >= ext_client::offsets::calarm_guide_mgr_wnd::slot_count) {
      return nullptr;
    }

    auto* entry = data->entry(slot_index);
    return is_valid_entry(entry) ? entry : nullptr;
  }

  auto hide_promo_widgets_in_res_map(calarm_guide_mgr_wnd* mgr, promo_target targets, bool soft) -> void {
    if (!mgr || static_cast<unsigned>(targets) == 0) {
      return;
    }

    const auto* map_obj = reinterpret_cast<const std::uint8_t*>(mgr) + ext_client::offsets::cif_wnd::fields::ui_res_map;
    if (!ext_client::msvc9::is_readable_ptr(map_obj, ext_client::msvc9::ui_res_map_size)) {
      return;
    }

    ext_client::msvc9::map_ref::from(map_obj).for_each([&](std::uint32_t, void* value) {
      if (!widget_matches_targets(value, targets)) {
        return;
      }
      // Res-map entries can dangle after remove_guide — visibility only, never texture clears here.
      (void)soft;
      hide_widget_visible_only(static_cast<cgwnd*>(value));
    });
  }

  auto show_promo_widgets_in_res_map(calarm_guide_mgr_wnd* mgr, promo_target targets) -> void {
    if (!mgr || static_cast<unsigned>(targets) == 0) {
      return;
    }

    const auto* map_obj = reinterpret_cast<const std::uint8_t*>(mgr) + ext_client::offsets::cif_wnd::fields::ui_res_map;
    if (!ext_client::msvc9::is_readable_ptr(map_obj, ext_client::msvc9::ui_res_map_size)) {
      return;
    }

    ext_client::msvc9::map_ref::from(map_obj).for_each([&](std::uint32_t, void* value) {
      if (!widget_matches_targets(value, targets)) {
        return;
      }
      show_widget_visible_only(static_cast<cgwnd*>(value));
    });
  }

  auto guide_from_list_value_ptr(void* value_ptr) -> cgwnd* {
    if (!value_ptr || !ext_client::msvc9::is_readable_ptr(value_ptr, sizeof(void*))) {
      return nullptr;
    }
    auto* widget = *reinterpret_cast<cgwnd* const*>(value_ptr);
    if (!ext_client::msvc9::is_game_ptr(widget)) {
      return nullptr;
    }
    return widget;
  }

  struct guide_visit_ctx {
    promo_target targets = promo_target::none;
  };

  auto visit_hide_promo_guide(cgwnd* widget, void* ctx) -> void {
    const auto* visit = static_cast<const guide_visit_ctx*>(ctx);
    if (!visit || static_cast<unsigned>(visit->targets) == 0) {
      return;
    }
    const auto widget_target = guide_target_for_widget(widget);
    if (widget_target == promo_target::none || !calarm_guide_mgr_wnd::has_promo_target(visit->targets, widget_target)) {
      return;
    }
    hide_widget_visible_only(widget);
  }

  auto visit_show_promo_guide(cgwnd* widget, void* ctx) -> void {
    const auto* visit = static_cast<const guide_visit_ctx*>(ctx);
    if (!visit || static_cast<unsigned>(visit->targets) == 0) {
      return;
    }
    const auto widget_target = guide_target_for_widget(widget);
    if (widget_target == promo_target::none || !calarm_guide_mgr_wnd::has_promo_target(visit->targets, widget_target)) {
      return;
    }
    show_widget_visible_only(widget);
  }

  auto hide_promo_guides_for(calarm_guide_mgr_wnd* mgr, promo_target targets) -> void {
    if (!calarm_guide_mgr_wnd::mgr_ready(mgr) || static_cast<unsigned>(targets) == 0) {
      return;
    }

    guide_visit_ctx ctx{targets};
    calarm_guide_mgr_wnd::for_each_guide(mgr, visit_hide_promo_guide, &ctx);
  }

  auto show_promo_guides_for(calarm_guide_mgr_wnd* mgr, promo_target targets) -> void {
    if (!calarm_guide_mgr_wnd::mgr_ready(mgr) || static_cast<unsigned>(targets) == 0) {
      return;
    }

    guide_visit_ctx ctx{targets};
    calarm_guide_mgr_wnd::for_each_guide(mgr, visit_show_promo_guide, &ctx);
  }

  auto remove_promo_guides_for(calarm_guide_mgr_wnd* mgr, promo_target targets) -> void {
    if (!calarm_guide_mgr_wnd::mgr_ready(mgr) || static_cast<unsigned>(targets) == 0) {
      return;
    }

    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::magic_lamp)) {
      mgr->remove_guide(ext_client::offsets::calarm_guide_mgr_wnd::guide_ids::magic_lamp);
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::facebook)) {
      mgr->remove_guide(ext_client::offsets::calarm_guide_mgr_wnd::guide_ids::facebook);
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::daily_login)) {
      mgr->remove_guide(ext_client::offsets::calarm_guide_mgr_wnd::guide_ids::daily_login);
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::web_item_alarm)) {
      mgr->remove_guide(static_cast<int>(ext_client::offsets::cg_interface::guide_ids::web_item_alarm));
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, promo_target::macro_guide)) {
      mgr->remove_guide(static_cast<int>(ext_client::offsets::calarm_guide_mgr_wnd::guide_ids::macro));
    }
  }

  struct guide_remove_ctx {
    calarm_guide_mgr_wnd* mgr = nullptr;
    promo_target targets = promo_target::none;
  };

  auto visit_remove_promo_guide_by_unique_id(cgwnd* widget, void* ctx) -> void {
    auto* visit = static_cast<guide_remove_ctx*>(ctx);
    if (!visit || !visit->mgr || static_cast<unsigned>(visit->targets) == 0) {
      return;
    }

    const auto widget_target = guide_target_for_widget(widget);
    if (widget_target == promo_target::none || !calarm_guide_mgr_wnd::has_promo_target(visit->targets, widget_target)) {
      return;
    }

    const int guide_uid = cif_manager::unique_id(widget);
    if (guide_uid > 0) {
      visit->mgr->remove_guide(guide_uid);
    }
  }

  auto remove_promo_guides_by_unique_id(calarm_guide_mgr_wnd* mgr, promo_target targets) -> void {
    if (!calarm_guide_mgr_wnd::mgr_ready(mgr) || static_cast<unsigned>(targets) == 0) {
      return;
    }

    guide_remove_ctx ctx{mgr, targets};
    calarm_guide_mgr_wnd::for_each_guide(mgr, visit_remove_promo_guide_by_unique_id, &ctx);
  }

  struct hide_walk_ctx {
    promo_target targets = promo_target::none;
  };

  auto visit_hide_promo_in_hud(cgwnd* widget, void* ctx) -> void {
    const auto* visit = static_cast<const hide_walk_ctx*>(ctx);
    if (!visit || static_cast<unsigned>(visit->targets) == 0 || !is_loose_ui_widget(widget)) {
      return;
    }
    if (!widget_matches_targets(widget, visit->targets)) {
      return;
    }
    cgwnd::set_visible(widget, false);
  }

  auto hide_promo_widgets_on_mgr(calarm_guide_mgr_wnd* mgr, promo_target targets) -> void {
    if (!mgr || static_cast<unsigned>(targets) == 0) {
      return;
    }

    hide_walk_ctx ctx{targets};
    cif_manager::walk_each(static_cast<cgwnd*>(mgr), 12, visit_hide_promo_in_hud, &ctx);
  }

  auto collect_promo_mgrs(cg_interface* iface, calarm_guide_mgr_wnd** out, std::size_t cap) -> std::size_t {
    if (!iface || !out || cap == 0) {
      return 0;
    }

    const int child_ids[] = {
      ext_client::offsets::cg_interface::child_ids::guide_host,
      ext_client::offsets::cg_interface::child_ids::alarm_guide_mgr,
      ext_client::offsets::cg_interface::child_ids::alarm_guide_mgr_alt,
    };

    std::size_t count = 0;
    for (const int child_id : child_ids) {
      auto* child = iface->get_ui_child(child_id, true);
      auto* mgr = calarm_guide_mgr_wnd::from(child);
      if (!mgr || !calarm_guide_mgr_wnd::is_instance(mgr)) {
        continue;
      }

      bool duplicate = false;
      for (std::size_t i = 0; i < count; ++i) {
        if (out[i] == mgr) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) {
        continue;
      }

      out[count++] = mgr;
      if (count >= cap) {
        break;
      }
    }

    return count;
  }

  auto hide_promo_widgets_in_hud(cg_interface* iface, promo_target targets) -> void {
    if (!iface || static_cast<unsigned>(targets) == 0) {
      return;
    }

    auto* root = iface->as_gwnd();
    if (!is_loose_ui_widget(root)) {
      return;
    }

    hide_walk_ctx ctx{targets};
    cif_manager::walk_each(root, 24, visit_hide_promo_in_hud, &ctx);
  }

} // namespace

auto calarm_guide_mgr_wnd::has_promo_target(promo_target mask, promo_target bit) -> bool {
  return (static_cast<unsigned>(mask) & static_cast<unsigned>(bit)) != 0;
}

auto calarm_guide_mgr_wnd::set_current(calarm_guide_mgr_wnd* instance) -> void {
  ext_client::sdk::live_instance<calarm_guide_mgr_wnd>::set(instance);
  g_cached_resolve = instance;
}

auto calarm_guide_mgr_wnd::for_each_guide(const calarm_guide_mgr_wnd* mgr, void (*visit)(cgwnd*, void*), void* ctx) -> void {
  if (!mgr || !visit) {
    return;
  }

  const auto* list_obj = reinterpret_cast<const std::uint8_t*>(mgr) + ext_client::offsets::calarm_guide_mgr_wnd::fields::guide_list;
  if (!ext_client::msvc9::is_readable_ptr(list_obj, 8)) {
    return;
  }

  ext_client::msvc9::list_ref::from(list_obj).for_each([&](void* value_ptr) {
    if (auto* widget = guide_from_list_value_ptr(value_ptr)) {
      visit(widget, ctx);
    }
  });
}

auto calarm_guide_mgr_wnd::current() -> calarm_guide_mgr_wnd* {
  auto* instance = ext_client::sdk::live_instance<calarm_guide_mgr_wnd>::get();
  if (instance && is_instance(instance)) {
    return instance;
  }
  return nullptr;
}

auto calarm_guide_mgr_wnd::mgr_ready(const calarm_guide_mgr_wnd* mgr) -> bool {
  if (!mgr || !is_instance(mgr)) {
    return false;
  }
  return ext_client::msvc9::is_readable_ptr(mgr, ext_client::offsets::calarm_guide_mgr_wnd::size);
}

auto calarm_guide_mgr_wnd::is_attached_to_iface(const calarm_guide_mgr_wnd* mgr) -> bool {
  if (!mgr_ready(mgr)) {
    return false;
  }

  auto* iface = cg_interface::get();
  if (!iface || !cg_interface::is_instance(iface)) {
    return false;
  }

  auto* live = iface->alarm_guide_mgr();
  return live == mgr;
}

namespace {

  auto is_live_outer_screen(const void* ptr, std::uint32_t expected_vftable) -> bool {
    if (!ptr) {
      return false;
    }
    std::uint32_t vft = 0;
    if (!ext_client::msvc9::try_read_u32(ptr, &vft)) {
      return false;
    }
    return vft == expected_vftable;
  }

} // namespace

auto outer_screen_blocks_alarm_strip() -> bool {
  if (is_live_outer_screen(cps_character_select::current(), ext_client::offsets::cps_character_select::vtable::address)) {
    return true;
  }
  if (is_live_outer_screen(cps_title::current(), ext_client::offsets::cps_title::vtable::address)) {
    return true;
  }
  return false;
}

auto calarm_guide_mgr_wnd::ingame_alarm_strip_active() -> bool {
  if (outer_screen_blocks_alarm_strip()) {
    return false;
  }

  auto* iface = cg_interface::get();
  if (!iface || !cg_interface::is_instance(iface)) {
    return false;
  }

  // Login / char-select transitions can leave g_CGInterface set before the HUD exists.
  if (!iface->get_ui_child(ext_client::offsets::cg_interface::child_ids::main_hud, true)) {
    return false;
  }

  auto* mgr = iface->alarm_guide_mgr();
  if (!mgr || !is_attached_to_iface(mgr) || !mgr_ready(mgr)) {
    return false;
  }

  return refresh_slots_safe(mgr);
}

auto calarm_guide_mgr_wnd::ingame_promo_ui_active() -> bool {
  if (outer_screen_blocks_alarm_strip()) {
    return false;
  }

  auto* iface = cg_interface::get();
  if (!iface || !cg_interface::is_instance(iface)) {
    return false;
  }

  if (!iface->get_ui_child(ext_client::offsets::cg_interface::child_ids::main_hud, true)) {
    return false;
  }

  auto* host = from(iface->guide_host(false));
  return host && is_instance(host) && mgr_ready(host);
}

auto calarm_guide_mgr_wnd::invalidate_cache() -> void {
  g_cached_resolve = nullptr;
  calarm_guide_mgr_wnd::set_current(nullptr);
}

auto remember_mgr(calarm_guide_mgr_wnd* mgr) -> calarm_guide_mgr_wnd* {
  if (!mgr || !calarm_guide_mgr_wnd::is_attached_to_iface(mgr)) {
    return nullptr;
  }
  g_cached_resolve = mgr;
  return mgr;
}

auto try_cheap_child_lookup(cg_interface* iface) -> calarm_guide_mgr_wnd* {
  if (auto* child = iface->get_ui_child(ext_client::offsets::cg_interface::child_ids::alarm_guide_mgr_alt, true)) {
    if (auto* mgr = remember_mgr(calarm_guide_mgr_wnd::from(child))) {
      return mgr;
    }
  }

  if (auto* child = iface->get_ui_child(ext_client::offsets::cg_interface::child_ids::alarm_guide_mgr, true)) {
    if (auto* mgr = remember_mgr(calarm_guide_mgr_wnd::from(child))) {
      return mgr;
    }
  }

  return nullptr;
}

auto find_mgr_in_res_map(cg_interface* iface) -> calarm_guide_mgr_wnd* {
  if (!iface || !cg_interface::is_instance(iface)) {
    return nullptr;
  }

  const auto* map_obj = iface->ui_res_map();
  if (!ext_client::msvc9::is_readable_ptr(map_obj, ext_client::msvc9::ui_res_map_size)) {
    return nullptr;
  }

  calarm_guide_mgr_wnd* found = nullptr;
  ext_client::msvc9::map_ref::from(map_obj).for_each([&](std::uint32_t, void* value) {
    if (found || !ext_client::msvc9::is_game_ptr(value)) {
      return;
    }
    if (auto* mgr = calarm_guide_mgr_wnd::from(value); calarm_guide_mgr_wnd::is_instance(mgr)) {
      found = mgr;
    }
  });
  return remember_mgr(found);
}

auto calarm_guide_mgr_wnd::resolve() -> calarm_guide_mgr_wnd* {
  if (!ingame_alarm_strip_active()) {
    invalidate_cache();
    return nullptr;
  }

  if (auto* mgr = current(); mgr && is_attached_to_iface(mgr)) {
    return mgr;
  }

  if (g_cached_resolve && is_attached_to_iface(g_cached_resolve)) {
    return g_cached_resolve;
  }

  invalidate_cache();

  auto* iface = cg_interface::get();
  if (!iface || !cg_interface::is_instance(iface)) {
    return nullptr;
  }

  if (auto* mgr = iface->alarm_guide_mgr(); mgr && mgr_ready(mgr)) {
    return remember_mgr(mgr);
  }

  return try_cheap_child_lookup(iface);
}

auto calarm_guide_mgr_wnd::resolve_from_res_map() -> calarm_guide_mgr_wnd* {
  if (auto* mgr = resolve()) {
    return mgr;
  }

  auto* iface = cg_interface::get();
  if (!iface || !cg_interface::is_instance(iface)) {
    return nullptr;
  }

  return find_mgr_in_res_map(iface);
}

auto calarm_guide_mgr_wnd::hide_promo_widget(cgwnd* widget) -> void {
  if (!is_loose_ui_widget(widget)) {
    return;
  }
  // Visibility only — texture clears dereference inner CIFStatic and AV on stale slots.
  cgwnd::set_visible(widget, false);
}

auto calarm_guide_mgr_wnd::loose_slot_widget(const calarm_guide_mgr_wnd* mgr, std::size_t index) -> cgwnd* {
  auto* slot = loose_alarm_slot_ptr(mgr, index);
  return is_loose_ui_widget(slot) ? static_cast<cgwnd*>(slot) : nullptr;
}

auto calarm_guide_mgr_wnd::loose_effect_widget(const calarm_guide_mgr_wnd* mgr, std::size_t index_0based) -> cgwnd* {
  if (index_0based >= k_effect_widget_count) {
    return nullptr;
  }
  auto* effect = loose_effect_ptr(mgr, static_cast<unsigned char>(index_0based + 1));
  return is_loose_ui_widget(effect) ? static_cast<cgwnd*>(effect) : nullptr;
}

auto calarm_guide_mgr_wnd::refresh_slots_safe(const calarm_guide_mgr_wnd* mgr) -> bool {
  if (!mgr_ready(mgr)) {
    return false;
  }

  for (std::size_t i = 0; i < ext_client::offsets::calarm_guide_mgr_wnd::slot_count; ++i) {
    if (mgr->alarm_status(i) == 0) {
      continue;
    }
    auto* slot = loose_alarm_slot_ptr(mgr, i);
    if (!slot) {
      return false;
    }
    const auto* inner = static_cast<const std::uint8_t*>(slot) + ext_client::offsets::cif_static::subobject_offset;
    if (!ext_client::msvc9::is_readable_ptr(inner, sizeof(void*) * 4)) {
      return false;
    }
  }
  return true;
}

auto calarm_guide_mgr_wnd::read_slot_debug(const calarm_guide_mgr_wnd* mgr, std::size_t index, slot_debug_info& out) -> void {
  out = {};
  if (!mgr || index >= ext_client::offsets::calarm_guide_mgr_wnd::slot_count) {
    return;
  }
  if (!ext_client::msvc9::is_readable_ptr(mgr, ext_client::offsets::calarm_guide_mgr_wnd::size)) {
    return;
  }

  out.alarm_status = mgr->alarm_status(index);
  if (auto* slot = loose_alarm_slot_ptr(mgr, index)) {
    out.slot_widget = reinterpret_cast<cif_decorated_static*>(slot);
    read_widget_texture_path(slot, ext_client::offsets::cif_decorated_static::fields::texture_path, out.tex_path, sizeof(out.tex_path));
  }
  if (const auto fx = promo_effect_index(index); fx < k_effect_widget_count) {
    const auto fx_index = static_cast<unsigned char>(fx + 1);
    if (auto* effect = loose_effect_ptr(mgr, fx_index)) {
      out.effect_widget = reinterpret_cast<cif_decorated_static*>(effect);
    }
  }

  auto* data = alarm_data_ptr();
  if (const auto* entry = entry_for_slot(data, index)) {
    out.entry_type = entry->type();
    out.entry_active = entry->active();
    entry_ref_icon_path(entry, out.ref_icon_path, sizeof(out.ref_icon_path));
  }

  read_mgr_path(mgr, slot_path_offset(index, true), out.yellow_path, sizeof(out.yellow_path));
  read_mgr_path(mgr, slot_path_offset(index, false), out.red_path, sizeof(out.red_path));
}

auto calarm_guide_mgr_wnd::alarm_slot(std::size_t index) -> cif_decorated_static* {
  if (index >= ext_client::offsets::calarm_guide_mgr_wnd::slot_count) {
    return nullptr;
  }
  return m_alarm_slots[index];
}

auto calarm_guide_mgr_wnd::alarm_slot(std::size_t index) const -> const cif_decorated_static* {
  return const_cast<calarm_guide_mgr_wnd*>(this)->alarm_slot(index);
}

auto calarm_guide_mgr_wnd::effect_widget(std::size_t index) -> cif_decorated_static* {
  if (index >= k_effect_widget_count) {
    return nullptr;
  }
  return m_effect_widgets[index];
}

auto calarm_guide_mgr_wnd::effect_widget(std::size_t index) const -> const cif_decorated_static* {
  return const_cast<calarm_guide_mgr_wnd*>(this)->effect_widget(index);
}

auto calarm_guide_mgr_wnd::alarm_status(std::size_t index) const -> std::uint8_t {
  if (index >= ext_client::offsets::calarm_guide_mgr_wnd::slot_count) {
    return 0;
  }
  return m_alarm_status[index];
}

auto clear_promo_alarm_data(calarm_guide_mgr_wnd* mgr, promo_target targets) -> void {
  if (!mgr || static_cast<unsigned>(targets) == 0) {
    return;
  }

  auto* data = alarm_data_ptr();
  for (std::size_t i = 0; i < ext_client::offsets::calarm_guide_mgr_wnd::slot_count; ++i) {
    if (!slot_matches_targets(data, i, targets)) {
      continue;
    }

    if (auto* entry = const_cast<calarm_entry*>(entry_for_slot(data, i))) {
      neutralize_promo_entry(entry);
    }

    mgr->m_alarm_status[i] = 0;
    mgr->m_slot_flags[i] = 0;
    mgr->m_slot_extra_state[i] = 0;
    clear_promo_slot_paths(mgr, i);
  }
}

auto calarm_guide_mgr_wnd::suppress_promo(promo_target targets) -> void {
  if (!mgr_ready(this) || static_cast<unsigned>(targets) == 0) {
    return;
  }

  clear_promo_alarm_data(this, targets);
  hide_promo_strip_targets(this, targets);
}

auto calarm_guide_mgr_wnd::suppress_promo_strip() -> void {
  suppress_promo(k_promo_all);
}

auto calarm_guide_mgr_wnd::hide_promo_effects(promo_target targets) -> void {
  hide_promo_strip_targets(this, targets, true);
}

auto calarm_guide_mgr_wnd::hide_promo_guides(promo_target targets) -> void {
  hide_promo_guides_for(this, targets);
}

auto calarm_guide_mgr_wnd::remove_promo_guides(promo_target targets) -> void {
  remove_promo_guides_for(this, targets);
}

auto calarm_guide_mgr_wnd::apply_promo_hide(promo_target targets) -> void {
  if (!is_attached_to_iface(this) || static_cast<unsigned>(targets) == 0) {
    return;
  }

  // Alarm strip child (0x9D): calarm_store drives slot icons via sub_739A80 -> sub_739940.
  // Promo corner guides (278/267/280) live on guide_host (0x9E) — use apply_iface_promo_hide.
  clear_promo_alarm_data(this, targets);
  if (refresh_slots_safe(this)) {
    update_alarm_state();
  }
}

auto calarm_guide_mgr_wnd::apply_promo_show(promo_target targets) -> void {
  if (!is_attached_to_iface(this) || static_cast<unsigned>(targets) == 0) {
    return;
  }

  (void)targets;
  if (refresh_slots_safe(this)) {
    update_alarm_state();
  }
}

auto calarm_guide_mgr_wnd::update_alarm_state() -> int {
  return mgr_vftable()->update_alarm_state(this);
}

auto calarm_guide_mgr_wnd::soft_hide_promo(promo_target targets) -> void {
  if (!mgr_ready(this) || static_cast<unsigned>(targets) == 0) {
    return;
  }

  auto* data = alarm_data_ptr();
  hide_promo_guides(targets);
  hide_promo_strip_targets(this, targets, true);

  for (std::size_t i = 0; i < ext_client::offsets::calarm_guide_mgr_wnd::slot_count; ++i) {
    auto* slot = read_ptr_at(this, ext_client::offsets::calarm_guide_mgr_wnd::fields::alarm_slots + i * sizeof(void*));
    if (!is_loose_ui_widget(slot)) {
      continue;
    }
    if (slot_matches_targets(data, i, targets) || slot_has_promo_texture(this, i) || widget_matches_targets(slot, targets)) {
      hide_widget_visible_only(static_cast<cgwnd*>(slot));
    }
  }

  hide_promo_widgets_in_res_map(this, targets, true);
}

auto calarm_guide_mgr_wnd::restore_promo_visibility(promo_target targets) -> void {
  if (!mgr_ready(this) || static_cast<unsigned>(targets) == 0) {
    return;
  }

  auto* data = alarm_data_ptr();
  show_promo_guides_for(this, targets);
  show_promo_strip_targets(this, targets);

  for (std::size_t i = 0; i < ext_client::offsets::calarm_guide_mgr_wnd::slot_count; ++i) {
    auto* slot = read_ptr_at(this, ext_client::offsets::calarm_guide_mgr_wnd::fields::alarm_slots + i * sizeof(void*));
    if (!is_loose_ui_widget(slot)) {
      continue;
    }
    if (slot_matches_targets(data, i, targets) || slot_has_promo_texture(this, i) || widget_matches_targets(slot, targets)) {
      show_widget_visible_only(static_cast<cgwnd*>(slot));
    }
  }

  show_promo_widgets_in_res_map(this, targets);
}

auto calarm_guide_mgr_wnd::hide_promo(promo_target targets) -> void {
  if (!mgr_ready(this) || static_cast<unsigned>(targets) == 0) {
    return;
  }

  auto* data = alarm_data_ptr();
  clear_promo_alarm_data(this, targets);
  remove_promo_guides(targets);
  hide_promo_guides(targets);
  hide_promo_strip_targets(this, targets);

  for (std::size_t i = 0; i < ext_client::offsets::calarm_guide_mgr_wnd::slot_count; ++i) {
    auto* slot = read_ptr_at(this, ext_client::offsets::calarm_guide_mgr_wnd::fields::alarm_slots + i * sizeof(void*));
    if (!is_loose_ui_widget(slot)) {
      continue;
    }
    if (slot_matches_targets(data, i, targets) || slot_has_promo_texture(this, i) || widget_matches_targets(slot, targets)) {
      hide_widget_loose(static_cast<cgwnd*>(slot));
    }
  }

  hide_promo_widgets_in_res_map(this, targets, false);
}

auto calarm_guide_mgr_wnd::hide_promo_strip() -> void {
  hide_promo(k_promo_all);
}

auto calarm_guide_mgr_wnd::guide_icon_count() const -> std::uint8_t {
  const auto* field = reinterpret_cast<const std::uint8_t*>(this) + ext_client::offsets::calarm_guide_mgr_wnd::fields::guide_icon_count;
  if (!ext_client::msvc9::is_readable_ptr(field, sizeof(std::uint8_t))) {
    return 0;
  }
  return *field;
}

auto calarm_guide_mgr_wnd::get_guide(const unsigned guide_id) -> cgwnd* {
  if (!calarm_guide_mgr_wnd::mgr_ready(this)) {
    return nullptr;
  }

  using get_guide_fn = void*(__thiscall*)(calarm_guide_mgr_wnd * self, unsigned int guide_id);
  const auto fn = reinterpret_cast<get_guide_fn>(ext_client::offsets::calarm_guide_mgr_wnd::functions::get_guide);
  auto* raw = fn(this, guide_id);
  return ext_client::msvc9::is_game_ptr(raw) ? static_cast<cgwnd*>(raw) : nullptr;
}

auto calarm_guide_mgr_wnd::create_guide_icon(const int guide_id) -> cgwnd* {
  return get_guide(static_cast<unsigned int>(guide_id));
}

auto calarm_guide_mgr_wnd::remove_guide(const int guide_id) -> int {
  if (!calarm_guide_mgr_wnd::mgr_ready(this)) {
    return 0;
  }

  using remove_guide_fn = int(__thiscall*)(calarm_guide_mgr_wnd * self, int guide_id);
  const auto fn = reinterpret_cast<remove_guide_fn>(ext_client::offsets::calarm_guide_mgr_wnd::functions::remove_guide);
  return fn(this, guide_id);
}

auto calarm_guide_mgr_wnd::update_guide_positions() -> int {
  if (!calarm_guide_mgr_wnd::mgr_ready(this)) {
    return 0;
  }

  using update_positions_fn = int(__thiscall*)(calarm_guide_mgr_wnd * self);
  const auto fn = reinterpret_cast<update_positions_fn>(ext_client::offsets::calarm_guide_mgr_wnd::functions::update_positions);
  return fn(this);
}

auto calarm_guide_mgr_wnd::is_guide_available(const unsigned guide_id) const -> bool {
  if (!calarm_guide_mgr_wnd::mgr_ready(this)) {
    return false;
  }

  struct ctx_t {
    unsigned int wanted = 0;
    bool found = false;
  } ctx{guide_id, false};

  calarm_guide_mgr_wnd::for_each_guide(
    this,
    [](cgwnd* widget, void* raw) {
      auto* visit = static_cast<ctx_t*>(raw);
      if (!widget || !visit) {
        return;
      }
      if (static_cast<unsigned int>(cif_manager::unique_id(widget)) == visit->wanted) {
        visit->found = true;
      }
    },
    &ctx);

  return ctx.found;
}

auto calarm_guide_mgr_wnd::remove_all_guides() -> void {
  if (!calarm_guide_mgr_wnd::mgr_ready(this)) {
    return;
  }

  std::vector<int> guide_ids;
  calarm_guide_mgr_wnd::for_each_guide(
    this,
    [](cgwnd* widget, void* ctx) {
      if (!widget || !ctx) {
        return;
      }
      static_cast<std::vector<int>*>(ctx)->push_back(cif_manager::unique_id(widget));
    },
    &guide_ids);

  for (const int id : guide_ids) {
    remove_guide(id);
  }
}

auto calarm_guide_mgr_wnd::set_effect_state(calarm_guide_mgr_wnd* mgr, const unsigned char fx_index, const int visible) -> unsigned char {
  if (!mgr) {
    return fx_index;
  }

  using set_effect_state_fn = unsigned char(__thiscall*)(calarm_guide_mgr_wnd * self, unsigned char fx_index, int visible);
  const auto fn = reinterpret_cast<set_effect_state_fn>(ext_client::offsets::calarm_guide_mgr_wnd::functions::set_effect_state);
  return fn(mgr, fx_index, visible);
}

auto calarm_guide_mgr_wnd::show_promo_effect(calarm_guide_mgr_wnd* mgr, const char slot_index) -> int {
  if (!mgr) {
    return 0;
  }

  using show_promo_effect_fn = int(__thiscall*)(calarm_guide_mgr_wnd * self, char slot_index);
  const auto fn = reinterpret_cast<show_promo_effect_fn>(ext_client::offsets::calarm_guide_mgr_wnd::functions::show_promo_effect);
  return fn(mgr, slot_index);
}

auto calarm_guide_mgr_wnd::refresh_slot_icons(calarm_guide_mgr_wnd* mgr) -> int {
  if (!mgr || !calarm_guide_mgr_wnd::refresh_slots_safe(mgr)) {
    return 0;
  }

  using refresh_slot_icons_fn = int(__thiscall*)(calarm_guide_mgr_wnd * self);
  const auto fn = reinterpret_cast<refresh_slot_icons_fn>(ext_client::offsets::calarm_guide_mgr_wnd::functions::refresh_slot_icons);
  return fn(mgr);
}

namespace {

  auto set_iface_guides_visible(cg_interface* iface, calarm_guide_mgr_wnd::promo_target targets, bool visible) -> void {
    if (!iface || static_cast<unsigned>(targets) == 0) {
      return;
    }

    const char show = visible ? 1 : 0;
    if (calarm_guide_mgr_wnd::has_promo_target(targets, calarm_guide_mgr_wnd::promo_target::facebook)) {
      iface->show_facebook_guide(show != 0);
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, calarm_guide_mgr_wnd::promo_target::magic_lamp)) {
      iface->show_magic_lamp_guide(show != 0);
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, calarm_guide_mgr_wnd::promo_target::daily_login)) {
      iface->show_daily_login_guide(show != 0);
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, calarm_guide_mgr_wnd::promo_target::web_item_alarm)) {
      iface->show_web_item_alarm_guide(show != 0);
    }
    if (calarm_guide_mgr_wnd::has_promo_target(targets, calarm_guide_mgr_wnd::promo_target::macro_guide)) {
      iface->show_macro_guide(show != 0);
    }
  }

  auto strip_mgr_from_iface(cg_interface* iface) -> calarm_guide_mgr_wnd* {
    if (!iface) {
      return nullptr;
    }

    auto* mgr = iface->alarm_guide_mgr();
    if (!mgr || !calarm_guide_mgr_wnd::is_instance(mgr) || !calarm_guide_mgr_wnd::mgr_ready(mgr)) {
      return nullptr;
    }
    return mgr;
  }

} // namespace

auto calarm_guide_mgr_wnd::apply_iface_promo_hide(cg_interface* iface, promo_target targets) -> void {
  if (!cg_interface::is_ingame_hud_ready() || static_cast<unsigned>(targets) == 0) {
    return;
  }
  if (!iface || !cg_interface::is_instance(iface)) {
    return;
  }

  set_iface_guides_visible(iface, targets, false);

  calarm_guide_mgr_wnd* mgrs[3] = {};
  const std::size_t mgr_count = collect_promo_mgrs(iface, mgrs, 3);
  for (std::size_t i = 0; i < mgr_count; ++i) {
    remove_promo_guides_for(mgrs[i], targets);
  }

  if (auto* strip = strip_mgr_from_iface(iface)) {
    strip->apply_promo_hide(targets);
  }
}

auto calarm_guide_mgr_wnd::apply_iface_promo_show(cg_interface* iface, promo_target targets) -> void {
  if (!cg_interface::is_ingame_hud_ready() || static_cast<unsigned>(targets) == 0) {
    return;
  }
  if (!iface || !cg_interface::is_instance(iface)) {
    return;
  }

  // sub_884720 show branch: get_ui_child(0x9E) then sub_7390B0 GetGuide (creates if missing).
  set_iface_guides_visible(iface, targets, true);

  if (auto* strip = strip_mgr_from_iface(iface)) {
    strip->apply_promo_show(targets);
  }
}
