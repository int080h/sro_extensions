#pragma once

#include "sdk/ui/cif_wnd.hpp"
#include "sdk/game/cso_item.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// CIFSlotWithHelp — Native item slot widget with tooltip helper (source: IFSlotWithHelp.cpp)
// Native VTable: 0x00FF7A3C (CGWnd) / 0x00FF79F4 (CTextBoard @ +0x84) | Class Size: 0x06F8
// Base: CIFWnd (CGWnd + CTextBoard)
//
// The client builds an item tooltip in a type dispatcher (sub_76C0E0) that switches on tid3 of the
// CRefObjItem TypeID and calls one builder per family:
//   tid3 1,2,3,9,10,11  armor            sub_768B10
//   tid3 4              shield           sub_7693D0
//   tid3 5,12           accessory        sub_768EF0
//   tid3 6              weapon           sub_769160
//   tid3 7              job suit         sub_768550
//   tid3 13             avatar           sub_768770
//   tid3 14             devil's spirit   sub_75E9E0
// Shared parts: title sub_75DFE0/sub_74D1C0, degree sub_7577E0, description sub_74CE80,
// stats sub_7494B0, requirements sub_766B90, magic option slots sub_74A0A0, magic options
// sub_75C2D0, advanced elixir sub_74D5F0.
//
// This module only produces the *data* of those lines (every text resolved through
// CUIStringManager). Drawing it is the job of render/item_tooltip_renderer.
// ---------------------------------------------------------------------------
namespace ext_client::sdk::ui {

  // ARGB colours the client passes to its tooltip list (sub_85C6D0 arguments)
  namespace tooltip_argb {
    inline constexpr std::uint32_t white     = 0xFFFFFFFFu;
    inline constexpr std::uint32_t heading   = 0xFFEFDAA4u; // -1058140: "Sort of item", section headings
    inline constexpr std::uint32_t magic     = 0xFF00EAFFu; // -16717057: magic option lines
    inline constexpr std::uint32_t devil_add = 0xFF6CE675u; // -9640331: devil additional options
  } // namespace tooltip_argb

  // One coloured text line
  struct tooltip_line {
    std::string text;
    std::uint32_t argb{tooltip_argb::white};
  };

  // Heading + lines block (Devil's Spirit: basic / additional / magic options)
  struct tooltip_section {
    std::string heading;
    std::uint32_t heading_argb{tooltip_argb::heading};
    std::vector<tooltip_line> lines;
  };

  struct item_tooltip_data {
    std::string title;
    std::string subtitle;
    std::string code_name;
    std::string slot_name;
    std::string icon_path;
    bool is_sox{false};
    std::uint8_t opt_level{0};
    std::uint8_t degree{0};
    std::uint32_t req_level{0};
    std::uint32_t current_durability{0};
    std::uint32_t max_durability{0};
    std::uint8_t durability_pct{0};
    std::uint8_t adv_elixir_level{0};
    std::uint32_t stack_count{1};

    // Combat attributes
    bool is_weapon{false};
    bool is_shield{false};
    bool is_armor{false};
    bool is_accessory{false};

    std::uint32_t min_phy_atk{0};
    std::uint32_t max_phy_atk{0};
    std::uint8_t phy_atk_pct{0};

    std::uint32_t min_mag_atk{0};
    std::uint32_t max_mag_atk{0};
    std::uint8_t mag_atk_pct{0};

    float phy_def{0.0f};
    std::uint8_t phy_def_pct{0};

    float mag_def{0.0f};
    std::uint8_t mag_def_pct{0};

    float min_phy_reinforce{0.0f};
    float max_phy_reinforce{0.0f};
    std::uint8_t phy_reinforce_pct{0};

    float min_mag_reinforce{0.0f};
    float max_mag_reinforce{0.0f};
    std::uint8_t mag_reinforce_pct{0};

    std::uint32_t hit_rate{0};
    std::uint8_t hit_pct{0};

    std::uint32_t parry_rate{0};
    std::uint8_t parry_pct{0};

    std::uint32_t critical{0};
    std::uint8_t critical_pct{0};

    std::uint32_t blocking_rate{0};
    std::uint8_t blocking_pct{0};

    float attack_range{0.0f};

    std::string race_name;           // Localized race restriction (Chinese / European / Arabian)
    std::string gender_name;         // Localized gender restriction (empty when unrestricted)
    std::string armor_position_name; // Localized armor position (head, shoulder, chest, ...)

