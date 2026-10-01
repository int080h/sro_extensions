#pragma once

#include <cstdint>

struct cclient_session {
  auto get_connected() -> std::uint32_t;
  auto get_connection_id() -> std::uint32_t;
  auto get_locale_byte() -> std::uint32_t;
  auto get_security_version_a() -> std::uint16_t;
  auto get_security_version_b() -> std::uint16_t;
  auto get_security_token() -> std::uint32_t;
  auto get_security_flag() -> std::uint32_t;

  auto set_connected(std::uint32_t val) -> void;
  auto set_connection_id(std::uint32_t val) -> void;
  auto set_locale_byte(std::uint32_t val) -> void;
  auto set_security_version_a(std::uint16_t val) -> void;
  auto set_security_version_b(std::uint16_t val) -> void;
  auto set_security_token(std::uint32_t val) -> void;
  auto set_security_flag(std::uint32_t val) -> void;
};
