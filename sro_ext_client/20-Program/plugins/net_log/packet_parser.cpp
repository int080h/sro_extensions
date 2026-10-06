#include "pch.hpp"
#include "plugins/net_log/packet_parser.hpp"
#include "plugins/net_log/packet_field_defs.hpp"
#include "plugins/net_log/packet_doc_db.hpp"
#include "utils/string.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace ext_client::plugins::net_log {

namespace {

class cursor {
public:
  cursor(const std::uint8_t* data, std::size_t size) : data_(data), size_(size), pos_(0) {}

  auto remaining() const -> std::size_t { return pos_ <= size_ ? size_ - pos_ : 0; }
  auto position() const -> std::size_t { return pos_; }
  auto set_position(std::size_t pos) -> void { pos_ = std::min(pos, size_); }

  auto read_u8() -> std::optional<std::uint8_t> {
    if (pos_ + 1 > size_) return std::nullopt;
    return data_[pos_++];
  }

  auto read_u16() -> std::optional<std::uint16_t> {
    if (pos_ + 2 > size_) return std::nullopt;
    std::uint16_t v;
    std::memcpy(&v, data_ + pos_, 2);
    pos_ += 2;
    return v;
  }

  auto read_u32() -> std::optional<std::uint32_t> {
    if (pos_ + 4 > size_) return std::nullopt;
    std::uint32_t v;
    std::memcpy(&v, data_ + pos_, 4);
    pos_ += 4;
    return v;
  }

  auto read_u64() -> std::optional<std::uint64_t> {
    if (pos_ + 8 > size_) return std::nullopt;
    std::uint64_t v;
    std::memcpy(&v, data_ + pos_, 8);
    pos_ += 8;
    return v;
  }

  auto read_i8() -> std::optional<std::int8_t> {
    auto v = read_u8();
    if (!v) return std::nullopt;
    return static_cast<std::int8_t>(*v);
  }

  auto read_i16() -> std::optional<std::int16_t> {
    auto v = read_u16();
    if (!v) return std::nullopt;
    return static_cast<std::int16_t>(*v);
  }

  auto read_i32() -> std::optional<std::int32_t> {
    auto v = read_u32();
    if (!v) return std::nullopt;
    return static_cast<std::int32_t>(*v);
  }

  auto read_i64() -> std::optional<std::int64_t> {
    auto v = read_u64();
    if (!v) return std::nullopt;
    return static_cast<std::int64_t>(*v);
  }

  auto read_f32() -> std::optional<float> {
    if (pos_ + 4 > size_) return std::nullopt;
    float v;
    std::memcpy(&v, data_ + pos_, 4);
    pos_ += 4;
    return v;
  }

  auto read_bool() -> std::optional<bool> {
    auto v = read_u8();
    if (!v) return std::nullopt;
    return *v != 0;
  }

  auto read_ascii() -> std::optional<std::string> {
    const auto save_pos = pos_;
    auto len = read_u16();
    if (!len) return std::nullopt;
    if (pos_ + *len > size_) {
      pos_ = save_pos;
      return std::nullopt;
    }
    std::string s(reinterpret_cast<const char*>(data_ + pos_), *len);
    pos_ += *len;
    while (!s.empty() && s.back() == '\0') {
      s.pop_back();
    }
    return s;
  }

  // UTF-16LE string: uint16 character count, then that many code units, no null.
  auto read_utf16() -> std::optional<std::string> {
    const auto save_pos = pos_;
    auto count = read_u16();
    if (!count) return std::nullopt;
    const auto bytes = static_cast<std::size_t>(*count) * 2;
    if (pos_ + bytes > size_) {
      pos_ = save_pos;
      return std::nullopt;
    }
    std::wstring wide(*count, L'\0');
    if (bytes > 0) {
      std::memcpy(wide.data(), data_ + pos_, bytes);
    }
    pos_ += bytes;
    return ext_client::utils::string::to_utf8(wide);
  }

  auto read_fixed_ascii(std::size_t len) -> std::optional<std::string> {
    if (pos_ + len > size_) return std::nullopt;
    std::string s(reinterpret_cast<const char*>(data_ + pos_), len);
    pos_ += len;
    while (!s.empty() && s.back() == '\0') {
      s.pop_back();
    }
    return s;
  }

  auto read_hex_bytes(std::size_t len) -> std::optional<std::string> {
    if (pos_ + len > size_) return std::nullopt;
    std::string out;
    out.reserve(len * 3);
    for (std::size_t i = 0; i < len; ++i) {
      char buf[4];
      std::snprintf(buf, sizeof(buf), "%02X ", data_[pos_ + i]);
      out.append(buf);
    }
    if (!out.empty()) out.pop_back();
    pos_ += len;
    return out;
  }

  auto peek_u8() const -> std::optional<std::uint8_t> {
    if (pos_ >= size_) return std::nullopt;
    return data_[pos_];
  }

  auto skip(std::size_t n) -> bool {
    if (pos_ + n > size_) return false;
    pos_ += n;
    return true;
  }

private:
  const std::uint8_t* data_;
  std::size_t size_;
  std::size_t pos_;
};

auto format_hex(std::uint64_t v) -> std::string {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%llu (0x%llX)", static_cast<unsigned long long>(v),
                static_cast<unsigned long long>(v));
  return buf;
}

auto format_signed(std::int64_t v) -> std::string {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(v));
  return buf;
}

auto format_float(float v) -> std::string {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.4f", static_cast<double>(v));
  return buf;
}

auto add_field(std::vector<parsed_field>& out, const std::string& name, pkt::field_type type,
               std::size_t offset, std::size_t size, std::string value,
               std::string desc = "", int indent = 0) -> void {
  out.push_back({name, type, offset, size, std::move(value), std::move(desc), indent});
}

// ---------------------------------------------------------------------------
// Dedicated Semantic Parsers for High-Value Silkroad Game Packets
// ---------------------------------------------------------------------------

