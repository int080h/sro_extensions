#include "utils/hooks.hpp"

#include <MinHook.h>
#include <windows.h>
#include <cstring>

namespace {

  auto mh_ok(MH_STATUS status) -> bool {
    return status == MH_OK || status == MH_ERROR_ALREADY_INITIALIZED || status == MH_ERROR_ALREADY_CREATED || status == MH_ERROR_ENABLED ||
           status == MH_ERROR_DISABLED;
  }

} // namespace

namespace ext_client::utils {

  // ---------------------------------------------------------------------------
  // Detour Hooking Implementation
  // ---------------------------------------------------------------------------

  auto hook_lib_init() -> bool {
    return mh_ok(MH_Initialize());
  }

  auto hook_lib_shutdown() -> bool {
    return mh_ok(MH_Uninitialize());
  }

  auto apply_detour(char* original_addr, char* detour_addr, char** out_gateway) -> bool {
    if (!original_addr || !detour_addr || !out_gateway) {
      return false;
    }

    void* trampoline = nullptr;
    MH_STATUS status = MH_CreateHook(original_addr, detour_addr, &trampoline);
    if (status != MH_OK && status != MH_ERROR_ALREADY_CREATED) {
      return false;
    }

    if (status == MH_OK) {
      *out_gateway = static_cast<char*>(trampoline);
    }

    status = MH_EnableHook(original_addr);
    return mh_ok(status);
  }

  auto remove_detour(char* original_addr) -> bool {
    if (!original_addr) {
      return false;
    }
    return mh_ok(MH_DisableHook(original_addr));
  }

  auto reapply_detour(char* original_addr) -> bool {
    if (!original_addr) {
      return false;
    }
    return mh_ok(MH_EnableHook(original_addr));
  }

  // ---------------------------------------------------------------------------
  // VMT Hooking Implementation
  // ---------------------------------------------------------------------------

  auto is_code_ptr(void* ptr) -> bool {
    constexpr DWORD protect_flags = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

    MEMORY_BASIC_INFORMATION out{};
    if (!VirtualQuery(ptr, &out, sizeof(out))) {
      return false;
    }

    return out.Type != 0 && !(out.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && (out.Protect & protect_flags);
  }

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

