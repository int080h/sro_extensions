#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

class cmsg;

namespace ext_client::net::injection {

  // =========================================================================
  // 1. Core Packet Injection
  // Allocates a CMsg from pool, writes wire payload, and transmits to server.
  // =========================================================================
  auto send_packet(std::uint16_t opcode, const void* data, std::size_t size) -> bool;
  auto send_packet(std::uint16_t opcode, const std::vector<std::uint8_t>& payload) -> bool;
  auto send_packet(std::uint16_t opcode) -> bool;
  auto send_raw(cmsg* msg) -> bool;

  // =========================================================================
  // 2. High-Level Game Action Injections
  // Ready-to-use helpers for standard client actions
  // =========================================================================
  // 0x7025: Chat message (type: 1=General, 2=PM/Whisper, 3=Party, 4=Guild, etc.)
  // Goes through the client's own sender so the outgoing text is stored for the
  // local echo. A raw packet is accepted by the server but the client line stays blank.
  auto send_chat(std::uint8_t chat_type, std::string_view message, std::string_view recipient = {}) -> bool;

  // 0x7045: Target/Entity selection
  auto send_select_target(std::uint32_t target_unique_id) -> bool;

  // 0x704C: Use inventory item
  auto send_use_inventory_item(std::uint8_t slot, std::uint16_t usage_type = 0, std::uint32_t target_id = 0) -> bool;

  // 0x7074: Action emote
  auto send_action_emote(std::uint8_t emote_id) -> bool;

  // 0x7074: Cast skill packet
  auto send_cast_skill(std::uint32_t skill_id, std::uint32_t target_id = 0) -> bool;

  // 0x7005: Client exit request
  auto send_exit_request() -> bool;

} // namespace ext_client::net::injection