auto parse_handshake_setup(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off0 = cur.position();
  auto flag_opt = cur.read_u8();
  if (!flag_opt) return false;
  const std::uint8_t flag = *flag_opt;

  std::string flag_desc;
  if (flag & 0x01) flag_desc += "[Session] ";
  if (flag & 0x02) flag_desc += "[Blowfish Encrypted] ";
  if (flag & 0x04) flag_desc += "[CRC Rolling Counter] ";
  if (flag & 0x08) flag_desc += "[Extended Mode] ";
  if (flag & 0x10) flag_desc += "[Challenge / Signature] ";
  if (flag_desc.empty()) flag_desc = "None";

  add_field(out, "SecurityFlags", pkt::field_type::u8, off0, 1, format_hex(flag), flag_desc);

  if (flag & 0x02) {
    const auto off_seed = cur.position();
    auto seed_hex = cur.read_hex_bytes(8);
    if (!seed_hex) return false;
    add_field(out, "BlowfishKeySeed", pkt::field_type::raw, off_seed, 8, *seed_hex, "8-byte cipher key derivation seed");
  }

  if (flag & 0x04) {
    const auto off_crc1 = cur.position();
    auto crc_client = cur.read_u32();
    if (!crc_client) return false;
    add_field(out, "ClientCRCSeed", pkt::field_type::u32, off_crc1, 4, format_hex(*crc_client), "LFSR PRNG client seed");

    if (cur.remaining() >= 4 && ((flag & 0x10) ? cur.remaining() > 4 : true)) {
      const auto off_crc2 = cur.position();
      auto crc_server = cur.read_u32();
      if (crc_server) {
        add_field(out, "ServerCRCSeed", pkt::field_type::u32, off_crc2, 4, format_hex(*crc_server), "LFSR PRNG server seed");
      }
    }
  }

  if ((flag & 0x10) || cur.remaining() > 0) {
    const auto rem = cur.remaining();
    if (rem > 0) {
      const auto off_sig = cur.position();
      auto sig = cur.read_hex_bytes(rem);
      if (sig) {
        add_field(out, "HandshakeChallengeSignature", pkt::field_type::raw, off_sig, rem, *sig,
                  rem == 8 ? "8-byte handshake secondary challenge" : "Cryptographic server challenge/signature");
      }
    }
  }

  return true;
}

auto parse_identification(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off = cur.position();
  auto name_opt = cur.read_ascii();
  if (name_opt) {
    add_field(out, "ServiceName", pkt::field_type::ascii, off, cur.position() - off, *name_opt,
              "Identity service moniker (GatewayServer, AgentServer)");
    if (cur.remaining() > 0 && cur.peek_u8() == 0) {
      cur.skip(1);
    }
    return true;
  }
  if (cur.remaining() >= 2) {
    const auto save_pos = cur.position();
    auto len_opt = cur.read_u16();
    if (len_opt && *len_opt > 0 && cur.remaining() > 0) {
      const auto take = (std::min)(static_cast<std::size_t>(*len_opt), cur.remaining());
      auto s = cur.read_fixed_ascii(take);
      if (s) {
        add_field(out, "ServiceName", pkt::field_type::ascii, save_pos, cur.position() - save_pos, *s,
                  "Identity service moniker (GatewayServer, AgentServer)");
        if (cur.remaining() > 0 && cur.peek_u8() == 0) cur.skip(1);
        return true;
      }
    }
    cur.set_position(save_pos);
  }
  if (cur.remaining() > 0) {
    const auto rem = cur.remaining();
    auto raw_s = cur.read_fixed_ascii(rem);
    if (raw_s) {
      add_field(out, "ServiceName", pkt::field_type::ascii, off, rem, *raw_s, "Service identification string");
      return true;
    }
  }
  return false;
}

auto parse_massive_chunk(cursor& cur, std::vector<parsed_field>& out) -> bool {
  if (cur.remaining() >= 1) {
    const auto off_idx = cur.position();
    auto idx = cur.read_u8();
    if (idx) {
      add_field(out, "ChunkSequenceIndex", pkt::field_type::u8, off_idx, 1, format_hex(*idx), "Massive fragment index");
    }
  }
  if (cur.remaining() >= 1) {
    const auto off_flag = cur.position();
    auto flag = cur.read_u8();
    if (flag) {
      add_field(out, "ChunkFlag", pkt::field_type::u8, off_flag, 1, format_hex(*flag), "Segment continuation flag");
    }
  }
  if (cur.remaining() > 0) {
    const auto rem = cur.remaining();
    const auto off_data = cur.position();
    auto hex_str = cur.read_hex_bytes(rem);
    if (hex_str) {
      add_field(out, "ChunkPayload", pkt::field_type::raw, off_data, rem, *hex_str,
                std::to_string(rem) + " bytes payload data");
    }
  }
  return true;
}

auto parse_gateway_patch_request(cursor& cur, std::vector<parsed_field>& out) -> bool {
  if (cur.remaining() >= 1) {
    const auto off = cur.position();
    auto loc = cur.read_u8();
    if (loc) add_field(out, "Locale", pkt::field_type::u8, off, 1, std::to_string(*loc));
  }
  if (cur.remaining() >= 2) {
    const auto off = cur.position();
    auto s = cur.read_ascii();
    if (s) add_field(out, "ServiceName", pkt::field_type::ascii, off, cur.position() - off, *s);
  }
  if (cur.remaining() >= 4) {
    const auto off = cur.position();
    auto ver = cur.read_u32();
    if (ver) add_field(out, "ClientVersion", pkt::field_type::u32, off, 4, std::to_string(*ver));
  }
  return true;
}

auto parse_gateway_patch_response(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_res = cur.position();
  auto res_opt = cur.read_u8();
  if (!res_opt) return false;
  const std::uint8_t res = *res_opt;
  add_field(out, "Result", pkt::field_type::u8, off_res, 1, res == 1 ? "1 (Success / Current)" : format_hex(res),
            res == 1 ? "Client is up to date" : "Patch required");
  if (cur.remaining() >= 4) {
    const auto off = cur.position();
    auto ver = cur.read_u32();
    if (ver) add_field(out, "ServerVersion", pkt::field_type::u32, off, 4, std::to_string(*ver));
  }
  return true;
}

