#include "pch.hpp"
#include "core/hooks/engine_hooks.hpp"
#include "core/config.hpp"
#include "core/event_bus.hpp"
#include "utils/hooks.hpp"
#include "utils/offsets.hpp"
#include "sdk/render/cgfx_video3d.hpp"
#include <d3d9.h>
#include <intrin.h>

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;
using namespace ext_client::core::event;

namespace ext_client::core::hooks::d3d_hooks {
  namespace {
    hook_group g_hooks;

    auto log_present_params(const char *prefix, const D3DPRESENT_PARAMETERS *params) -> void {
      if (!params) {
        log_msg("%s: <null>", prefix);
        return;
      }
      RECT rc_wnd{}, rc_client{};
      const bool valid_wnd = params->hDeviceWindow && IsWindow(params->hDeviceWindow);
      if (valid_wnd) {
        GetWindowRect(params->hDeviceWindow, &rc_wnd);
        GetClientRect(params->hDeviceWindow, &rc_client);
      }
      log_msg("%s: [Size=%ux%u] [Fmt=%d] [Count=%u] [MS: type=%d qual=%lu] [Swap=%d] "
              "[Hwnd=%p (valid=%d vis=%d icon=%d wnd=%ldx%ld client=%ldx%ld)] "
              "[Windowed=%d] [DS: enable=%d fmt=%d] [Flags=0x%08X] [Hz=%u] [Interval=%u]",
              prefix,
              params->BackBufferWidth, params->BackBufferHeight,
              params->BackBufferFormat, params->BackBufferCount,
              params->MultiSampleType, params->MultiSampleQuality,
              params->SwapEffect,
              params->hDeviceWindow,
              valid_wnd ? 1 : 0,
              valid_wnd ? IsWindowVisible(params->hDeviceWindow) : 0,
              valid_wnd ? IsIconic(params->hDeviceWindow) : 0,
              rc_wnd.right - rc_wnd.left, rc_wnd.bottom - rc_wnd.top,
              rc_client.right - rc_client.left, rc_client.bottom - rc_client.top,
              params->Windowed,
              params->EnableAutoDepthStencil, params->AutoDepthStencilFormat,
              params->Flags,
              params->FullScreen_RefreshRateInHz,
              params->PresentationInterval);
    }

    auto apply_presentation_params(D3DPRESENT_PARAMETERS *params) -> void {
      if (!params) {
        return;
      }
      const auto settings = ext_client::core::config::runtime();
      const auto &cfg = settings->graphic;
      if (cfg.d3d_triple_buffering && params->Windowed) {
        log_msg("[core_hooks] Forcing triple buffering (BackBufferCount = 2).");
        params->BackBufferCount = 2;
      }
      if (cfg.d3d_discard_depth_stencil) {
        log_msg("[core_hooks] Forcing D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL.");
        params->Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
      }
    }

    auto apply_behavior_flags(IDirect3D9 *d3d, UINT adapter, D3DDEVTYPE device_type, DWORD &behavior_flags) -> void {
      const auto settings = ext_client::core::config::runtime();
      const auto &cfg = settings->graphic;
      if (!cfg.d3d_force_hardware_vp) {
        return;
      }

      D3DCAPS9 caps{};
      if (FAILED(d3d->GetDeviceCaps(adapter, device_type, &caps))) {
        log_msg("[core_hooks] Failed to query device capabilities.");
        return;
      }

      if (!(caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)) {
        log_msg("[core_hooks] GPU does not support Hardware T&L. Software VP will be used.");
        return;
      }

      log_msg("[core_hooks] GPU supports Hardware T&L. Forcing Hardware VP.");
      behavior_flags &= ~D3DCREATE_SOFTWARE_VERTEXPROCESSING;
      behavior_flags &= ~D3DCREATE_MIXED_VERTEXPROCESSING;
      behavior_flags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;

      if (cfg.d3d_force_pure_device) {
        if (caps.DevCaps & D3DDEVCAPS_PUREDEVICE) {
          log_msg("[core_hooks] GPU supports Pure Device. Forcing Pure Device.");
          behavior_flags |= D3DCREATE_PUREDEVICE;
        } else {
          log_msg("[core_hooks] Pure Device requested but not supported by GPU.");
        }
      }
    }

