#include "utils/hooks.hpp"

#include "utils/x86_lde.hpp"

#include <windows.h>
#include <cstring>
#include <vector>
#include <mutex>

namespace ext_client::utils {

// ---------------------------------------------------------------------------
// Instruction structs for patching (x86 only)
// ---------------------------------------------------------------------------

#pragma pack(push, 1)

struct jmp_rel_short {
  std::uint8_t opcode;   // EB xx
  std::uint8_t operand;
};

struct jmp_rel {
  std::uint8_t opcode;   // E9 xxxxxxxx
  std::uint32_t operand;
};

struct call_rel {
  std::uint8_t opcode;   // E8 xxxxxxxx
  std::uint32_t operand;
};

struct jcc_rel {
  std::uint8_t opcode0;  // 0F 8x xxxxxxxx
  std::uint8_t opcode1;
  std::uint32_t operand;
};

#pragma pack(pop)

// ---------------------------------------------------------------------------
// Executable memory allocator (replaces MinHook's buffer.c)
// ---------------------------------------------------------------------------

namespace {

constexpr std::uintptr_t MEMORY_BLOCK_SIZE = 0x1000;
constexpr std::size_t MEMORY_SLOT_SIZE = 32;
constexpr std::size_t TRAMPOLINE_MAX_SIZE = MEMORY_SLOT_SIZE;

struct memory_slot {
  union {
    memory_slot* next;
    std::uint8_t buffer[MEMORY_SLOT_SIZE];
  };
};

struct memory_block {
  memory_block* next;
  memory_slot* free;
  std::uint32_t used_count;
};

memory_block* g_memory_blocks = nullptr;

auto is_executable_address(void* addr) -> bool {
  MEMORY_BASIC_INFORMATION mi;
  if (VirtualQuery(addr, &mi, sizeof(mi)) == 0)
    return false;
  constexpr DWORD exec_flags = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
  return mi.State == MEM_COMMIT && (mi.Protect & exec_flags);
}

auto is_code_padding(std::uint8_t* p, std::uint32_t size) -> bool {
  if (p[0] != 0x00 && p[0] != 0x90 && p[0] != 0xCC)
    return false;
  for (std::uint32_t i = 1; i < size; ++i) {
    if (p[i] != p[0])
      return false;
  }
  return true;
}

auto get_memory_block() -> memory_block* {
  auto* block = (memory_block*)VirtualAlloc(nullptr, MEMORY_BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (!block)
    return nullptr;

  auto* slot = (memory_slot*)(block + 1);
  block->free = nullptr;
  block->used_count = 0;
  do {
    slot->next = block->free;
    block->free = slot;
    slot++;
  } while ((std::uintptr_t)slot - (std::uintptr_t)block <= MEMORY_BLOCK_SIZE - MEMORY_SLOT_SIZE);

  block->next = g_memory_blocks;
  g_memory_blocks = block;
  return block;
}

auto alloc_trampoline() -> void* {
  for (auto* block = g_memory_blocks; block; block = block->next) {
    if (block->free) {
      auto* slot = block->free;
      block->free = slot->next;
      block->used_count++;
      return slot;
    }
  }

  auto* block = get_memory_block();
  if (!block)
    return nullptr;

  auto* slot = block->free;
  block->free = slot->next;
  block->used_count++;
  return slot;
}

auto free_trampoline(void* buf) -> void {
  auto target_block_addr = ((std::uintptr_t)buf / MEMORY_BLOCK_SIZE) * MEMORY_BLOCK_SIZE;
  memory_block* prev = nullptr;
  for (auto* block = g_memory_blocks; block; block = block->next) {
    if ((std::uintptr_t)block == target_block_addr) {
      auto* slot = (memory_slot*)buf;
      slot->next = block->free;
      block->free = slot;
      block->used_count--;
      if (block->used_count == 0) {
        if (prev)
          prev->next = block->next;
        else
          g_memory_blocks = block->next;
        VirtualFree(block, 0, MEM_RELEASE);
      }
      return;
    }
    prev = block;
  }
}

// ---------------------------------------------------------------------------
// Trampoline builder (replaces MinHook's trampoline.c)
// ---------------------------------------------------------------------------

struct trampoline_info {
  void* target;
  void* detour;
  void* trampoline;
  bool patch_above;
};

auto create_trampoline(trampoline_info* ct) -> bool {
  jmp_rel jmp = {0xE9, 0};
  call_rel call = {0xE8, 0};
  jcc_rel jcc = {0x0F, 0x80, 0};

  std::uint8_t old_pos = 0;
  std::uint8_t new_pos = 0;
  std::uintptr_t jmp_dest = 0;
  bool finished = false;

  ct->patch_above = false;

  do {
    x86_insn hs;
    std::uint32_t copy_size;
    const void* copy_src;
    std::uintptr_t p_old = (std::uintptr_t)ct->target + old_pos;
    std::uintptr_t p_new = (std::uintptr_t)ct->trampoline + new_pos;

    copy_size = x86_disasm((const void*)p_old, &hs);
    if (hs.flags & F_ERROR)
      return false;

    copy_src = (const void*)p_old;

    if (old_pos >= sizeof(jmp_rel)) {
      // Trampoline is long enough; append JMP back to target
      jmp.operand = (std::uint32_t)(p_old - (p_new + sizeof(jmp)));
      copy_src = &jmp;
      copy_size = sizeof(jmp);
      finished = true;
    } else if (hs.opcode == 0xE8) {
      // Direct relative CALL
      std::uintptr_t dest = p_old + hs.len + (std::int32_t)hs.imm.imm32;
      call.operand = (std::uint32_t)(dest - (p_new + sizeof(call)));
      copy_src = &call;
      copy_size = sizeof(call);
    } else if ((hs.opcode & 0xFD) == 0xE9) {
      // Direct relative JMP (EB or E9)
      std::uintptr_t dest = p_old + hs.len;
      if (hs.opcode == 0xEB)
        dest += (std::int8_t)hs.imm.imm8;
      else
        dest += (std::int32_t)hs.imm.imm32;

      // Internal jump within the patched region
      if ((std::uintptr_t)ct->target <= dest && dest < ((std::uintptr_t)ct->target + sizeof(jmp_rel))) {
        if (jmp_dest < dest)
          jmp_dest = dest;
      } else {
        jmp.operand = (std::uint32_t)(dest - (p_new + sizeof(jmp)));
        copy_src = &jmp;
        copy_size = sizeof(jmp);
        finished = (p_old >= jmp_dest);
      }
    } else if ((hs.opcode & 0xF0) == 0x70
               || (hs.opcode & 0xFC) == 0xE0
               || (hs.opcode2 & 0xF0) == 0x80) {
      // Direct relative Jcc
      std::uintptr_t dest = p_old + hs.len;
      if ((hs.opcode & 0xF0) == 0x70 || (hs.opcode & 0xFC) == 0xE0)
        dest += (std::int8_t)hs.imm.imm8;
      else
        dest += (std::int32_t)hs.imm.imm32;

      if ((std::uintptr_t)ct->target <= dest && dest < ((std::uintptr_t)ct->target + sizeof(jmp_rel))) {
        if (jmp_dest < dest)
          jmp_dest = dest;
      } else if ((hs.opcode & 0xFC) == 0xE0) {
        // LOOPNZ/LOOPZ/LOOP/JCXZ to outside — not supported
        return false;
      } else {
        std::uint8_t cond = ((hs.opcode != 0x0F ? hs.opcode : hs.opcode2) & 0x0F);
        jcc.opcode1 = 0x80 | cond;
        jcc.operand = (std::uint32_t)(dest - (p_new + sizeof(jcc)));
        copy_src = &jcc;
        copy_size = sizeof(jcc);
      }
    } else if ((hs.opcode & 0xFE) == 0xC2) {
      // RET (C2 or C3)
      finished = (p_old >= jmp_dest);
    }

    // Can't alter instruction length inside a branch
    if (p_old < jmp_dest && copy_size != hs.len)
      return false;

    // Trampoline too large
    if ((new_pos + copy_size) > TRAMPOLINE_MAX_SIZE)
      return false;

    std::memcpy((std::uint8_t*)ct->trampoline + new_pos, copy_src, copy_size);
    new_pos += (std::uint8_t)copy_size;
    old_pos += hs.len;
  } while (!finished);

  // Check if there's enough space for the 5-byte JMP
  if (old_pos < sizeof(jmp_rel)
      && !is_code_padding((std::uint8_t*)ct->target + old_pos, sizeof(jmp_rel) - old_pos)) {
    // Check if there's enough space for a 2-byte short JMP
    if (old_pos < sizeof(jmp_rel_short)
        && !is_code_padding((std::uint8_t*)ct->target + old_pos, sizeof(jmp_rel_short) - old_pos)) {
      return false;
    }

    // Can we place the long jump above the function?
    if (!is_executable_address((std::uint8_t*)ct->target - sizeof(jmp_rel)))
      return false;

    if (!is_code_padding((std::uint8_t*)ct->target - sizeof(jmp_rel), sizeof(jmp_rel)))
      return false;

    ct->patch_above = true;
  }

  return true;
}

// ---------------------------------------------------------------------------
// Hook entry storage
// ---------------------------------------------------------------------------

struct hook_entry {
  void* target;
  void* detour;
  void* trampoline;
  std::uint8_t backup[8];
  bool patch_above;
  bool is_enabled;
};

std::vector<hook_entry> g_hook_entries;
std::mutex g_hook_mutex;

auto find_hook_entry(void* target) -> std::size_t {
  for (std::size_t i = 0; i < g_hook_entries.size(); ++i) {
    if (g_hook_entries[i].target == target)
      return i;
  }
  return SIZE_MAX;
}

auto enable_hook_ll(std::size_t pos, bool enable) -> bool {
  auto* hook = &g_hook_entries[pos];
  DWORD old_protect;
  std::size_t patch_size = sizeof(jmp_rel);
  std::uint8_t* patch_target = (std::uint8_t*)hook->target;

  if (hook->patch_above) {
    patch_target -= sizeof(jmp_rel);
    patch_size += sizeof(jmp_rel_short);
  }

  if (!VirtualProtect(patch_target, patch_size, PAGE_EXECUTE_READWRITE, &old_protect))
    return false;

  if (enable) {
    auto* jmp = (jmp_rel*)patch_target;
    jmp->opcode = 0xE9;
    jmp->operand = (std::uint32_t)((std::uint8_t*)hook->detour - (patch_target + sizeof(jmp_rel)));

    if (hook->patch_above) {
      auto* short_jmp = (jmp_rel_short*)hook->target;
      short_jmp->opcode = 0xEB;
      short_jmp->operand = (std::uint8_t)(0 - (sizeof(jmp_rel_short) + sizeof(jmp_rel)));
    }
  } else {
    if (hook->patch_above)
      std::memcpy(patch_target, hook->backup, sizeof(jmp_rel) + sizeof(jmp_rel_short));
    else
      std::memcpy(patch_target, hook->backup, sizeof(jmp_rel));
  }

  VirtualProtect(patch_target, patch_size, old_protect, &old_protect);
  //FlushInstructionCache(GetCurrentProcess(), patch_target, patch_size);

  hook->is_enabled = enable;
  return true;
}

} // namespace

// ---------------------------------------------------------------------------
// Public Detour Hooking API
// ---------------------------------------------------------------------------

auto hook_lib_init() -> bool {
  // No global initialization needed — memory blocks are allocated on demand.
  return true;
}

auto hook_lib_shutdown() -> bool {
  std::lock_guard<std::mutex> lock(g_hook_mutex);

  // Disable all active hooks
  for (std::size_t i = 0; i < g_hook_entries.size(); ++i) {
    if (g_hook_entries[i].is_enabled) {
      enable_hook_ll(i, false);
    }
  }

  // Free all trampolines
  for (auto& entry : g_hook_entries) {
    if (entry.trampoline)
      free_trampoline(entry.trampoline);
  }
  g_hook_entries.clear();

  // Free all memory blocks
  while (g_memory_blocks) {
    auto* next = g_memory_blocks->next;
    VirtualFree(g_memory_blocks, 0, MEM_RELEASE);
    g_memory_blocks = next;
  }

  return true;
}

auto apply_detour(char* original_addr, char* detour_addr, char** out_gateway) -> bool {
  if (!original_addr || !detour_addr || !out_gateway)
    return false;

  std::lock_guard<std::mutex> lock(g_hook_mutex);

  if (!is_executable_address(original_addr) || !is_executable_address(detour_addr))
    return false;

  // Already hooked?
  if (find_hook_entry(original_addr) != SIZE_MAX) {
    // Return existing trampoline
    auto& existing = g_hook_entries[find_hook_entry(original_addr)];
    *out_gateway = (char*)existing.trampoline;
    return true;
  }

  void* buf = alloc_trampoline();
  if (!buf)
    return false;

  trampoline_info ct;
  ct.target = original_addr;
  ct.detour = detour_addr;
  ct.trampoline = buf;

  if (!create_trampoline(&ct)) {
    free_trampoline(buf);
    return false;
  }

  hook_entry entry;
  entry.target = ct.target;
  entry.detour = ct.detour;
  entry.trampoline = ct.trampoline;
  entry.patch_above = ct.patch_above;
  entry.is_enabled = false;

  // Back up original bytes
  if (ct.patch_above) {
    std::memcpy(entry.backup, (std::uint8_t*)original_addr - sizeof(jmp_rel), sizeof(jmp_rel) + sizeof(jmp_rel_short));
  } else {
    std::memcpy(entry.backup, original_addr, sizeof(jmp_rel));
  }

  g_hook_entries.push_back(entry);
  std::size_t pos = g_hook_entries.size() - 1;
  *out_gateway = (char*)buf;

  // Enable the hook
  bool ok = enable_hook_ll(pos, true);

  if (!ok) {
    free_trampoline(buf);
    g_hook_entries.pop_back();
    return false;
  }

  return true;
}

auto remove_detour(char* original_addr) -> bool {
  if (!original_addr)
    return false;

  std::lock_guard<std::mutex> lock(g_hook_mutex);

  std::size_t pos = find_hook_entry(original_addr);
  if (pos == SIZE_MAX)
    return true; // Not hooked — nothing to do

  if (g_hook_entries[pos].is_enabled) {
    enable_hook_ll(pos, false);
  }

  void* tramp = g_hook_entries[pos].trampoline;
  g_hook_entries.erase(g_hook_entries.begin() + pos);
  free_trampoline(tramp);
  return true;
}

auto reapply_detour(char* original_addr) -> bool {
  if (!original_addr)
    return false;

  std::lock_guard<std::mutex> lock(g_hook_mutex);

  std::size_t pos = find_hook_entry(original_addr);
  if (pos == SIZE_MAX)
    return false;

  if (g_hook_entries[pos].is_enabled)
    return true;

  return enable_hook_ll(pos, true);
}

