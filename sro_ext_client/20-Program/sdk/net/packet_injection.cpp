#include "pch.hpp"
#include "sdk/net/packet_injection.hpp"

#include "sdk/net/cclient_net.hpp"
#include "sdk/net/cmsg.hpp"
#include "sdk/net/msg_define.hpp"
#include "sdk/render/cg_interface.hpp"
#include "utils/memory.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"
#include "utils/string.hpp"

#include <vector>

namespace ext_client::net::injection {

namespace {

  class byte_writer {
  public:
    auto write_u8(std::uint8_t v) -> void {
      bytes_.push_back(v);
    }

    auto write_u16(std::uint16_t v) -> void {
      const auto* p = reinterpret_cast<const std::uint8_t*>(&v);
      bytes_.insert(bytes_.end(), p, p + sizeof(v));
    }

    auto write_u32(std::uint32_t v) -> void {
      const auto* p = reinterpret_cast<const std::uint8_t*>(&v);
      bytes_.insert(bytes_.end(), p, p + sizeof(v));
    }

    auto write_raw(const void* data, std::size_t len) -> void {
      if (data && len > 0) {
        const auto* p = static_cast<const std::uint8_t*>(data);
        bytes_.insert(bytes_.end(), p, p + len);
      }
    }

    [[nodiscard]] auto data() const -> const std::uint8_t* { return bytes_.data(); }
    [[nodiscard]] auto size() const -> std::size_t { return bytes_.size(); }
    [[nodiscard]] auto bytes() const -> const std::vector<std::uint8_t>& { return bytes_; }

  private:
    std::vector<std::uint8_t> bytes_{};
  };

} // namespace

// ===========================================================================
// 1. Core Packet Injection
// ===========================================================================
auto send_packet(std::uint16_t opcode, const void* data, std::size_t size) -> bool {
  if (!cclient_net::is_connected()) {
    return false;
  }

  auto* msg = cclient_net::alloc_msg(opcode, 0);
  if (!msg) {
    return false;
  }

  if (data && size > 0) {
    msg->write_payload(data, static_cast<int>(size));
  }

  // send_msg transmits and automatically frees the cmsg
  const auto ret = cclient_net::send_msg(msg);
  return ret == 0;
}

auto send_packet(std::uint16_t opcode, const std::vector<std::uint8_t>& payload) -> bool {
  return send_packet(opcode, payload.data(), payload.size());
}

auto send_packet(std::uint16_t opcode) -> bool {
  return send_packet(opcode, nullptr, 0);
}

auto send_raw(cmsg* msg) -> bool {
  if (!msg || !cclient_net::is_connected()) {
    return false;
  }
  return cclient_net::send_msg(msg) == 0;
}

// ===========================================================================
// 2. High-Level Game Action Injections
// ===========================================================================
auto send_chat(std::uint8_t chat_type, std::string_view message, std::string_view recipient) -> bool {
  // sub_87A220 writes the text into CGInterface+0x66C under the live chat index, then sends 0x7025.
  // The local chat line is that map entry. A hand-built packet never inserts it, so the line is empty.
  auto* iface = cg_interface::get();
  if (!iface || !ext_client::utils::memory::is_readable_ptr(iface)) {
    return false;
  }

  using chat_send_fn = void(__thiscall*)(void* self, std::uint8_t type, void* message, void* recipient);
  const auto send = ext_client::off::as_fn<chat_send_fn>(0x0087A220);
  if (!send) {
    return false;
  }

  const auto wide = ext_client::utils::string::to_wide(message);
  ext_client::msvc9::wstring game_message(wide.c_str());
  if (chat_type == 2 && !recipient.empty()) {
    const auto wide_to = ext_client::utils::string::to_wide(recipient);
    ext_client::msvc9::wstring game_recipient(wide_to.c_str());
    send(iface, chat_type, game_message.raw(), game_recipient.raw());
  } else {
    send(iface, chat_type, game_message.raw(), nullptr);
  }
  return true;
}

auto send_select_target(std::uint32_t target_unique_id) -> bool {
  byte_writer w;
  w.write_u32(target_unique_id);
  return send_packet(0x7045, w.data(), w.size());
}

auto send_use_inventory_item(std::uint8_t slot, std::uint16_t usage_type, std::uint32_t target_id) -> bool {
  byte_writer w;
  w.write_u8(slot);
  w.write_u16(usage_type);
  w.write_u32(target_id);
  return send_packet(0x704C, w.data(), w.size());
}

auto send_action_emote(std::uint8_t emote_id) -> bool {
  byte_writer w;
  w.write_u8(emote_id);
  return send_packet(0x7074, w.data(), w.size());
}

auto send_cast_skill(std::uint32_t skill_id, std::uint32_t target_id) -> bool {
  byte_writer w;
  w.write_u8(1);            // Action type = 1 (skill / attack)
  w.write_u8(4);            // Action subtype = 4 (skill cast)
  w.write_u32(skill_id);    // Target skill ID
  if (target_id > 0) {
    w.write_u8(1);          // Has target = 1
    w.write_u32(target_id); // Target entity unique ID
  } else {
    w.write_u8(0);          // Untargeted / self-cast = 0
  }
  return send_packet(0x7074, w.data(), w.size());
}

auto send_exit_request() -> bool {
  byte_writer w;
  w.write_u8(1);
  return send_packet(0x7005, w.data(), w.size());
}

} // namespace ext_client::net::injection