    auto hook_create_device(IDirect3D9 *d3d) -> void;
    auto hook_device_methods(IDirect3DDevice9 *device) -> void;

    make_hook<convention_type::stdcall_t, IDirect3D9 *, UINT> g_direct3d_create9;
    make_hook<convention_type::stdcall_t, HRESULT, IDirect3D9 *, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS *,
              IDirect3DDevice9 **>
        g_create_device;
    make_hook<convention_type::stdcall_t, HRESULT, IDirect3DDevice9 *> g_end_scene;
    make_hook<convention_type::stdcall_t, HRESULT, IDirect3DDevice9 *, D3DPRESENT_PARAMETERS *> g_reset;

    auto WINAPI direct3d_create9_detour(UINT SDKVersion) -> IDirect3D9 * {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[core_hooks] Direct3DCreate9 called (SDKVersion=0x%08X Caller=%p TID=%u)",
              SDKVersion, _ReturnAddress(), GetCurrentThreadId());
      IDirect3D9 *d3d = g_direct3d_create9.call_original(SDKVersion);
      if (d3d) {
        log_msg("[core_hooks] Direct3DCreate9 returned d3d=%p", d3d);
        hook_create_device(d3d);
      } else {
        log_msg("[core_hooks] Direct3DCreate9 returned NULL!");
      }
      return d3d;
    }

    auto __stdcall create_device_detour(IDirect3D9 *self, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
                                        DWORD BehaviorFlags, D3DPRESENT_PARAMETERS *pPresentationParameters,
                                        IDirect3DDevice9 **ppReturnedDeviceInterface) -> HRESULT {
      ext_client::utils::hook_call_scope active_call;
      log_msg("[core_hooks] CreateDevice called (self=%p Adapter=%u DeviceType=%d hFocusWindow=%p Behavior=0x%08X TID=%u Caller=%p)",
              self, Adapter, DeviceType, hFocusWindow, BehaviorFlags, GetCurrentThreadId(), _ReturnAddress());
      log_present_params("[core_hooks] CreateDevice params", pPresentationParameters);

      apply_behavior_flags(self, Adapter, DeviceType, BehaviorFlags);
      apply_presentation_params(pPresentationParameters);

      HRESULT hr = g_create_device.call_original(self, Adapter, DeviceType, hFocusWindow, BehaviorFlags,
                                                 pPresentationParameters, ppReturnedDeviceInterface);
      log_msg("[core_hooks] CreateDevice returned hr=0x%08X (device=%p)", hr,
              (ppReturnedDeviceInterface && *ppReturnedDeviceInterface) ? *ppReturnedDeviceInterface : nullptr);

      if (SUCCEEDED(hr) && ppReturnedDeviceInterface && *ppReturnedDeviceInterface) {
        hook_device_methods(*ppReturnedDeviceInterface);

        d3d_device_created_context ctx{*ppReturnedDeviceInterface};
        TRIGGER_EVENT(EVENT_ON_D3D_DEVICE_CREATED, ctx);
      }
      log_msg("[core_hooks] create_device_detour exiting with hr=0x%08X", hr);
      return hr;
    }

    auto __stdcall end_scene_detour(IDirect3DDevice9 *device) -> HRESULT {
      ext_client::utils::hook_call_scope active_call;
      static bool s_logged_first_end_scene = false;
      if (!s_logged_first_end_scene) {
        s_logged_first_end_scene = true;
        log_msg("[core_hooks] first EndScene reached (device=%p TID=%u)", device, GetCurrentThreadId());
      }
      const HRESULT hr = g_end_scene.call_original(device);
      d3d_end_scene_context ctx{device};
      TRIGGER_EVENT(EVENT_ON_D3D_END_SCENE, ctx);
      return hr;
    }