  // ---------------------------------------------------------------------------
  // VMT Hooking Implementation
  // ---------------------------------------------------------------------------



  auto table_hook::initialize(void** original_table) -> void {
    m_old_vmt = original_table;

    std::size_t size = 0;
    while (m_old_vmt[size] && is_code_ptr(m_old_vmt[size])) {
      ++size;
    }

    m_new_vmt = (new void*[size + 1]) + 1;
    std::memcpy(m_new_vmt - 1, m_old_vmt - 1, sizeof(void*) * (size + 1));
  }

  auto table_hook::hook_instance(void* inst) const -> void {
    auto& vtbl = *reinterpret_cast<void***>(inst);
    assert(vtbl == m_old_vmt || vtbl == m_new_vmt);
    vtbl = m_new_vmt;
  }

  auto table_hook::unhook_instance(void* inst) const -> void {
    auto& vtbl = *reinterpret_cast<void***>(inst);
    assert(vtbl == m_old_vmt || vtbl == m_new_vmt);
    vtbl = m_old_vmt;
  }

  auto table_hook::initialize_and_hook_instance(void* inst) -> bool {
    auto& vtbl = *reinterpret_cast<void***>(inst);
    const bool initialized = (m_old_vmt == nullptr);
    if (initialized) {
      initialize(vtbl);
    }
    hook_instance(inst);
    return initialized;
  }

  vmt_hook::vmt_hook(void* class_base)
    : m_class(class_base) {
    initialize_and_hook_instance(class_base);
  }

  vmt_hook::~vmt_hook() {
    unhook_instance(m_class);
  }

} // namespace ext_client::utils

