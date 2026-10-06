#pragma once

#include "sdk/game/cref_obj_item.hpp"
#include "utils/memory.hpp"
#include "utils/offsets.hpp"

#include <cstdint>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// CSOItem — Active runtime entity instance for items and equipment
// Constructor: 0x009DA7C0 | Vtable: 0x00FE2A98 | Class Size: 0x1F8 (504 bytes)
// Deserializer: sub_9DA110 | Tooltip builder: sub_5BD400 / sub_768B10
// ---------------------------------------------------------------------------

#pragma pack(push, 1)
struct cso_item_stats {
  std::uint32_t max_phy_atk;          // +0x00 (at CSOItem + 0x0E0)
  std::uint32_t min_phy_atk;          // +0x04 (at CSOItem + 0x0E4)
  std::uint32_t max_mag_atk;          // +0x08 (at CSOItem + 0x0E8)
  std::uint32_t min_mag_atk;          // +0x0C (at CSOItem + 0x0EC)
  float         phy_def;              // +0x10 (at CSOItem + 0x0F0)
  float         mag_def;              // +0x14 (at CSOItem + 0x0F4)
  float         phy_absorb;           // +0x18 (at CSOItem + 0x0F8)
  float         mag_absorb;           // +0x1C (at CSOItem + 0x0FC)
  std::uint32_t pad_20;               // +0x20 (at CSOItem + 0x100)
  float         min_phy_reinforce;    // +0x24 (at CSOItem + 0x104)
  float         max_phy_reinforce;    // +0x28 (at CSOItem + 0x108)
  float         min_mag_reinforce;    // +0x2C (at CSOItem + 0x10C)
  float         max_mag_reinforce;    // +0x30 (at CSOItem + 0x110)
  float         phy_reinforce_single; // +0x34 (at CSOItem + 0x114)
  float         mag_reinforce_single; // +0x38 (at CSOItem + 0x118)
  std::uint32_t blocking_rate_base;   // +0x3C (at CSOItem + 0x11C)
  std::uint32_t max_durability;       // +0x40 (at CSOItem + 0x120)
  std::uint32_t parry_rate_base;      // +0x44 (at CSOItem + 0x124)
  std::uint32_t hit_rate_base;        // +0x48 (at CSOItem + 0x128)
  std::uint32_t critical_base;        // +0x4C (at CSOItem + 0x12C)
  std::uint32_t parry_rate;           // +0x50 (at CSOItem + 0x130)
  std::uint32_t hit_rate;             // +0x54 (at CSOItem + 0x134)
  std::uint32_t critical;             // +0x58 (at CSOItem + 0x138)
  std::uint32_t blocking_rate;        // +0x5C (at CSOItem + 0x13C)
  std::uint8_t  phy_atk_pct;          // +0x60 (at CSOItem + 0x140) (0..31 -> /31.0 * 100%)
  std::uint8_t  mag_atk_pct;          // +0x61 (at CSOItem + 0x141)
  std::uint8_t  blocking_pct;         // +0x62 (at CSOItem + 0x142)
  std::uint8_t  pad_63;               // +0x63 (at CSOItem + 0x143)
  std::uint8_t  phy_def_pct;          // +0x64 (at CSOItem + 0x144)
  std::uint8_t  parry_pct;            // +0x65 (at CSOItem + 0x145)
  std::uint8_t  phy_absorb_pct;       // +0x66 (at CSOItem + 0x146)
  std::uint8_t  hit_pct;              // +0x67 (at CSOItem + 0x147)
  std::uint8_t  critical_pct;         // +0x68 (at CSOItem + 0x148)
  std::uint8_t  mag_def_pct;          // +0x69 (at CSOItem + 0x149)
  std::uint8_t  mag_absorb_pct;       // +0x6A (at CSOItem + 0x14A)
  std::uint8_t  phy_reinforce_pct;    // +0x6B (at CSOItem + 0x14B)
  std::uint8_t  mag_reinforce_pct;    // +0x6C (at CSOItem + 0x14C)
};
#pragma pack(pop)

struct s_item_socket {
  std::uint32_t stone_id{0};
  std::uint32_t param{0};
};

struct s_magic_param {
  std::uint16_t id{0};
  std::uint32_t value{0};
};

class cso_item {
public:
  static constexpr std::size_t k_class_size = 0x1F8; // 504 bytes

  // 1. Validation & Active Slot Flag
  auto is_valid() const -> bool {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return false;
    // +0x28 is active item / presence flag (1 = valid, 0 = empty)
    const auto active_byte = ext_client::off::field_at<std::uint8_t>(this, 0x028);
    if (active_byte == 0) return false;
    return ref_id() != 0;
  }