auto parse_gateway_login_request(cursor& cur, std::vector<parsed_field>& out) -> bool {
  if (cur.remaining() >= 1) {
    const auto off_loc = cur.position();
    auto loc = cur.read_u8();
    if (loc) {
      add_field(out, "Locale", pkt::field_type::u8, off_loc, 1, std::to_string(*loc),
                *loc == 22 ? "iSRO" : (*loc == 18 ? "vSRO" : "Silkroad Locale"));
    }
  }
  if (cur.remaining() >= 2) {
    const auto off_user = cur.position();
    auto user = cur.read_ascii();
    if (user) {
      add_field(out, "Username", pkt::field_type::ascii, off_user, cur.position() - off_user, *user, "Account login name");
    }
  }
  if (cur.remaining() >= 2) {
    const auto off_pass = cur.position();
    auto pass = cur.read_ascii();
    if (pass) {
      std::string masked(pass->size(), '*');
      add_field(out, "Password", pkt::field_type::ascii, off_pass, cur.position() - off_pass, masked, "Account password (masked)");
    }
  }
  if (cur.remaining() >= 2) {
    const auto off_shard = cur.position();
    auto shard = cur.read_u16();
    if (shard) {
      add_field(out, "ShardID", pkt::field_type::u16, off_shard, 2, std::to_string(*shard), "Target server shard ID");
    }
  }
  return true;
}

auto parse_gateway_login_response(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_res = cur.position();
  auto res_opt = cur.read_u8();
  if (!res_opt) return false;
  const std::uint8_t res = *res_opt;
  add_field(out, "Result", pkt::field_type::u8, off_res, 1, res == 1 ? "1 (Success)" : format_hex(res),
            res == 1 ? "Authentication successful" : "Authentication failed");

  if (res == 1) {
    if (cur.remaining() >= 4) {
      const auto off_tok = cur.position();
      auto tok = cur.read_u32();
      if (tok) {
        add_field(out, "UserLoginToken", pkt::field_type::u32, off_tok, 4, format_hex(*tok), "AgentServer session authorization token");
      }
    }

    // Determine variant:
    // Format A (SRO Gateway Login Token & Account Ack): [ServerTypeFlag: u8] [ShardID: u16] [SubFlag: u8] [AccountName: ascii] ...
    // Format B (Direct Agent Redirect): [AgentServerIP: ascii] [AgentServerPort: u16] ...
    if (cur.remaining() >= 4 && cur.peek_u8() && (*cur.peek_u8() == 1 || *cur.peek_u8() == 2)) {
      const auto off_flag1 = cur.position();
      auto flag1 = cur.read_u8();
      if (flag1) add_field(out, "ServerTypeFlag", pkt::field_type::u8, off_flag1, 1, format_hex(*flag1));

      if (cur.remaining() >= 2) {
        const auto off_shard = cur.position();
        auto shard = cur.read_u16();
        if (shard) add_field(out, "ShardID", pkt::field_type::u16, off_shard, 2, std::to_string(*shard));
      }
      if (cur.remaining() >= 1) {
        const auto off_flag2 = cur.position();
        auto flag2 = cur.read_u8();
        if (flag2) add_field(out, "SubFlag", pkt::field_type::u8, off_flag2, 1, format_hex(*flag2));
      }
      if (cur.remaining() >= 2) {
        const auto off_acc = cur.position();
        auto acc = cur.read_ascii();
        if (acc) {
          add_field(out, "AccountName", pkt::field_type::ascii, off_acc, cur.position() - off_acc, *acc, "Authenticated user account");
        } else if (cur.remaining() > 0) {
          const auto rem = cur.remaining();
          auto raw = cur.read_fixed_ascii(rem);
          if (raw) add_field(out, "AccountName", pkt::field_type::ascii, off_acc, rem, *raw, "Account name (partial)");
        }
      }
    } else {
      if (cur.remaining() >= 2) {
        const auto off_ip = cur.position();
        auto ip = cur.read_ascii();
        if (ip) {
          add_field(out, "AgentServerIP", pkt::field_type::ascii, off_ip, cur.position() - off_ip, *ip, "AgentServer connection address");
        }
      }
      if (cur.remaining() >= 2) {
        const auto off_port = cur.position();
        auto port = cur.read_u16();
        if (port) {
          add_field(out, "AgentServerPort", pkt::field_type::u16, off_port, 2, std::to_string(*port), "AgentServer TCP port");
        }
      }
    }
  } else {
    if (cur.remaining() >= 1) {
      const auto off_err = cur.position();
      auto err = cur.read_u8();
      if (err) {
        std::string desc = "Error code " + std::to_string(*err);
        switch (*err) {
        case 1: desc = "Account Blocked / Banned"; break;
        case 2: desc = "Invalid Username or Password"; break;
        case 3: desc = "Already Logged In / Overlap"; break;
        case 4: desc = "Server Full"; break;
        case 5: desc = "Server Under Maintenance"; break;
        case 27: desc = "Password Incorrect"; break;
        case 35: desc = "Max Login Attempt Exceeded"; break;
        default: break;
        }
        add_field(out, "ErrorCode", pkt::field_type::u8, off_err, 1, format_hex(*err), desc);
      }
    }
    if (cur.remaining() >= 4) {
      const auto off_a = cur.position();
      auto a = cur.read_u32();
      if (a) add_field(out, "AttemptCount", pkt::field_type::u32, off_a, 4, std::to_string(*a));
    }
    if (cur.remaining() >= 4) {
      const auto off_m = cur.position();
      auto m = cur.read_u32();
      if (m) add_field(out, "MaxAttempts", pkt::field_type::u32, off_m, 4, std::to_string(*m));
    }
  }
  return true;
}

