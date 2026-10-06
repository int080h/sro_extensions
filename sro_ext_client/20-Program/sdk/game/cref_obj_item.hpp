#pragma once

#include "utils/memory.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"
#include "utils/string.hpp"
#include "sdk/game/cglobal_data_manager.hpp"
#include "sdk/ui/cui_string_manager.hpp"

#include <cstdint>
#include <string>

// ---------------------------------------------------------------------------
// CRefObjItem — Static base item template loaded from server_dep/refitem.txt
// Lookup: sub_A93E20(ref_item_id) returns CRefObjItem* (via CGlobalDataManager)
// Class Size: >= 0x2B8 (700+ bytes)
// ---------------------------------------------------------------------------
class cref_obj_item {
public:
  // Static lookup using engine's native CGlobalDataManager::GetRefItem (sub_A93E20)
  static auto get(std::uint32_t ref_id) -> cref_obj_item* {
    auto* ptr = reinterpret_cast<cref_obj_item*>(cglobal_data_manager::get_ref_item(ref_id));
    return ext_client::utils::memory::is_game_ptr(ptr) ? ptr : nullptr;
  }

  // 1. Core Identification & Type Mask
  // Supports both 16-bit bitfield format (tid_t) and 4-byte packed format
  auto type_id_raw() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return *reinterpret_cast<const std::uint32_t*>(this);
  }

  auto type_id() const -> std::uint32_t {
    return type_id_raw();
  }

  auto type_id1() const -> std::uint8_t {
    const auto raw = type_id_raw();
    const auto b0 = static_cast<std::uint8_t>(raw & 0xFF);
    if (b0 == 3 || b0 == 1 || b0 == 2) {
      return b0;
    }
    return static_cast<std::uint8_t>((raw >> 2) & 0x07);
  }

  auto type_id2() const -> std::uint8_t {
    const auto raw = type_id_raw();
    const auto b0 = static_cast<std::uint8_t>(raw & 0xFF);
    if (b0 == 3 || b0 == 1 || b0 == 2) {
      return static_cast<std::uint8_t>((raw >> 8) & 0xFF);
    }
    return static_cast<std::uint8_t>((raw >> 5) & 0x03);
  }

  auto type_id3() const -> std::uint8_t {
    const auto raw = type_id_raw();
    const auto b0 = static_cast<std::uint8_t>(raw & 0xFF);
    if (b0 == 3 || b0 == 1 || b0 == 2) {
      return static_cast<std::uint8_t>((raw >> 16) & 0xFF);
    }
    return static_cast<std::uint8_t>((raw >> 7) & 0x0F);
  }

  auto type_id4() const -> std::uint8_t {
    const auto raw = type_id_raw();
    const auto b0 = static_cast<std::uint8_t>(raw & 0xFF);
    if (b0 == 3 || b0 == 1 || b0 == 2) {
      return static_cast<std::uint8_t>((raw >> 24) & 0xFF);
    }
    return static_cast<std::uint8_t>((raw >> 11) & 0x1F);
  }

  auto ref_id() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x004);
  }

  // 2. Item CodeName (std::wstring at this + 0x008, accessed natively at sub_769734)
  auto code_name() const -> std::wstring {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return L"";
    const auto* str_obj = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(this) + 0x008);
    const auto ref = ext_client::msvc9::wstring_ref::from(str_obj);
    const auto* d = ref.data();
    if (d && *d != L'\0') {
      return std::wstring(d, ref.length());
    }
    return L"";
  }

  // 2B. Item Name Key (std::wstring at this + 0x060, accessed natively at sub_75E36D)
  auto name_key() const -> std::wstring {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return L"";
    const auto* str_obj = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(this) + 0x060);
    const auto ref = ext_client::msvc9::wstring_ref::from(str_obj);
    const auto* d = ref.data();
    if (d && *d != L'\0') {
      return std::wstring(d, ref.length());
    }
    return L"";
  }

  // 2C. Localized Display Name dynamically resolved from game textdata (sub_75DFE0)
  auto name() const -> std::string {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return "";
    const auto k = name_key();
    if (!k.empty()) {
      const auto loc = ext_client::sdk::ui::get_string(k.c_str());
      if (!loc.empty()) {
        return ext_client::utils::string::to_utf8(loc.c_str());
      }
    }
    const auto code = code_name();
    if (!code.empty()) {
      const std::wstring sn = L"SN_" + code;
      const auto loc = ext_client::sdk::ui::get_string(sn.c_str());
      if (!loc.empty()) {
        return ext_client::utils::string::to_utf8(loc.c_str());
      }
      return ext_client::utils::string::to_utf8(code.c_str());
    }
    return "";
  }

  // 2D. Description key (std::wstring at this + 0x07C, translated by sub_74CE80 via CUIStringManager)
  auto description_key() const -> std::wstring {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return L"";
    const auto* str_obj = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(this) + 0x07C);
    const auto ref = ext_client::msvc9::wstring_ref::from(str_obj);
    const auto* d = ref.data();
    if (d && *d != L'\0') {
      return std::wstring(d, ref.length());
    }
    return L"";
  }

  // 2E. Gender restriction dword at +0x1C4 (sub_766B90): 0=Female, 1=Male, 2=Any, 3/4/5=Pet types
  auto gender_code() const -> std::int32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 2;
    return ext_client::off::field_at<std::int32_t>(this, 0x1C4);
  }

  // 2F. Race restriction byte at +0x09C (sub_766B90): 0=Chinese, 1=European, 2=Arabian, 3=Any
  auto race_code() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 3;
    return ext_client::off::field_at<std::uint8_t>(this, 0x09C);
  }

  // 2G. Requirement table (sub_766B90): 4 entries, type dword at +0x0D0 + i*4 (-1 = unused),
  //     value dword at +0x0E0 + i*4. Type 1=Level, 2=Job grade, 3/4=Conflict job level,
  //     10=Guild level, 257.. / 513.. = Mastery levels.
  auto requirement_type(std::size_t index) const -> std::int32_t {
    if (index >= 4 || !ext_client::utils::memory::is_valid_ptr(this)) return -1;
    return ext_client::off::field_at<std::int32_t>(this, 0x0D0 + index * 4);
  }

  auto requirement_value(std::size_t index) const -> std::int32_t {
    if (index >= 4 || !ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::int32_t>(this, 0x0E0 + index * 4);
  }

  // 2H. Required STR / INT dwords at +0x1C8 / +0x1CC (sub_766B90)
  auto required_str() const -> std::int32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::int32_t>(this, 0x1C8);
  }

  auto required_int() const -> std::int32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::int32_t>(this, 0x1CC);
  }

  // 2I. Avatar: max magic option slot count byte at +0x534 (sub_74A0A0)
  auto avatar_magic_slot_count() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x534);
  }

  // 2J. Avatar attachment: wear flag byte at +0x535 (sub_768770: UIIT_STT_AVATAR_WEAR / _NOT_WEAR)
  auto avatar_attach_wear_flag() const -> bool {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return false;
    return ext_client::off::field_at<std::uint8_t>(this, 0x535) != 0;
  }

  // 2K. Item parameters: Param1..Param6 (offsets +0x2B4, +0x2B8, +0x2BC, +0x2C0, +0x2C4, +0x2C8)
  // Consumables: HP heal amount, HP heal %, MP heal amount, MP heal %, cure levels (sub_74B950)
  // Devil spirit: Param1 is awaken period in seconds (sub_75E9E0, -1 = not awakened)
  auto param1() const -> std::int32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::int32_t>(this, 0x2B4);
  }

  auto param2() const -> std::int32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::int32_t>(this, 0x2B8);
  }

  auto param3() const -> std::int32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::int32_t>(this, 0x2BC);
  }

  auto param4() const -> std::int32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::int32_t>(this, 0x2C0);
  }

  auto param5() const -> std::int32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::int32_t>(this, 0x2C4);
  }

  auto param6() const -> std::int32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::int32_t>(this, 0x2C8);
  }

  auto awaken_period_seconds() const -> std::int32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return -1;
    return param1();
  }

  // 2L. Native TypeID decoding. The engine stores the id as: bits[2:3] object group,
  //     bits[4:9] tid1, bits[10:15] tid2, byte 2 = tid3, byte 3 = tid4 (see sub_581D20 / sub_76C0E0).
  //     Byte 2 / byte 3 are also valid for the packed layout used by the unit tests.
  auto native_tid3() const -> std::uint8_t {
    return static_cast<std::uint8_t>((type_id_raw() >> 16) & 0xFF);
  }

  auto native_tid4() const -> std::uint8_t {
    return static_cast<std::uint8_t>((type_id_raw() >> 24) & 0xFF);
  }

  auto is_equipment_family() const -> bool {
    const auto raw = type_id_raw();
    const bool engine = ((raw >> 2) & 3) != 1 && ((raw >> 4) & 0x3F) == 3 && ((raw >> 10) & 0x3F) == 1;
    const bool packed = (raw & 0xFF) == 3 && ((raw >> 8) & 0xFF) == 1;
    return engine || packed;
  }

  // COS / Pet family (sub_49FB00): TypeID1=3, TypeID2=2
  auto is_cos_family() const -> bool {
    const auto raw = type_id_raw();
    const bool engine = ((raw >> 2) & 3) != 1 && ((raw >> 4) & 0x3F) == 3 && ((raw >> 10) & 0x3F) == 2;
    const bool packed = (raw & 0xFF) == 3 && ((raw >> 8) & 0xFF) == 2;
    return engine || packed;
  }

  // Pet / Fellow summon scroll (sub_49FB30): TypeID1=3, TypeID2=2, TypeID3=1
  auto is_cos_pet() const -> bool {
    return is_cos_family() && native_tid3() == 1;
  }

  // Consumable / Expendables family (sub_499160): TypeID1=3, TypeID2=3
  auto is_consumable_family() const -> bool {
    const auto raw = type_id_raw();
    const bool engine = ((raw >> 2) & 3) != 1 && ((raw >> 4) & 0x3F) == 3 && ((raw >> 10) & 0x3F) == 3;
    const bool packed = (raw & 0xFF) == 3 && ((raw >> 8) & 0xFF) == 3;
    return engine || packed;
  }

  // Devil's Spirit (tid3 == 14, tooltip builder sub_75E9E0)
  auto is_devil_spirit() const -> bool {
    return is_equipment_family() && native_tid3() == 14;
  }

  // 3. Armor position byte at +0x03
  // 1=Head, 2=Shoulder, 3=Chest, 4=Legs, 5=Hands, 6=Feet
  auto armor_position() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x003);
  }

  // 4. Rarity & SoX classification (sub_9D5DB0 at +0xA0: 0=normal, 2=SoX/rare, 6=set/legend)
  auto rarity() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x0A0);
  }

  auto is_sox() const -> bool {
    const auto r = rarity();
    return r == 2 || r == 6;
  }

  // 4B. Item Icon DDJ path (MSVC9 std::string at this + 0x15C, sub_76C350 / sub_ABF7F0)
  auto icon_path() const -> std::string {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return "";
    const auto* str_obj = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(this) + 0x15C);
    const auto ref = ext_client::msvc9::string_ref::from(str_obj);
    const auto* d = ref.data();
    if (d && *d != '\0') {
      std::string path(d, ref.length());
      if (path != "icon\\xxx" && path != "xxx" && path.find(".ddj") != std::string::npos) {
        return path;
      }
    }
    // Fallback to +0x178 if +0x15C was empty/xxx
    const auto* str_obj2 = reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(this) + 0x178);
    const auto ref2 = ext_client::msvc9::string_ref::from(str_obj2);
    const auto* d2 = ref2.data();
    if (d2 && *d2 != '\0') {
      std::string path2(d2, ref2.length());
      if (path2 != "xxx" && path2.find(".ddj") != std::string::npos) {
        if (path2.rfind("icon\\", 0) != 0) {
          path2 = "icon\\" + path2;
        }
        return path2;
      }
    }
    return "";
  }

  // SoX tier index (0..2) derived from the class byte. The visible title is NOT stored here: it is
  // looked up from the game's translation table (see resolve_sox_subtitle in cif_slot_with_help.cpp).
  auto sox_tier() const -> std::uint8_t {
    const auto cb = class_byte();
    return (cb > 0) ? static_cast<std::uint8_t>((cb - 1) % 3) : 0;
  }

  // 5. Degree & Class byte at +0x1D0 (Degree = (Class - 1) / 3 + 1)
  auto class_byte() const -> std::uint8_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint8_t>(this, 0x1D0);
  }

  auto degree() const -> std::uint8_t {
    const auto cb = class_byte();
    return (cb > 0) ? static_cast<std::uint8_t>((cb - 1) / 3 + 1) : 0;
  }

  // 6. Required Level at +0x1D4
  auto req_level() const -> std::uint32_t {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0;
    return ext_client::off::field_at<std::uint32_t>(this, 0x1D4);
  }

  // 7. Attack Range at +0x254 (val / 10.0f meters)
  auto attack_range() const -> float {
    if (!ext_client::utils::memory::is_valid_ptr(this)) return 0.0f;
    const auto raw = ext_client::off::field_at<std::int32_t>(this, 0x254);
    return raw > 0 ? (static_cast<float>(raw) / 10.0f) : 0.0f;
  }

  // 8. Item classification. Purely TypeID driven, exactly like the client's tooltip dispatcher
  //    (sub_76C0E0): tid3 selects the builder, tid4 the sub-type. Item names / type labels are never
  //    guessed from the code name; they come from the game's own tables and translations.
  //    Verified against the client's refitem data (itemdata_*.txt, TypeID1=3 / TypeID2=1):
  //      tid3  1,2,3    Chinese garment / protector / heavy armor
  //            9,10,11  European garment / protector / heavy armor
  //            4        shield (tid4 1 = Chinese, 2 = European)
  //            5 / 12   Chinese / European accessory (tid4 1 earring, 2 necklace, 3 ring)
  //            6        weapon
  //            7        job suit (trader / thief / hunter)
  //            13       avatar (tid4 1 dress, 2 attachment, 3 hat, 4 flag)
  //            14       devil's spirit
  auto is_equipment() const -> bool {
    return is_equipment_family();
  }

  auto is_weapon() const -> bool {
    return is_equipment() && native_tid3() == 6;
  }

  auto is_shield() const -> bool {
    return is_equipment() && native_tid3() == 4;
  }

  auto is_armor() const -> bool {
    const auto t3 = native_tid3();
    return is_equipment() && ((t3 >= 1 && t3 <= 3) || (t3 >= 9 && t3 <= 11));
  }

  auto is_accessory() const -> bool {
    const auto t3 = native_tid3();
    return is_equipment() && (t3 == 5 || t3 == 12);
  }

  // tid3 7: job suit (trader / thief / hunter). The client uses a reduced tooltip for it (sub_768550).
  auto is_job_suit() const -> bool {
    return is_equipment() && native_tid3() == 7;
  }

  auto is_avatar() const -> bool {
    return is_equipment() && native_tid3() == 13;
  }

  // Race restriction straight from the template (+0x9C): 0 = Chinese, 1 = European
  auto is_european() const -> bool { return race_code() == 1; }
  auto is_chinese() const -> bool { return race_code() == 0; }
};

