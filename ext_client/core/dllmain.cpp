#include "core/ext_client.hpp"

#include <windows.h>

namespace {
  DWORD WINAPI init_thread_proc(LPVOID param) {
    ext_client::core::get().run(static_cast<HMODULE>(param));
    return 0;
  }
} // namespace

auto WINAPI DllMain(HMODULE h_module, DWORD reason, LPVOID) -> BOOL {
  switch (reason) {
    case DLL_PROCESS_ATTACH:
      ext_client::core::get().set_main_thread_id(GetCurrentThreadId());
      DisableThreadLibraryCalls(h_module);
      CreateThread(nullptr, 0, init_thread_proc, h_module, 0, nullptr);
      break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}