auto parse_gateway_serverlist_response(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_cnt = cur.position();
  auto cnt_opt = cur.read_u8();
  if (!cnt_opt) return false;
  const std::uint8_t count = *cnt_opt;
  add_field(out, "ServerCount", pkt::field_type::u8, off_cnt, 1, std::to_string(count), "Number of Gateway/Agent server nodes");

  for (std::uint8_t i = 0; i < count && cur.remaining() >= 5; ++i) {
    const auto off_node = cur.position();
    auto sid = cur.read_u8();
    auto ip_raw = cur.read_u32();
    if (!sid || !ip_raw) break;

    const std::uint8_t* ip_b = reinterpret_cast<const std::uint8_t*>(&*ip_raw);
    char ip_str[32];
    std::snprintf(ip_str, sizeof(ip_str), "%u.%u.%u.%u", ip_b[0], ip_b[1], ip_b[2], ip_b[3]);

    add_field(out, "ServerNode[" + std::to_string(i) + "].ID", pkt::field_type::u8, off_node, 1, std::to_string(*sid), "", 1);
    add_field(out, "ServerNode[" + std::to_string(i) + "].IP", pkt::field_type::ascii, off_node + 1, 4, ip_str, "Agent IP address", 1);
  }
  return true;
}

auto parse_gateway_shard_list_response(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_cnt = cur.position();
  auto cnt_opt = cur.read_u8();
  if (!cnt_opt) return false;
  const std::uint8_t count = *cnt_opt;
  add_field(out, "ServerCount", pkt::field_type::u8, off_cnt, 1, std::to_string(count), "Total server nodes listed");

  for (std::uint8_t i = 0; i < count && cur.remaining() > 0; ++i) {
    const auto off_node = cur.position();
    auto sid = cur.read_u8();
    if (!sid.has_value()) break;
    add_field(out, "ServerNode[" + std::to_string(i) + "].ID", pkt::field_type::u8, off_node, 1, std::to_string(*sid), "", 1);

    const auto off_ip = cur.position();
    auto ip = cur.read_ascii();
    if (ip) {
      add_field(out, "ServerNode[" + std::to_string(i) + "].IP", pkt::field_type::ascii, off_ip, cur.position() - off_ip, *ip, "Server IP address", 1);
    } else {
      const auto rem = cur.remaining();
      if (rem > 0) {
        auto partial = cur.read_fixed_ascii(rem);
        if (partial) {
          add_field(out, "ServerNode[" + std::to_string(i) + "].IP", pkt::field_type::ascii, off_ip, rem, *partial, "Server IP address (partial)", 1);
        }
      }
      break;
    }

    if (cur.remaining() >= 2) {
      const auto off_port = cur.position();
      auto port = cur.read_u16();
      if (port) {
        add_field(out, "ServerNode[" + std::to_string(i) + "].Port", pkt::field_type::u16, off_port, 2,
                  std::to_string(*port) + " (" + format_hex(*port) + ")", "Gateway server port", 1);
      }
    }
  }
  return true;
}

auto parse_chat_request(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_type = cur.position();
  auto type_opt = cur.read_u8();
  if (!type_opt) return false;
  const std::uint8_t type = *type_opt;
  add_field(out, "ChatType", pkt::field_type::u8, off_type, 1, std::to_string(type), chat_type_name(type));

  const auto off_seq = cur.position();
  auto seq_opt = cur.read_u8();
  if (!seq_opt) return false;
  add_field(out, "ChannelSequence", pkt::field_type::u8, off_seq, 1, std::to_string(*seq_opt));

  const auto off_reserved = cur.position();
  auto reserved = cur.read_u16();
  if (!reserved) return false;
  add_field(out, "Reserved", pkt::field_type::u16, off_reserved, 2, std::to_string(*reserved),
            "Two zero bytes written ahead of the message");

  if (type == 2) { // PM
    const auto off_target = cur.position();
    auto target_opt = cur.read_ascii();
    if (!target_opt) return false;
    add_field(out, "RecipientPlayerName", pkt::field_type::ascii, off_target, cur.position() - off_target,
              *target_opt, "Whisper target name");
  }

  const auto off_msg = cur.position();
  auto msg_opt = cur.read_utf16();
  if (!msg_opt) return false;
  add_field(out, "Message", pkt::field_type::ascii, off_msg, cur.position() - off_msg, *msg_opt,
            "UTF-16LE chat message text");

  return true;
}

auto parse_chat_update(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_type = cur.position();
  auto type_opt = cur.read_u8();
  if (!type_opt) return false;
  const std::uint8_t type = *type_opt;
  add_field(out, "ChatType", pkt::field_type::u8, off_type, 1, std::to_string(type), chat_type_name(type));

  if (type == 1 || type == 3 || type == 4) { // All, Party, Guild
    const auto off_id = cur.position();
    auto id_opt = cur.read_u32();
    if (!id_opt) return false;
    add_field(out, "SenderUniqueID", pkt::field_type::u32, off_id, 4, format_hex(*id_opt), "World entity ID");

    const auto off_msg = cur.position();
    auto msg_opt = cur.read_ascii();
    if (!msg_opt) return false;
    add_field(out, "Message", pkt::field_type::ascii, off_msg, cur.position() - off_msg, *msg_opt);
  } else if (type == 2 || type == 5) { // PM or Global
    const auto off_sender = cur.position();
    auto sender_opt = cur.read_ascii();
    if (!sender_opt) return false;
    add_field(out, "SenderName", pkt::field_type::ascii, off_sender, cur.position() - off_sender, *sender_opt);

    const auto off_msg = cur.position();
    auto msg_opt = cur.read_ascii();
    if (!msg_opt) return false;
    add_field(out, "Message", pkt::field_type::ascii, off_msg, cur.position() - off_msg, *msg_opt);
  } else if (type == 6) { // Notice
    const auto off_msg = cur.position();
    auto msg_opt = cur.read_ascii();
    if (!msg_opt) return false;
    add_field(out, "NoticeMessage", pkt::field_type::ascii, off_msg, cur.position() - off_msg, *msg_opt);
  } else {
    // Fallback: read any remaining string or fields
    if (cur.remaining() >= 4) {
      const auto off_id = cur.position();
      auto id_opt = cur.read_u32();
      if (id_opt) {
        add_field(out, "SenderID", pkt::field_type::u32, off_id, 4, format_hex(*id_opt));
      }
    }
    if (cur.remaining() >= 2) {
      const auto off_msg = cur.position();
      auto msg_opt = cur.read_ascii();
      if (msg_opt) {
        add_field(out, "Message", pkt::field_type::ascii, off_msg, cur.position() - off_msg, *msg_opt);
      }
    }
  }

  return true;
}

