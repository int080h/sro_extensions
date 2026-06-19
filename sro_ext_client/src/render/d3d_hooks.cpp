#include "render/d3d_hooks.hpp"

#include "utils/client_config.hpp"
#include "ext_client.hpp"
#include "sdk/cgfx_video3d.hpp"
#include "utils/hooks.hpp"
#include "utils/log.hpp"
#include "utils/offsets.hpp"

#include <d3d9.h>

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

namespace ext_client::render {

  auto resolve_hwnd(cgfx_video3d* app) -> HWND {
    if (!app) {
      return nullptr;
    }
    if (HWND hwnd = app->hwnd()) {
      return hwnd;
    }
    if (HWND hwnd = app->hwnd_device()) {
      return hwnd;
    }
    return FindWindowA(nullptr, "SRO_CLIENT");
  }

  auto apply_presentation_params(D3DPRESENT_PARAMETERS* params) -> void {
    if (!params) {
      return;
    }
    const auto& cfg = ext_client::config::data().graphic;
    if (cfg.d3d_triple_buffering && params->Windowed) {
      log_msg("[d3d_hooks] Forcing triple buffering (BackBufferCount = 2).");
      params->BackBufferCount = 2;
    }
    if (cfg.d3d_discard_depth_stencil) {
      log_msg("[d3d_hooks] Forcing D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL.");
      params->Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
    }
  }

  auto apply_behavior_flags(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE device_type, DWORD& behavior_flags) -> void {
    const auto& cfg = ext_client::config::data().graphic;
    if (!cfg.d3d_force_hardware_vp) {
      return;
    }

    D3DCAPS9 caps{};
    if (FAILED(d3d->GetDeviceCaps(adapter, device_type, &caps))) {
      log_msg("[d3d_hooks] Failed to query device capabilities.");
      return;
    }

    if (!(caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)) {
      log_msg("[d3d_hooks] GPU does not support Hardware T&L. Software VP will be used.");
      return;
    }

    log_msg("[d3d_hooks] GPU supports Hardware T&L. Forcing Hardware VP.");
    behavior_flags &= ~D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    behavior_flags &= ~D3DCREATE_MIXED_VERTEXPROCESSING;
    behavior_flags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;

    if (cfg.d3d_force_pure_device) {
      if (caps.DevCaps & D3DDEVCAPS_PUREDEVICE) {
        log_msg("[d3d_hooks] GPU supports Pure Device. Forcing Pure Device.");
        behavior_flags |= D3DCREATE_PUREDEVICE;
      } else {
        log_msg("[d3d_hooks] Pure Device requested but not supported by GPU.");
      }
    }
  }

  namespace {
    constexpr std::size_t k_end_scene_slot = 42;
    constexpr std::size_t k_reset_slot = 16;

    make_hook<convention_type::stdcall_t, IDirect3D9*, UINT> g_direct3d_create9;
    make_hook<convention_type::stdcall_t, HRESULT, IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**> g_create_device;
    make_hook<convention_type::stdcall_t, HRESULT, IDirect3DDevice9*> g_end_scene;
    make_hook<convention_type::stdcall_t, HRESULT, IDirect3DDevice9*, D3DPRESENT_PARAMETERS*> g_reset;
    hook_group g_hooks;

    device_callbacks g_callbacks;

    auto WINAPI direct3d_create9_detour(UINT SDKVersion) -> IDirect3D9*;
    auto __stdcall create_device_detour(
      IDirect3D9* self,
      UINT Adapter,
      D3DDEVTYPE DeviceType,
      HWND hFocusWindow,
      DWORD BehaviorFlags,
      D3DPRESENT_PARAMETERS* pPresentationParameters,
      IDirect3DDevice9** ppReturnedDeviceInterface
    ) -> HRESULT;
    auto __stdcall end_scene_detour(IDirect3DDevice9* device) -> HRESULT;
    auto __stdcall reset_detour(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) -> HRESULT;
    auto hook_create_device(IDirect3D9* d3d) -> void;
    auto hook_device_methods(IDirect3DDevice9* device) -> void;

    auto WINAPI direct3d_create9_detour(UINT SDKVersion) -> IDirect3D9* {
      IDirect3D9* d3d = g_direct3d_create9.call_original(SDKVersion);
      if (d3d) {
        log_msg("[d3d_hooks] Direct3DCreate9 intercepted (SDKVersion=%u)", SDKVersion);
        hook_create_device(d3d);
      }
      return d3d;
    }

