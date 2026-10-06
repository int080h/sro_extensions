#pragma once

#include "plugins/net_log/net_log_types.hpp"
#include "core/event_bus.hpp"
#include "render/menu_builder.hpp"

#include <cstdio>
#include <mutex>
#include <vector>

namespace ext_client::plugins::net_log {

extern std::mutex g_override_mutex;
extern override_rule g_outgoing_override;
extern override_rule g_incoming_override;

extern std::mutex g_log_mutex;
extern log_entry g_ring[k_log_ring_capacity];
extern std::size_t g_ring_head;
extern std::size_t g_ring_count;
extern std::uint32_t g_next_log_id;

extern FILE* g_log_file;
extern char g_active_log_path[260];
extern std::size_t g_unflushed_writes;

auto should_capture(ext_client::packet_direction direction,
                    ext_client::core::event::packet_layer layer) -> bool;
auto push_entry(ext_client::packet_direction direction,
                ext_client::core::event::packet_layer layer,
                std::uint16_t opcode,
                std::vector<std::uint8_t> payload,
                const ext_client::core::event::packet_context& ctx,
                bool blocked,
                bool modified,
                const char* capture_point) -> void;
auto record_packet(ext_client::core::event::packet_context& ctx, bool blocked, bool modified) -> void;
auto capture_packet(ext_client::core::event::packet_context& ctx, bool blocked, bool modified) -> void;
auto clear_log() -> void;
auto close_log_file() -> void;
auto flush_log_file() -> void;
auto copy_log_entries(std::vector<log_entry>& out) -> void;
auto refresh_log_entries(std::vector<log_entry>& out, std::uint64_t& revision, std::uint64_t& epoch) -> bool;
auto flush_pending_file() -> void;
auto handle_shutdown() -> void;
auto replace_log_entries(std::vector<log_entry> entries) -> void;

auto format_opcode(std::uint16_t opcode) -> const char*;
auto opcode_display_name(std::uint16_t opcode) -> const char*;
auto format_layer(ext_client::core::event::packet_layer layer) -> const char*;
auto opcode_in_list(std::uint16_t opcode, const char* list_text) -> bool;
auto should_block_opcode(std::uint16_t opcode) -> bool;
auto parse_hex(const char* text, std::vector<std::uint8_t>& out) -> bool;
auto parse_opcode_list(const char* text, std::vector<std::uint16_t>& out) -> bool;

auto handle_packet(ext_client::core::event::packet_context& ctx) -> void;
auto handle_menu(ext_client::render::menu::menu_builder& ui) -> void;
} // namespace ext_client::plugins::net_log