auto parse_movement_client(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_type = cur.position();
  auto move_type = cur.read_u8();
  if (!move_type) return false;
  add_field(out, "MovementType", pkt::field_type::u8, off_type, 1, std::to_string(*move_type),
            *move_type == 1 ? "Move to destination" : "Stop / Update facing");

  if (*move_type == 1) {
    const auto off_reg = cur.position();
    auto reg = cur.read_u16();
    if (!reg) return false;
    add_field(out, "DestRegionID", pkt::field_type::u16, off_reg, 2, format_hex(*reg));

    const auto off_x = cur.position();
    auto x = cur.read_i32();
    if (!x) return false;
    add_field(out, "DestX", pkt::field_type::i32, off_x, 4, format_signed(*x), "World X coordinate");

    const auto off_y = cur.position();
    auto y = cur.read_i32();
    if (!y) return false;
    add_field(out, "DestY", pkt::field_type::i32, off_y, 4, format_signed(*y), "World Y height coordinate");

    const auto off_z = cur.position();
    auto z = cur.read_i32();
    if (!z) return false;
    add_field(out, "DestZ", pkt::field_type::i32, off_z, 4, format_signed(*z), "World Z coordinate");
  } else {
    if (cur.remaining() >= 2) {
      const auto off_ang = cur.position();
      auto ang = cur.read_u16();
      if (ang) {
        add_field(out, "FacingAngle", pkt::field_type::u16, off_ang, 2, std::to_string(*ang));
      }
    }
  }
  return true;
}

auto parse_movement_server(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_id = cur.position();
  auto uid = cur.read_u32();
  if (!uid) return false;
  add_field(out, "EntityUniqueID", pkt::field_type::u32, off_id, 4, format_hex(*uid), "Entity moving");

  const auto off_has = cur.position();
  auto has = cur.read_bool();
  if (!has) return false;
  add_field(out, "HasMovement", pkt::field_type::bool_, off_has, 1, *has ? "true" : "false");

  if (*has) {
    const auto off_reg = cur.position();
    auto reg = cur.read_u16();
    if (!reg) return false;
    add_field(out, "DestRegionID", pkt::field_type::u16, off_reg, 2, format_hex(*reg));

    const auto off_x = cur.position();
    auto x = cur.read_i32();
    if (!x) return false;
    add_field(out, "DestX", pkt::field_type::i32, off_x, 4, format_signed(*x));

    const auto off_y = cur.position();
    auto y = cur.read_i32();
    if (!y) return false;
    add_field(out, "DestY", pkt::field_type::i32, off_y, 4, format_signed(*y));

    const auto off_z = cur.position();
    auto z = cur.read_i32();
    if (!z) return false;
    add_field(out, "DestZ", pkt::field_type::i32, off_z, 4, format_signed(*z));
  }
  return true;
}

auto parse_entity_spawn(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_model = cur.position();
  auto model = cur.read_u32();
  if (!model) return false;
  add_field(out, "RefObjID", pkt::field_type::u32, off_model, 4, format_hex(*model), "Model ID from Media.pk2 characterdata");

  const auto off_scale = cur.position();
  auto scale = cur.read_u8();
  if (!scale) return false;
  add_field(out, "Scale", pkt::field_type::u8, off_scale, 1, std::to_string(*scale));

  const auto off_type = cur.position();
  auto type_opt = cur.read_u8();
  if (!type_opt) return false;
  const std::uint8_t entity_type = *type_opt;
  add_field(out, "EntityType", pkt::field_type::u8, off_type, 1, std::to_string(entity_type), entity_type_name(entity_type));

  const auto off_uid = cur.position();
  auto uid = cur.read_u32();
  if (!uid) return false;
  add_field(out, "UniqueID", pkt::field_type::u32, off_uid, 4, format_hex(*uid), "Session UniqueID");

  const auto off_reg = cur.position();
  auto reg = cur.read_u16();
  if (!reg) return false;
  add_field(out, "RegionID", pkt::field_type::u16, off_reg, 2, format_hex(*reg));

  const auto off_x = cur.position();
  auto x = cur.read_f32();
  if (!x) return false;
  add_field(out, "PosX", pkt::field_type::f32, off_x, 4, format_float(*x));

  const auto off_y = cur.position();
  auto y = cur.read_f32();
  if (!y) return false;
  add_field(out, "PosY", pkt::field_type::f32, off_y, 4, format_float(*y));

  const auto off_z = cur.position();
  auto z = cur.read_f32();
  if (!z) return false;
  add_field(out, "PosZ", pkt::field_type::f32, off_z, 4, format_float(*z));

  const auto off_ang = cur.position();
  auto ang = cur.read_u16();
  if (ang) {
    add_field(out, "Angle", pkt::field_type::u16, off_ang, 2, std::to_string(*ang));
  }

  if (entity_type == 1) { // Player
    if (cur.remaining() >= 4) {
      const auto off_life = cur.position();
      auto life = cur.read_u8();
      if (life) add_field(out, "LifeState", pkt::field_type::u8, off_life, 1, std::to_string(*life), life_state_name(*life), 1);

      const auto off_body = cur.position();
      auto body = cur.read_u8();
      if (body) add_field(out, "BodyMode", pkt::field_type::u8, off_body, 1, std::to_string(*body), body_mode_name(*body), 1);

      const auto off_motion = cur.position();
      auto motion = cur.read_u8();
      if (motion) add_field(out, "MotionState", pkt::field_type::u8, off_motion, 1, std::to_string(*motion), motion_state_name(*motion), 1);

      const auto off_pvp = cur.position();
      auto pvp = cur.read_u8();
      if (pvp) add_field(out, "PvPState", pkt::field_type::u8, off_pvp, 1, std::to_string(*pvp), pvp_state_name(*pvp), 1);
    }
    if (cur.remaining() >= 12) {
      const auto off_w = cur.position();
      auto w = cur.read_f32();
      if (w) add_field(out, "SpeedWalking", pkt::field_type::f32, off_w, 4, format_float(*w), "", 1);

      const auto off_r = cur.position();
      auto r = cur.read_f32();
      if (r) add_field(out, "SpeedRunning", pkt::field_type::f32, off_r, 4, format_float(*r), "", 1);

      const auto off_b = cur.position();
      auto b = cur.read_f32();
      if (b) add_field(out, "SpeedBerserk", pkt::field_type::f32, off_b, 4, format_float(*b), "", 1);
    }
    if (cur.remaining() >= 2) {
      const auto off_name = cur.position();
      auto name = cur.read_ascii();
      if (name) add_field(out, "CharacterName", pkt::field_type::ascii, off_name, cur.position() - off_name, *name, "", 1);
    }
  }

  return true;
}