  // 2. Identification (sub_9D5BF0, sub_9D5C20)
  auto ref_id() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x034);
  }

  auto item_id() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x038);
  }

  // 3. Opt / Plus Level (sub_9D5D20: *(this + 0x8C))
  auto opt_level() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x08C);
  }

  // 4. Advanced Elixir Level at +0x72
  auto adv_elixir_level() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x072);
  }

  // 5. Durability & Quantity (sub_9D5C50, sub_9D5C10)
  auto durability() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x098);
  }

  auto count() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x09C);
  }

  // 6. Variance Seed (sub_9D5D70: 64-bit integer at +0x090)
  auto variance_raw() const -> std::uint64_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint64_t>(this, 0x090);
  }

  // 6B. Devil Spirit awaken state byte at +0x05C (sub_9D5D30): 0 = not awakened, 1 = awakening
  auto awaken_state() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x05C);
  }

  // 6C. Devil Spirit remaining awaken time in milliseconds at +0x060 (sub_9D5D60)
  auto awaken_remaining_ms() const -> std::int64_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::int64_t>(this, 0x060);
  }

  // 6D. Pet / COS properties (sub_9DA110 / sub_76A9D0)
  auto pet_level() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x1E8);
  }

  auto pet_condition() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x05C);
  }

  auto cos_id() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x038);
  }

  // 7. Sockets (sub_9D69F0: up to 3 sockets at +0x74 + i*8, bounded by +0x8C)
  auto get_socket(std::size_t index) const -> s_item_socket {
    s_item_socket s{};
    if (index >= 3 || !ext_client::utils::memory::is_valid_ptr(this)) return s;
    const auto offset = 0x074 + (index * 8);
    s.stone_id = ext_client::off::field_at<std::uint32_t>(this, offset);
    s.param    = ext_client::off::field_at<std::uint32_t>(this, offset + 4);
    return s;
  }

  auto sockets() const -> std::vector<s_item_socket> {
    std::vector<s_item_socket> res;
    for (std::size_t i = 0; i < 3; ++i) {
      auto s = get_socket(i);
      if (s.stone_id != 0 && s.stone_id != 0xFFFFFFFF) {
        res.push_back(s);
      }
    }
    return res;
  }

  // 8. Computed Item Stats Structure (sub_9D5C60: this + 0x0E0)
  auto stats() const -> const cso_item_stats* {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return nullptr;
    const auto* s = &ext_client::off::field_at<cso_item_stats>(this, 0x0E0);
    return ext_client::utils::memory::is_valid_ptr(s) ? s : nullptr;
  }

  // 9. Base Template (CRefObjItem via sub_9D5BF0 / sub_A93E20)
  auto get_ref_item() const -> cref_obj_item* {
    return cref_obj_item::get(ref_id());
  }

  // 10. Magic Parameters / Blues (sub_9D5DA0: std::map at +0xC4, sub_75C2D0)
  auto magic_param_count() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x0A4);
  }

  auto magic_params() const -> std::vector<s_magic_param> {
    std::vector<s_magic_param> result;
    if (!is_valid()) return result;

    // Map layout at this + 0xC4:
    // +0xC8: _Myhead pointer
    // +0xCC: _Mysize count
    const auto head_ptr = ext_client::off::field_at<std::uintptr_t>(this, 0x0C8);
    const auto size     = ext_client::off::field_at<std::uint32_t>(this, 0x0CC);

    if (!ext_client::utils::memory::is_valid_ptr(reinterpret_cast<const void*>(head_ptr)) || size == 0 || size > 32) {
      return result;
    }

    struct map_node {
      map_node*     left;     // +0x00
      map_node*     parent;   // +0x04
      map_node*     right;    // +0x08
      std::uint16_t key;      // +0x0C (Magic option ID)
      std::uint16_t pad;      // +0x0E
      std::uint32_t value;    // +0x10 (Option parameter)
      std::uint8_t  color;    // +0x14
      std::uint8_t  isnil;    // +0x15
      std::uint8_t  pad2[2];  // +0x16
    };

    const auto* head = reinterpret_cast<const map_node*>(head_ptr);
    const auto* root = head->parent;
    if (!ext_client::utils::memory::is_valid_ptr(root) || root->isnil != 0) {
      return result;
    }

    // Safe recursive in-order collection with depth guard and pointer validation
    auto traverse = [&](auto& self, const map_node* node, int depth) -> void {
      if (!node || depth > 16 || !ext_client::utils::memory::is_valid_ptr(node) || node->isnil != 0) {
        return;
      }
      if (node->left && ext_client::utils::memory::is_valid_ptr(node->left) && node->left->isnil == 0) {
        self(self, node->left, depth + 1);
      }
      result.push_back({node->key, node->value});
      if (node->right && ext_client::utils::memory::is_valid_ptr(node->right) && node->right->isnil == 0) {
        self(self, node->right, depth + 1);
      }
    };

    traverse(traverse, root, 0);
    return result;
  }
};