    auto __stdcall create_device_detour(
      IDirect3D9* self,
      UINT Adapter,
      D3DDEVTYPE DeviceType,
      HWND hFocusWindow,
      DWORD BehaviorFlags,
      D3DPRESENT_PARAMETERS* pPresentationParameters,
      IDirect3DDevice9** ppReturnedDeviceInterface
    ) -> HRESULT {
      log_msg("[d3d_hooks] CreateDevice: Adapter=%u, DeviceType=%d, BehaviorFlags=0x%X",
              Adapter, DeviceType, BehaviorFlags);

      apply_behavior_flags(self, Adapter, DeviceType, BehaviorFlags);
      apply_presentation_params(pPresentationParameters);

      HRESULT hr = g_create_device.call_original(self, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
      if (SUCCEEDED(hr) && ppReturnedDeviceInterface && *ppReturnedDeviceInterface) {
        log_msg("[d3d_hooks] Device successfully created @ %p", *ppReturnedDeviceInterface);
        hook_device_methods(*ppReturnedDeviceInterface);
      } else {
        log_msg("[d3d_hooks] CreateDevice failed (hr=0x%08X)", hr);
      }
      return hr;
    }

    auto __stdcall end_scene_detour(IDirect3DDevice9* device) -> HRESULT {
      const HRESULT hr = g_end_scene.call_original(device);
      if (g_callbacks.on_end_scene) {
        g_callbacks.on_end_scene(device);
      }
      return hr;
    }

    auto __stdcall reset_detour(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) -> HRESULT {
      apply_presentation_params(params);

      if (g_callbacks.on_pre_reset) {
        g_callbacks.on_pre_reset();
      }
      const HRESULT hr = g_reset.call_original(device, params);
      if (SUCCEEDED(hr)) {
        if (g_callbacks.on_post_reset) {
          g_callbacks.on_post_reset();
        }
      }
      return hr;
    }

    auto hook_create_device(IDirect3D9* d3d) -> void {
      if (g_create_device.is_applied()) {
        return;
      }
      const auto vtable = reinterpret_cast<std::uintptr_t>(*reinterpret_cast<void***>(d3d));
      const auto create_device_addr = ext_client::offsets::vtable_slot(vtable, 16);
      if (g_hooks.install(g_create_device, create_device_addr, &create_device_detour, "d3d_hooks", "CreateDevice")) {
        log_msg("[d3d_hooks] hooked CreateDevice @ 0x%08X", create_device_addr);
      }
    }

    auto hook_device_methods(IDirect3DDevice9* device) -> void {
      if (g_end_scene.is_applied() || g_reset.is_applied()) {
        return;
      }
      const auto vtable = reinterpret_cast<std::uintptr_t>(*reinterpret_cast<void***>(device));
      const auto end_scene_addr = ext_client::offsets::vtable_slot(vtable, k_end_scene_slot);
      const auto reset_addr = ext_client::offsets::vtable_slot(vtable, k_reset_slot);

      if (g_hooks.install(g_end_scene, end_scene_addr, &end_scene_detour, "d3d_hooks", "EndScene") &&
          g_hooks.install(g_reset, reset_addr, &reset_detour, "d3d_hooks", "Reset")) {
        log_msg("[d3d_hooks] hooked EndScene @ 0x%08X and Reset @ 0x%08X", end_scene_addr, reset_addr);
        if (g_callbacks.on_device_created) {
          g_callbacks.on_device_created(device);
        }
      }
    }

  } // namespace

  d3d_hooks::~d3d_hooks() {
    uninstall();
  }

  auto d3d_hooks::install(const device_callbacks& callbacks) -> bool {
    if (m_installed) {
      return true;
    }

    g_callbacks = callbacks;

    HMODULE h_d3d9 = GetModuleHandleA("d3d9.dll");
    if (!h_d3d9) {
      return false;
    }

    auto d3d_create9_fn = reinterpret_cast<std::uintptr_t>(GetProcAddress(h_d3d9, "Direct3DCreate9"));
    if (d3d_create9_fn && !g_direct3d_create9.is_applied()) {
      if (g_hooks.install(g_direct3d_create9, d3d_create9_fn, &direct3d_create9_detour, "d3d_hooks", "Direct3DCreate9")) {
        log_msg("[d3d_hooks] hooked Direct3DCreate9 @ 0x%08X", d3d_create9_fn);
      }
    }

    if (auto* app = cgfx_video3d::get()) {
      if (auto* device = app->device()) {
        log_msg("[d3d_hooks] active device already exists, hooking device methods immediately");
        hook_device_methods(device);
      }
    }

    m_installed = g_end_scene.is_applied() && g_reset.is_applied();
    return m_installed;
  }

  auto d3d_hooks::uninstall() -> void {
    g_hooks.uninstall();
    g_callbacks = {};
    m_installed = false;
    log_msg("[d3d_hooks] unhooked");
  }

  auto d3d_hooks::is_installed() const -> bool {
    return g_end_scene.is_applied() && g_reset.is_applied();
  }

} // namespace ext_client::render
