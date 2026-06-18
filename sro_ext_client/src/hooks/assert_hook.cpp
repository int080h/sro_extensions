#include "hooks/assert_hook.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/offsets.hpp"
#include <cstring>

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::hooks::assertion {
  namespace {

    make_hook<convention_type::cdecl_t, bool, int, const char*, const char*, __int16> g_assert_report;
    hook_group g_hooks;

    auto __cdecl assert_report_detour(int line, const char* file, const char* msg, __int16 flags) -> bool {
      if (file && std::strstr(file, "MsgStreamBuffer.h") != nullptr) {
        log_msg("[assert_hook] MsgStreamBuffer read overflow bypassed: line=%d, msg=\"%s\"", line, msg ? msg : "nullptr");
        return true; // Return true to prevent BSLib's DebugBreak/ExitProcess crash
      }
      return g_assert_report.call_original(line, file, msg, flags);
    }

  } // namespace

  auto install() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }
    using namespace ext_client::offsets::bslib::functions;
    if (!g_hooks.install(g_assert_report, assert_report, &assert_report_detour, "assert_hook", "assert_report")) {
      return false;
    }
    log_msg("[assert_hook] Hook installed successfully");
    return true;
  }

  auto uninstall() -> void {
    g_hooks.uninstall();
  }

  auto is_installed() -> bool {
    return g_hooks.is_installed();
  }
} // namespace ext_client::hooks::assertion
