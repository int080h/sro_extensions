#include "hooks/intro_hook.hpp"

#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::hooks::intro {
  namespace {

    namespace off = ext_client::offsets::cps_version_check;
    namespace intro_renderer = ext_client::offsets::cps_version_check::intro_renderer;

    enum class intro_render_stage : int {
      d3d_setup = 1,
      wire_stages = 2,
    };

    struct intro_renderer_state {
      void* vftable;
    };

    bool g_intro_render_pipeline_ready = false;

    make_hook<convention_type::cdecl_t, int, int, int, char> g_load_intro_camera;

    hook_group g_hooks;

    auto intro_render_stage_callback_address() -> int(__cdecl*)(int) {
      return reinterpret_cast<int(__cdecl*)(int)>(off::functions::intro_render_stage_callback);
    }

    auto invoke_stage_callback(intro_render_stage stage) -> int {
      const auto callback = intro_render_stage_callback_address();
      if (!callback) {
        return 0;
      }
      return callback(static_cast<int>(stage));
    }

    auto intro_renderer_instance() -> intro_renderer_state* {
      using get_intro_renderer_fn = intro_renderer_state*(__cdecl*)();
      const auto fn = ext_client::offsets::as_fn<get_intro_renderer_fn>(off::functions::get_intro_renderer);
      return fn();
    }

    auto register_intro_stage_callback(intro_renderer_state* renderer, int(__cdecl* callback)(int)) -> bool {
      if (!callback || !ext_client::msvc9::is_readable_ptr(renderer, sizeof(void*))) {
        return false;
      }

      auto** vtable = *reinterpret_cast<void***>(renderer);
      const auto vtbl_index = intro_renderer::vtbl_register_stage_callback / sizeof(void*);
      if (!vtable || !ext_client::msvc9::is_readable_ptr(vtable, intro_renderer::vtbl_register_stage_callback + sizeof(void*))) {
        return false;
      }

      const auto register_fn_addr = vtable[vtbl_index];
      if (!register_fn_addr || !ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(register_fn_addr), 1)) {
        return false;
      }

      using register_stage_callback_fn = void(__thiscall*)(intro_renderer_state*, int(__cdecl*)(int));
      reinterpret_cast<register_stage_callback_fn>(register_fn_addr)(renderer, callback);
      return true;
    }

    auto setup_login_render_pipeline() -> bool {
      if (g_intro_render_pipeline_ready) {
        return true;
      }

      auto* renderer = intro_renderer_instance();
      if (!renderer) {
        return false;
      }

      if (!register_intro_stage_callback(renderer, intro_render_stage_callback_address())) {
        return false;
      }

      invoke_stage_callback(intro_render_stage::d3d_setup);
      invoke_stage_callback(intro_render_stage::wire_stages);

      g_intro_render_pipeline_ready = true;
      return true;
    }

    auto __cdecl load_intro_camera_detour(int camera_state, int pack, char run_scripts) -> int {
      if (!setup_login_render_pipeline()) {
        log_msg("[intro_hook] intro render pipeline setup failed");
      }

      return g_load_intro_camera.call_original(camera_state, pack, run_scripts);
    }

  } // namespace

  auto install() -> bool {
    if (g_hooks.is_installed()) {
      return true;
    }

    using namespace off::functions;

    if (!g_hooks.install(g_load_intro_camera, load_intro_camera, &load_intro_camera_detour, "intro_hook", "load_intro_camera")) {
      return false;
    }

    log_msg("[intro_hook] installed");
    return true;
  }

  auto uninstall() -> void {
    g_hooks.uninstall();
    g_intro_render_pipeline_ready = false;
  }

  auto is_installed() -> bool {
    return g_hooks.is_installed();
  }

} // namespace ext_client::hooks::intro
