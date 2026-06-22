#pragma once

#include <cstdint>

namespace ext_client::utils {

  struct x86_insn {
    std::uint8_t len;
    std::uint8_t p_rep;
    std::uint8_t p_lock;
    std::uint8_t p_seg;
    std::uint8_t p_66;
    std::uint8_t p_67;
    std::uint8_t opcode;
    std::uint8_t opcode2;
    std::uint8_t modrm;
    std::uint8_t modrm_mod;
    std::uint8_t modrm_reg;
    std::uint8_t modrm_rm;
    std::uint8_t sib;
    std::uint8_t sib_scale;
    std::uint8_t sib_index;
    std::uint8_t sib_base;
    union {
      std::uint8_t imm8;
      std::uint16_t imm16;
      std::uint32_t imm32;
    } imm;
    union {
      std::uint8_t disp8;
      std::uint16_t disp16;
      std::uint32_t disp32;
    } disp;
    std::uint32_t flags;
  };

  // Instruction flag constants
  inline constexpr std::uint32_t F_MODRM         = 0x00000001;
  inline constexpr std::uint32_t F_SIB           = 0x00000002;
  inline constexpr std::uint32_t F_IMM8          = 0x00000004;
  inline constexpr std::uint32_t F_IMM16         = 0x00000008;
  inline constexpr std::uint32_t F_IMM32         = 0x00000010;
  inline constexpr std::uint32_t F_DISP8         = 0x00000020;
  inline constexpr std::uint32_t F_DISP16        = 0x00000040;
  inline constexpr std::uint32_t F_DISP32        = 0x00000080;
  inline constexpr std::uint32_t F_RELATIVE      = 0x00000100;
  inline constexpr std::uint32_t F_2IMM16        = 0x00000800;
  inline constexpr std::uint32_t F_ERROR         = 0x00001000;
  inline constexpr std::uint32_t F_ERROR_OPCODE  = 0x00002000;
  inline constexpr std::uint32_t F_ERROR_LENGTH  = 0x00004000;
  inline constexpr std::uint32_t F_ERROR_LOCK    = 0x00008000;
  inline constexpr std::uint32_t F_ERROR_OPERAND = 0x00010000;

  // Prefix flag constants (stored in upper bits of flags)
  inline constexpr std::uint32_t F_PREFIX_REPNZ  = 0x01000000;
  inline constexpr std::uint32_t F_PREFIX_REPX   = 0x02000000;
  inline constexpr std::uint32_t F_PREFIX_REP    = 0x03000000;
  inline constexpr std::uint32_t F_PREFIX_66     = 0x04000000;
  inline constexpr std::uint32_t F_PREFIX_67     = 0x08000000;
  inline constexpr std::uint32_t F_PREFIX_LOCK   = 0x10000000;
  inline constexpr std::uint32_t F_PREFIX_SEG    = 0x20000000;

  // Internal prefix bits
  inline constexpr std::uint8_t PRE_NONE = 0x01;
  inline constexpr std::uint8_t PRE_F2   = 0x02;
  inline constexpr std::uint8_t PRE_F3   = 0x04;
  inline constexpr std::uint8_t PRE_66   = 0x08;
  inline constexpr std::uint8_t PRE_67   = 0x10;
  inline constexpr std::uint8_t PRE_LOCK = 0x20;
  inline constexpr std::uint8_t PRE_SEG  = 0x40;

  // Opcode class flags
  inline constexpr std::uint8_t C_NONE    = 0x00;
  inline constexpr std::uint8_t C_MODRM   = 0x01;
  inline constexpr std::uint8_t C_IMM8    = 0x02;
  inline constexpr std::uint8_t C_IMM16   = 0x04;
  inline constexpr std::uint8_t C_IMM_P66 = 0x10;
  inline constexpr std::uint8_t C_REL8    = 0x20;
  inline constexpr std::uint8_t C_REL32   = 0x40;
  inline constexpr std::uint8_t C_GROUP   = 0x80;
  inline constexpr std::uint8_t C_ERROR   = 0xff;

  auto x86_disasm(const void* code, x86_insn* hs) -> unsigned int;

} // namespace ext_client::utils