    // Localized description (sub_74CE80) and requirement lines (sub_766B90)
    std::string description;
    std::vector<std::string> requirement_lines;

    // Avatar specifics (sub_768770 / sub_74A0A0)
    bool is_avatar{false};
    std::string avatar_magic_slots_text; // "Magic option(s): 2Unit"
    std::string avatar_wear_text;        // "Attachment: <wear state>"

    // Devil's Spirit specifics (sub_75E9E0)
    bool is_devil_spirit{false};
    bool is_devil_spirit_active{false}; // True only when awaken period is currently active (> 0 remaining time)
    bool is_expired{false};              // True when time-limited item period has expired
    std::vector<tooltip_section> devil_sections;
    std::string blues_heading;
    std::string awaken_header;
    std::string awaken_text;

    // Blue magic options & sockets
    std::vector<std::string> blues;
    std::vector<std::string> sockets;

    // Consumable & Scroll Effect Lines (sub_769640 / sub_75BA30)
    std::vector<tooltip_line> effect_lines;

    // Pet / Fellow Summon specifics (sub_76A9D0)
    bool is_pet_item{false};
    std::string pet_type_name;    // e.g. "Buff", "Protect", "Encourage"
    std::string pet_name;         // "No name" or custom name
    std::uint32_t pet_level{0};
    std::string pet_condition;    // "Normal", "Dead"
  };

  // 1. Full data from a runtime CSOItem (inventory / equipment slot instance)
  auto extract_tooltip_data(const cso_item* item) -> item_tooltip_data;

  // 1B. Static-template-only data (no runtime instance, e.g. other players' visual equipment)
  auto extract_tooltip_data_from_ref(const cref_obj_item* ref, std::uint8_t opt_level = 0) -> item_tooltip_data;

  // 2. Native localization resolvers (CUIStringManager / CGlobalDataManager)
  auto resolve_native_item_type_name(const cref_obj_item* ref) -> std::string;
  auto resolve_sox_subtitle(const cref_obj_item* ref) -> std::string;
  auto resolve_armor_position_name(std::uint8_t pos) -> std::string;

} // namespace ext_client::sdk::ui

// Canonical class facade for CIFSlotWithHelp
class cif_slot_with_help : public cif_wnd {
public:
  static constexpr std::uint32_t k_vtable_addr       = 0x00FF7A3C;
  static constexpr std::uint32_t k_vtable_textboard  = 0x00FF79F4;
  static constexpr std::size_t   k_class_size        = 0x06F8;

  // Static tooltip extraction methods mirroring the native type dispatcher sub_76C0E0
  static auto extract_tooltip_data(const cso_item* item) -> ext_client::sdk::ui::item_tooltip_data {
    return ext_client::sdk::ui::extract_tooltip_data(item);
  }

  static auto extract_tooltip_data_from_ref(const cref_obj_item* ref, std::uint8_t opt_level = 0) -> ext_client::sdk::ui::item_tooltip_data {
    return ext_client::sdk::ui::extract_tooltip_data_from_ref(ref, opt_level);
  }

  static auto resolve_native_item_type_name(const cref_obj_item* ref) -> std::string {
    return ext_client::sdk::ui::resolve_native_item_type_name(ref);
  }

  static auto resolve_sox_subtitle(const cref_obj_item* ref) -> std::string {
    return ext_client::sdk::ui::resolve_sox_subtitle(ref);
  }

  static auto resolve_armor_position_name(std::uint8_t pos) -> std::string {
    return ext_client::sdk::ui::resolve_armor_position_name(pos);
  }
};

// Compatibility aliases in ext_client::sdk::game
namespace ext_client::sdk::game {
  using ext_client::sdk::ui::tooltip_line;
  using ext_client::sdk::ui::tooltip_section;
  using ext_client::sdk::ui::item_tooltip_data;
  using ext_client::sdk::ui::extract_tooltip_data;
  using ext_client::sdk::ui::extract_tooltip_data_from_ref;
  using ext_client::sdk::ui::resolve_native_item_type_name;
  using ext_client::sdk::ui::resolve_sox_subtitle;
  using ext_client::sdk::ui::resolve_armor_position_name;
  namespace tooltip_argb = ext_client::sdk::ui::tooltip_argb;
} // namespace ext_client::sdk::game
