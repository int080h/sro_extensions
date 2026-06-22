#include "plugins/quest_plugin.hpp"

#include "core/core_event_manager.hpp"
#include "core/core_plugin_manager.hpp"
#include "utils/log.hpp"

using ext_client::utils::log_msg;
using namespace ext_client::core::event;

namespace ext_client::plugins::quest {

  auto handle_get_quest_definition(void* raw_ctx) -> void {
    auto* ctx = static_cast<get_quest_definition_context*>(raw_ctx);
    if (ctx && !ctx->result) {
      log_msg("[quest_plugin] WARNING: Quest ID %d (0x%X) not found in client DB! Returning dummy definition to prevent crash.",
              ctx->quest_id, ctx->quest_id);

      // Return a zeroed-out static dummy structure to prevent the null-pointer dereference in packet parser.
      static constexpr std::size_t k_dummy_quest_def_size = 2048;
      static char dummy_quest_def[k_dummy_quest_def_size] = {0};
      ctx->result = dummy_quest_def;
    }
  }

  auto initialize() -> void {
    REGISTER_PLUGIN("quest", "Quest Helper", "Injects custom quest definitions and handles fallback logic for missing quests to prevent client crashes.");

    ADD_PLUGIN_EVENT("quest", event_type::on_get_quest_definition, handle_get_quest_definition);
  }

  PLUGIN_INIT(initialize);

} // namespace ext_client::plugins::quest