auto parse_entity_despawn(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_uid = cur.position();
  auto uid = cur.read_u32();
  if (!uid) return false;
  add_field(out, "UniqueID", pkt::field_type::u32, off_uid, 4, format_hex(*uid));

  if (cur.remaining() >= 1) {
    const auto off_reason = cur.position();
    auto reason = cur.read_u8();
    if (reason) {
      add_field(out, "DespawnReason", pkt::field_type::u8, off_reason, 1, std::to_string(*reason), despawn_reason_name(*reason));
    }
  }
  return true;
}

auto parse_character_stats(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_pmin = cur.position();
  auto pmin = cur.read_u32();
  if (!pmin) return false;
  add_field(out, "PhyMinAtk", pkt::field_type::u32, off_pmin, 4, format_signed(*pmin));

  const auto off_pmax = cur.position();
  auto pmax = cur.read_u32();
  if (!pmax) return false;
  add_field(out, "PhyMaxAtk", pkt::field_type::u32, off_pmax, 4, format_signed(*pmax));

  const auto off_mmin = cur.position();
  auto mmin = cur.read_u32();
  if (!mmin) return false;
  add_field(out, "MagMinAtk", pkt::field_type::u32, off_mmin, 4, format_signed(*mmin));

  const auto off_mmax = cur.position();
  auto mmax = cur.read_u32();
  if (!mmax) return false;
  add_field(out, "MagMaxAtk", pkt::field_type::u32, off_mmax, 4, format_signed(*mmax));

  const auto off_pdef = cur.position();
  auto pdef = cur.read_u16();
  if (!pdef) return false;
  add_field(out, "PhyDef", pkt::field_type::u16, off_pdef, 2, format_signed(*pdef));

  const auto off_mdef = cur.position();
  auto mdef = cur.read_u16();
  if (!mdef) return false;
  add_field(out, "MagDef", pkt::field_type::u16, off_mdef, 2, format_signed(*mdef));

  const auto off_hit = cur.position();
  auto hit = cur.read_u16();
  if (!hit) return false;
  add_field(out, "HitRate", pkt::field_type::u16, off_hit, 2, format_signed(*hit));

  const auto off_par = cur.position();
  auto par = cur.read_u16();
  if (!par) return false;
  add_field(out, "ParryRate", pkt::field_type::u16, off_par, 2, format_signed(*par));

  const auto off_hp = cur.position();
  auto hp = cur.read_u32();
  if (!hp) return false;
  add_field(out, "HPMax", pkt::field_type::u32, off_hp, 4, format_signed(*hp));

  const auto off_mp = cur.position();
  auto mp = cur.read_u32();
  if (!mp) return false;
  add_field(out, "MPMax", pkt::field_type::u32, off_mp, 4, format_signed(*mp));

  const auto off_str = cur.position();
  auto str = cur.read_u16();
  if (!str) return false;
  add_field(out, "Strength (STR)", pkt::field_type::u16, off_str, 2, format_signed(*str));

  const auto off_int = cur.position();
  auto int_val = cur.read_u16();
  if (!int_val) return false;
  add_field(out, "Intelligence (INT)", pkt::field_type::u16, off_int, 2, format_signed(*int_val));

  return true;
}

auto parse_character_info(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_gold = cur.position();
  auto gold = cur.read_u64();
  if (!gold) return false;
  add_field(out, "Gold", pkt::field_type::u64, off_gold, 8, format_signed(*gold), "Current character inventory gold");

  const auto off_sp = cur.position();
  auto sp = cur.read_u32();
  if (!sp) return false;
  add_field(out, "SkillPoints (SP)", pkt::field_type::u32, off_sp, 4, format_signed(*sp));

  const auto off_bp = cur.position();
  auto bp = cur.read_u8();
  if (bp) {
    add_field(out, "BerserkPoints", pkt::field_type::u8, off_bp, 1, std::to_string(*bp), "Berserk gauge orbs");
  }
  return true;
}

auto parse_entity_damage(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_att = cur.position();
  auto att = cur.read_u32();
  if (!att) return false;
  add_field(out, "AttackerUniqueID", pkt::field_type::u32, off_att, 4, format_hex(*att));

  const auto off_def = cur.position();
  auto def = cur.read_u32();
  if (!def) return false;
  add_field(out, "DefenderUniqueID", pkt::field_type::u32, off_def, 4, format_hex(*def));

  const auto off_flag = cur.position();
  auto flag = cur.read_u8();
  if (!flag) return false;
  add_field(out, "DamageFlag", pkt::field_type::u8, off_flag, 1, std::to_string(*flag), damage_flag_name(*flag));

  const auto off_dmg = cur.position();
  auto dmg = cur.read_u32();
  if (!dmg) return false;
  add_field(out, "DamageAmount", pkt::field_type::u32, off_dmg, 4, format_signed(*dmg));

  return true;
}

