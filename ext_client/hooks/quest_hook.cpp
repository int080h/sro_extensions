#include "hooks/quest_hook.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/offsets.hpp"

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::hooks::quest {
  namespace {

    // get_quest_definition (sub_A75820)
    // Signature in IDA: int __thiscall sub_A75820(char *this, char a2)
    // a2 is a 32-bit Quest ID passed on stack
    make_hook<convention_type::thiscall_t, void*, void*, unsigned int> g_get_quest_definition;
    hook_group g_hooks;

    auto __fastcall get_quest_definition_detour(void* self, void* edx, unsigned int quest_id) -> void* {
      void* result = g_get_quest_definition.call_original(self, quest_id);
      if (!result) {
        log_msg("[quest_hook] WARNING: Quest ID %d (0x%X) not found in client DB! Returning dummy definition to prevent crash.", quest_id, quest_id);
        
        // Return a zeroed-out static dummy structure to prevent the null-pointer dereference in packet parser
        static char dummy_quest_def[1024] = {0};
        return dummy_quest_def;
      }
      return result;
    }

  } // namespace

  auto install() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }

    using namespace ext_client::offsets::quest::functions;
    if (!g_hooks.install(g_get_quest_definition, get_quest_definition, &get_quest_definition_detour, "quest_hook", "get_quest_definition")) {
      return false;
    }

    log_msg("[quest_hook] Hook installed successfully");
    return true;
  }

  auto uninstall() -> void {
    g_hooks.uninstall();
  }

  auto is_installed() -> bool {
    return g_hooks.is_installed();
  }

} // namespace ext_client::hooks::quest
