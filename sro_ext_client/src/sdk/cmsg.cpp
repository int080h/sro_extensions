#include "sdk/cmsg.hpp"

#include "utils/offsets.hpp"

namespace {

  using write_payload_fn = int(__thiscall*)(void* self, void* src, int size);
  using send_from_buffer_fn = void(__cdecl*)(cmsg_stream_buffer* buffer);

  using ext_client::offsets::as_fn;

  // Binary internal wire CMsg field offsets (verified from IDA).
  // These are used when cmsg* points to the game's pool-allocated CMsg (0x1068+ bytes).
  namespace wire {
    inline constexpr std::size_t data_ptr      = 0x1034;
    inline constexpr std::size_t read_cursor   = 0x103C;
    inline constexpr std::size_t write_cursor  = 0x103E;
    inline constexpr std::size_t opcode_ptr    = 0x1050;
    inline constexpr std::size_t size_ptr      = 0x1054;
  }

} // namespace

// =========================================================================
// Building mode
// =========================================================================

cmsg::cmsg(std::uint16_t opcode) {
  cmsg_stream_buffer::construct(&m_stream, opcode);
  m_stream.toggle_before();
}

cmsg::~cmsg() {
  m_stream.toggle_after();
  m_stream.destroy();
}

auto cmsg::operator<<(std::uint8_t v) -> cmsg& {
  m_stream.write_u8(v);
  return *this;
}

auto cmsg::operator<<(std::uint16_t v) -> cmsg& {
  m_stream.write_u16(v);
  return *this;
}

auto cmsg::operator<<(std::uint32_t v) -> cmsg& {
  m_stream.write_u32(v);
  return *this;
}

auto cmsg::operator<<(const std::string& v) -> cmsg& {
  m_stream.write_string(v);
  return *this;
}

auto cmsg::operator<<(const char* v) -> cmsg& {
  m_stream.write_string(std::string(v));
  return *this;
}

auto cmsg::send() -> void {
  send_from_buffer(&m_stream);
}

// =========================================================================
// Static API
// =========================================================================

auto cmsg::send_from_buffer(cmsg_stream_buffer* buf) -> void {
  if (!buf) {
    return;
  }
  as_fn<send_from_buffer_fn>(ext_client::offsets::cmsg::functions::send_from_buffer)(buf);
}

// =========================================================================
// Reading mode — raw offset arithmetic against binary's internal wire CMsg
// =========================================================================

auto cmsg::opcode() const -> std::uint16_t {
  const auto* raw = reinterpret_cast<const std::uint8_t*>(this);
  const auto* ptr = *reinterpret_cast<const std::uint16_t* const*>(raw + wire::opcode_ptr);
  return ptr ? *ptr : 0;
}

auto cmsg::header_opcode() const -> std::uint16_t {
  const auto* raw = reinterpret_cast<const std::uint8_t*>(this);
  const auto* data = *reinterpret_cast<void* const*>(raw + wire::data_ptr);
  if (!data) {
    return 0;
  }
  return *reinterpret_cast<const std::uint16_t*>(static_cast<const std::uint8_t*>(data) + 2);
}

auto cmsg::body_size() const -> std::size_t {
  const auto* raw = reinterpret_cast<const std::uint8_t*>(this);
  const auto* ptr = *reinterpret_cast<const std::uint16_t* const*>(raw + wire::size_ptr);
  return ptr ? static_cast<std::size_t>(*ptr & 0x7FFFu) : 0;
}

auto cmsg::is_encrypted() const -> bool {
  const auto* raw = reinterpret_cast<const std::uint8_t*>(this);
  const auto* ptr = *reinterpret_cast<const std::uint16_t* const*>(raw + wire::size_ptr);
  return ptr && (*ptr & 0x8000u) != 0;
}

auto cmsg::read_cursor_pos() const -> std::size_t {
  const auto* raw = reinterpret_cast<const std::uint8_t*>(this);
  return *reinterpret_cast<const std::uint16_t*>(raw + wire::read_cursor);
}

auto cmsg::extract_payload() const -> std::vector<std::uint8_t> {
  const auto* raw = reinterpret_cast<const std::uint8_t*>(this);
  const auto* data = *reinterpret_cast<void* const*>(raw + wire::data_ptr);
  if (!data) {
    return {};
  }
  const auto size = body_size();
  if (size == 0) {
    return {};
  }
  const auto* bytes = static_cast<const std::uint8_t*>(data);
  return std::vector<std::uint8_t>(bytes + 6, bytes + 6 + size);
}

auto cmsg::write_payload(const void* src, int size) -> int {
  if (!src || size <= 0) {
    return 0;
  }
  return as_fn<write_payload_fn>(ext_client::offsets::cmsg::functions::write_payload)(this, const_cast<void*>(src), size);
}