auto parse_item_movement(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_src = cur.position();
  auto src = cur.read_u8();
  if (!src) return false;
  add_field(out, "SourceSlot", pkt::field_type::u8, off_src, 1, std::to_string(*src));

  const auto off_dst = cur.position();
  auto dst = cur.read_u8();
  if (!dst) return false;
  add_field(out, "DestSlot", pkt::field_type::u8, off_dst, 1, std::to_string(*dst));

  const auto off_count = cur.position();
  auto count = cur.read_u16();
  if (count) {
    add_field(out, "ItemQuantity", pkt::field_type::u16, off_count, 2, std::to_string(*count));
  }
  return true;
}

auto parse_target_selection(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_uid = cur.position();
  auto uid = cur.read_u32();
  if (!uid) return false;
  add_field(out, "TargetUniqueID", pkt::field_type::u32, off_uid, 4, format_hex(*uid), "Targeted entity");
  return true;
}

auto parse_notice(cursor& cur, std::vector<parsed_field>& out) -> bool {
  const auto off_type = cur.position();
  auto type_opt = cur.read_u8();
  if (!type_opt) return false;
  add_field(out, "NoticeType", pkt::field_type::u8, off_type, 1, std::to_string(*type_opt), notice_type_name(*type_opt));

  const auto off_msg = cur.position();
  auto msg_opt = cur.read_ascii();
  if (!msg_opt) return false;
  add_field(out, "NoticeMessage", pkt::field_type::ascii, off_msg, cur.position() - off_msg, *msg_opt);
  return true;
}

auto try_semantic_parse(std::uint16_t opcode, cursor& cur, std::vector<parsed_field>& out) -> bool {
  switch (opcode) {
  case 0x2001:
    return parse_identification(cur, out);
  case 0x5000:
    return parse_handshake_setup(cur, out);
  case 0x600D:
    return parse_massive_chunk(cur, out);
  case 0x6100:
    return parse_gateway_patch_request(cur, out);
  case 0x6101:
    return parse_gateway_login_request(cur, out);
  case 0xA100:
    return parse_gateway_patch_response(cur, out);
  case 0xA101:
    return parse_gateway_login_response(cur, out);
  case 0xA106:
    return parse_gateway_serverlist_response(cur, out);
  case 0xA107:
    return parse_gateway_shard_list_response(cur, out);
  case 0x7025:
    return parse_chat_request(cur, out);
  case 0x3026:
  case 0x3055:
    return parse_chat_update(cur, out);
  case 0x7021:
    return parse_movement_client(cur, out);
  case 0xB021:
    return parse_movement_server(cur, out);
  case 0x3015:
    return parse_entity_spawn(cur, out);
  case 0x3016:
    return parse_entity_despawn(cur, out);
  case 0x303D:
    return parse_character_stats(cur, out);
  case 0x304E:
    return parse_character_info(cur, out);
  case 0x3068:
    return parse_entity_damage(cur, out);
  case 0x7034:
  case 0xB034:
    return parse_item_movement(cur, out);
  case 0x7045:
  case 0xB045:
    return parse_target_selection(cur, out);
  case 0x300C:
  case 0x3100:
    return parse_notice(cur, out);
  default:
    return false;
  }
}

// ---------------------------------------------------------------------------
// Improved Generic Table-Driven Parser
// ---------------------------------------------------------------------------

struct field_runtime_val {
  std::string name;
  std::int64_t int_val = 0;
};

auto walk_fields(const pkt::field_def* defs, std::uint16_t start, std::uint16_t count,
                 cursor& cur, std::vector<parsed_field>& out, std::vector<field_runtime_val>& vals,
                 int indent) -> bool {
  for (std::uint16_t i = 0; i < count; ++i) {
    const auto& def = defs[start + i];
    const std::size_t field_offset = cur.position();

    if (def.type == pkt::field_type::loop) {
      std::uint32_t loop_count = 0;
      if (def.loop_count_ref == 0xFFFF) {
        // ReadBool-based loop
        int iter = 0;
        while (iter++ < 256) {
          auto b = cur.read_bool();
          if (!b || !*b) break;
          if (def.child_count > 0) {
            walk_fields(defs, def.child_start, def.child_count, cur, out, vals, indent + 1);
          }
        }
      } else {
        // Count-based loop: look up the referenced field's value
        bool found = false;
        for (auto it = vals.rbegin(); it != vals.rend(); ++it) {
          if (it->name.find("count") != std::string::npos || it->name.find("Count") != std::string::npos) {
            loop_count = static_cast<std::uint32_t>(std::max<std::int64_t>(0, it->int_val));
            found = true;
            break;
          }
        }
        if (!found && !vals.empty()) {
          loop_count = static_cast<std::uint32_t>(std::max<std::int64_t>(0, vals.back().int_val));
          found = true;
        }
        if (!found) {
          auto c = cur.read_u8();
          if (!c) return false;
          loop_count = *c;
        }
        if (loop_count > 256) loop_count = 256; // Protect against corruption

        add_field(out, def.name, pkt::field_type::loop, field_offset, 0,
                  std::to_string(loop_count) + " iterations", "", indent);
        for (std::uint32_t j = 0; j < loop_count; ++j) {
          if (def.child_count > 0) {
            walk_fields(defs, def.child_start, def.child_count, cur, out, vals, indent + 1);
          }
        }
      }
      continue;
    }

    if (def.type == pkt::field_type::branch) {
      std::int64_t switch_val = -1;
      if (!vals.empty()) {
        switch_val = vals.back().int_val;
      }
      // If branch has children, find matching case or execute common
      if (def.child_count > 0) {
        if (def.branch_value == -1 || def.branch_value == switch_val) {
          walk_fields(defs, def.child_start, def.child_count, cur, out, vals, indent + 1);
        }
      }
      continue;
    }

    // Primitive fields
    parsed_field pf;
    pf.name = def.name;
    pf.type = def.type;
    pf.offset = field_offset;
    pf.indent = indent;

    std::int64_t read_num = 0;
    bool has_num = false;

    switch (def.type) {
    case pkt::field_type::u8: {
      auto v = cur.read_u8();
      if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
      pf.size = 1;
      pf.value = format_hex(*v);
      read_num = *v;
      has_num = true;
      break;
    }
    case pkt::field_type::u16: {
      auto v = cur.read_u16();
      if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
      pf.size = 2;
      pf.value = format_hex(*v);
      read_num = *v;
      has_num = true;
      break;
    }
    case pkt::field_type::u32: {
      auto v = cur.read_u32();
      if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
      pf.size = 4;
      pf.value = format_hex(*v);
      read_num = *v;
      has_num = true;
      break;
    }
    case pkt::field_type::u64: {
      auto v = cur.read_u64();
      if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
      pf.size = 8;
      pf.value = format_hex(*v);
      break;
    }
    case pkt::field_type::i8: {
      auto v = cur.read_i8();
      if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
      pf.size = 1;
      pf.value = format_signed(*v);
      read_num = *v;
      has_num = true;
      break;
    }
    case pkt::field_type::i16: {
      auto v = cur.read_i16();
      if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
      pf.size = 2;
      pf.value = format_signed(*v);
      read_num = *v;
      has_num = true;
      break;
    }
    case pkt::field_type::i32: {
      auto v = cur.read_i32();
      if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
      pf.size = 4;
      pf.value = format_signed(*v);
      read_num = *v;
      has_num = true;
      break;
    }
    case pkt::field_type::i64: {
      auto v = cur.read_i64();
      if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
      pf.size = 8;
      pf.value = format_signed(*v);
      break;
    }
    case pkt::field_type::f32: {
      auto v = cur.read_f32();
      if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
      pf.size = 4;
      pf.value = format_float(*v);
      break;
    }
    case pkt::field_type::bool_: {
      auto v = cur.read_bool();
      if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
      pf.size = 1;
      pf.value = *v ? "true" : "false";
      read_num = *v ? 1 : 0;
      has_num = true;
      break;
    }
    case pkt::field_type::ascii: {
      auto v = cur.read_ascii();
      if (!v) { pf.value = "<EOF>"; pf.size = 0; out.push_back(pf); return false; }
      pf.size = cur.position() - field_offset;
      pf.value = *v;
      break;
    }
    default:
      pf.size = 0;
      pf.value = "<raw>";
      break;
    }

    if (has_num) {
      vals.push_back({def.name ? def.name : "", read_num});
    }

    out.push_back(std::move(pf));
  }
  return true;
}