    auto __stdcall reset_detour(IDirect3DDevice9 *device, D3DPRESENT_PARAMETERS *params) -> HRESULT {
      ext_client::utils::hook_call_scope active_call;
      const void *caller = _ReturnAddress();
      const HRESULT coop_before = device ? device->TestCooperativeLevel() : E_FAIL;
      log_msg("[core_hooks] reset_detour entered (device=%p params=%p Caller=%p TID=%u CoopLevelBefore=0x%08X)",
              device, params, caller, GetCurrentThreadId(), coop_before);
      log_present_params("[core_hooks] reset params", params);

      if (caller == reinterpret_cast<const void *>(0x00D7915D)) {
        log_msg("[core_hooks] Reset called from crash shutdown cleanup (sub_D79130 / TopLevelExceptionFilter).");
      }

      apply_presentation_params(params);

      log_msg("[core_hooks] triggering EVENT_ON_D3D_PRE_RESET");
      TRIGGER_EVENT(EVENT_ON_D3D_PRE_RESET);

      log_msg("[core_hooks] calling original Reset");
      const HRESULT hr = g_reset.call_original(device, params);
      const HRESULT coop_after = device ? device->TestCooperativeLevel() : E_FAIL;
      log_msg("[core_hooks] original Reset returned hr=0x%08X (CoopLevelAfter=0x%08X)", hr, coop_after);

      if (SUCCEEDED(hr)) {
        log_msg("[core_hooks] triggering EVENT_ON_D3D_POST_RESET");
        TRIGGER_EVENT(EVENT_ON_D3D_POST_RESET);
      } else {
        log_msg("[core_hooks] Reset FAILED with hr=0x%08X!", hr);
      }
      return hr;
    }

    auto hook_create_device(IDirect3D9 *d3d) -> void {
      if (g_create_device.is_applied()) {
        return;
      }
      const auto vtable = reinterpret_cast<std::uintptr_t>(*reinterpret_cast<void ***>(d3d));
      const auto create_device_addr = ext_client::off::vtable_slot(vtable, 16);
      if (g_hooks.install(g_create_device, create_device_addr, &create_device_detour, "core_hooks", "CreateDevice")) {
        log_msg("[core_hooks] hooked CreateDevice @ 0x%08X", create_device_addr);
      }
    }

    auto hook_device_methods(IDirect3DDevice9 *device) -> void {
      if (g_end_scene.is_applied() && g_reset.is_applied()) {
        return;
      }
      const auto vtable = reinterpret_cast<std::uintptr_t>(*reinterpret_cast<void ***>(device));
      const auto end_scene_addr = ext_client::off::vtable_slot(vtable, 42);
      const auto reset_addr = ext_client::off::vtable_slot(vtable, 16);

      if (g_hooks.install(g_end_scene, end_scene_addr, &end_scene_detour, "core_hooks", "EndScene") &&
          g_hooks.install(g_reset, reset_addr, &reset_detour, "core_hooks", "Reset")) {
        log_msg("[core_hooks] hooked EndScene @ 0x%08X and Reset @ 0x%08X", end_scene_addr, reset_addr);
      }
    }
  } // namespace

  auto install_lazy() -> void {
    std::lock_guard lock(ext_client::utils::hook_lifecycle_mutex());
    if (ext_client::utils::hook_stopping())
      return;
    HMODULE h_d3d9 = GetModuleHandleA("d3d9.dll");
    if (!h_d3d9) {
      return;
    }

    auto d3d_create9_fn = reinterpret_cast<std::uintptr_t>(GetProcAddress(h_d3d9, "Direct3DCreate9"));
    if (d3d_create9_fn && !g_direct3d_create9.is_applied()) {
      if (g_hooks.install(g_direct3d_create9, d3d_create9_fn, &direct3d_create9_detour, "core_hooks",
                          "Direct3DCreate9")) {
        log_msg("[core_hooks] hooked Direct3DCreate9 @ 0x%08X", d3d_create9_fn);
      }
    }

    if (auto *app = cgfx_video3d::get()) {
      if (auto *device = app->get_device()) {
        if (!g_end_scene.is_applied() || !g_reset.is_applied()) {
          log_msg("[core_hooks] active device already exists, hooking device methods immediately");
          hook_device_methods(device);
        }
      }
    }
  }

  auto uninstall() -> bool {
    return g_hooks.uninstall();
  }
  auto is_installed() -> bool {
    return g_end_scene.is_applied() && g_reset.is_applied();
  }
} // namespace ext_client::core::hooks::d3d_hooks
