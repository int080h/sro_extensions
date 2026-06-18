#include "ext_client.hpp"

#include <windows.h>

namespace {
  HANDLE g_init_thread = nullptr;

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
      g_init_thread = CreateThread(nullptr, 0, init_thread_proc, h_module, 0, nullptr);
      if (!g_init_thread) {
        OutputDebugStringA("[ext_client] failed to create init thread\n");
      }
      break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      ext_client::core::get().request_unload();
      if (g_init_thread) {
        WaitForSingleObject(g_init_thread, 3000);
        CloseHandle(g_init_thread);
        g_init_thread = nullptr;
      }
      if (ext_client::core::get().is_running()) {
        ext_client::core::get().shutdown(ext_client::shutdown_mode::graceful_unload);
      }
      break;
  }
  return TRUE;
}