// ---------------------------------------------------------------------------
// Trailing Heuristic Scanner
// ---------------------------------------------------------------------------

auto scan_trailing_bytes(cursor& cur, std::vector<parsed_field>& out) -> void {
  while (cur.remaining() > 0) {
    const auto pos = cur.position();
    // Try reading length-prefixed ASCII string
    if (cur.remaining() >= 3) {
      const auto saved_pos = cur.position();
      auto s = cur.read_ascii();
      if (s && !s->empty()) {
        bool all_printable = true;
        for (char ch : *s) {
          if (static_cast<unsigned char>(ch) < 0x20 || static_cast<unsigned char>(ch) >= 0x7F) {
            all_printable = false;
            break;
          }
        }
        if (all_printable) {
          add_field(out, "DetectedString", pkt::field_type::ascii, saved_pos, cur.position() - saved_pos,
                    *s, "Heuristic string match");
          continue;
        }
      }
      cur.set_position(saved_pos); // rollback
    }

    // Try reading integer / ID if 4 bytes
    if (cur.remaining() >= 4) {
      auto u = cur.read_u32();
      if (u) {
        add_field(out, "TrailingData_u32", pkt::field_type::u32, pos, 4, format_hex(*u));
        continue;
      }
    }

    // Single byte fallback
    auto b = cur.read_u8();
    if (b) {
      add_field(out, "TrailingByte", pkt::field_type::u8, pos, 1, format_hex(*b));
    }
  }
}

} // anonymous namespace

auto parse_packet(std::uint16_t opcode, const std::uint8_t* data, std::size_t size) -> parse_result {
  parse_result result;
  result.doc_summary = opcode_summary_doc(opcode);

  if (!data || size == 0) {
    result.success = true;
    result.error = "Empty payload";
    result.bytes_consumed = 0;
    result.parser_method = "None";
    return result;
  }

  cursor cur(data, size);

  // 1. Try high-value specialized semantic decoder first
  if (try_semantic_parse(opcode, cur, result.fields)) {
    result.parser_method = "Specialized Semantic Decoder";
    result.bytes_consumed = cur.position();
    result.success = (cur.remaining() == 0);
    if (cur.remaining() > 0) {
      char buf[64];
      std::snprintf(buf, sizeof(buf), "%zu trailing bytes", cur.remaining());
      result.error = buf;
      // Heuristically examine trailing data
      scan_trailing_bytes(cur, result.fields);
    }
    return result;
  }

  // 2. Fall back to static definition table
  const auto* op_def = pkt::lookup(opcode);
  if (op_def && op_def->field_count > 0) {
    result.parser_method = "Static Protocol Schema";
    std::vector<field_runtime_val> vals;
    vals.reserve(op_def->field_count);
    walk_fields(pkt::g_field_table, op_def->field_start, op_def->field_count, cur, result.fields, vals, 0);

    result.bytes_consumed = cur.position();
    result.success = (cur.remaining() == 0);
    if (cur.remaining() > 0) {
      char buf[64];
      std::snprintf(buf, sizeof(buf), "%zu bytes unconsumed", cur.remaining());
      result.error = buf;
      scan_trailing_bytes(cur, result.fields);
    }
    return result;
  }

  // 3. Fallback: Heuristic ASCII / byte inspection
  result.parser_method = "Heuristic Raw Scanner";
  scan_trailing_bytes(cur, result.fields);
  result.bytes_consumed = cur.position();
  result.success = true;

  return result;
}

} // namespace ext_client::plugins::net_log
