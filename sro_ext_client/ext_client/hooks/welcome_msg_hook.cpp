#include "hooks/welcome_msg_hook.hpp"
#include "config/client_config.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"
#include <Windows.h>
#include <string>

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::hooks::welcome_msg {
  namespace {

    make_hook<convention_type::thiscall_t, int, void*, const void*> g_show_notice;
    hook_group g_hooks;

    auto __fastcall show_notice_detour(void* self, void*, const void* msg_obj) -> int {
      if (!msg_obj) {
        log_msg("[welcome_msg_hook] WARNING: msg_obj is null");
        return g_show_notice.call_original(self, msg_obj);
      }

      auto ref = ext_client::msvc9::wstring_ref::from(msg_obj);
      std::wstring std_msg(ref.data(), ref.length());

      char utf8_msg[512]{};
      WideCharToMultiByte(CP_UTF8, 0, std_msg.c_str(), -1, utf8_msg, sizeof(utf8_msg), nullptr, nullptr);
      log_msg("[welcome_msg_hook] Received notice: len=%d, text=\"%s\"", ref.length(), utf8_msg);

      // Check if it's the welcome back message
      if (std_msg.find(L"Welcome back, we hope you enjoy your stay") != std::wstring::npos) {
        const auto& cfg = ext_client::config::data().welcome_msg;
        if (!cfg.enabled) {
          log_msg("[welcome_msg_hook] Blocked display of welcome message");
          return 0;
        }

        // Apply customized welcome message
        wchar_t wide_cfg_text[256]{};
        MultiByteToWideChar(CP_UTF8, 0, cfg.text, -1, wide_cfg_text, 256);

        ext_client::msvc9::wstring custom_msg(wide_cfg_text);
        log_msg("[welcome_msg_hook] Replacing welcome message with custom text: \"%s\"", cfg.text);
        return g_show_notice.call_original(self, custom_msg.raw());
      }

      return g_show_notice.call_original(self, msg_obj);
    }

  } // namespace

  auto install() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }
    using namespace ext_client::offsets::cif_notify::functions;
    if (!g_hooks.install(g_show_notice, show_notice, &show_notice_detour, "welcome_msg_hook", "show_notice")) {
      return false;
    }
    log_msg("[welcome_msg_hook] Hook installed successfully");
    return true;
  }

  auto uninstall() -> void {
    g_hooks.uninstall();
  }

  auto is_installed() -> bool {
    return g_hooks.is_installed();
  }

} // namespace ext_client::hooks::welcome_msg
