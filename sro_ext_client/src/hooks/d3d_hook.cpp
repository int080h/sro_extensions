#include "ext_client.hpp"

#include "hooks/d3d_hook.hpp"

#include "utils/hooks.hpp"
#include "features/combat_overlay/combat_overlay.hpp"
#include "features/combat_overlay/target_window_hp.hpp"
#include "menu/menu.hpp"
#include "sdk/cgfx_video3d.hpp"
#include "utils/log.hpp"
#include "utils/shutdown_guard.hpp"
#include "utils/window_style.hpp"

#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

#include <d3d9.h>
#include <mutex>

using ext_client::utils::convention_type;
using ext_client::utils::hook_group;
using ext_client::utils::log_msg;
using ext_client::utils::make_hook;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

namespace {
  constexpr std::size_t k_end_scene_slot = 42;
  constexpr std::size_t k_reset_slot = 16;

  WNDPROC original_wndproc = nullptr;
  HWND hooked_hwnd = nullptr;
  bool imgui_ready = false;
  std::once_flag imgui_init_once;
  bool logged_waiting = false;

  make_hook<convention_type::stdcall_t, IDirect3D9*, UINT> g_direct3d_create9;
  make_hook<convention_type::stdcall_t, HRESULT, IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**> g_create_device;
  make_hook<convention_type::stdcall_t, HRESULT, IDirect3DDevice9*> g_end_scene;
  make_hook<convention_type::stdcall_t, HRESULT, IDirect3DDevice9*, D3DPRESENT_PARAMETERS*> g_reset;
  hook_group g_hooks;

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
  auto CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) -> LRESULT;
  auto hook_create_device(IDirect3D9* d3d) -> void;
  auto hook_device_methods(IDirect3DDevice9* device) -> void;

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

  auto hook_wnd_proc(HWND hwnd) -> void {
    if (!hwnd || !IsWindow(hwnd) || hooked_hwnd) {
      return;
    }

    ext_client::utils::window_style::ensure_minimize_button(hwnd);
    original_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&wnd_proc)));
    hooked_hwnd = hwnd;
  }

  auto unhook_wnd_proc() -> void {
    if (!hooked_hwnd || !original_wndproc) {
      return;
    }

    SetWindowLongPtrA(hooked_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original_wndproc));
    hooked_hwnd = nullptr;
    original_wndproc = nullptr;
  }

  auto init_imgui(IDirect3DDevice9* device) -> void {
    if (imgui_ready || !device) {
      return;
    }

    auto* app = cgfx_video3d::get();
    const HWND hwnd = resolve_hwnd(app);
    if (!hwnd) {
      return;
    }

    ext_client::menu::on_create(hwnd, device);
    hook_wnd_proc(hwnd);
    imgui_ready = true;
    log_msg("[d3d_hook] imgui ready (device=%p hwnd=%p)", device, hwnd);
  }

  auto rebind_imgui_hwnd() -> void {
    if (!imgui_ready) {
      return;
    }

    auto* app = cgfx_video3d::get();
    const HWND hwnd = resolve_hwnd(app);
    if (!hwnd || hwnd == hooked_hwnd) {
      return;
    }

    const HWND old_hwnd = hooked_hwnd;
    ImGui_ImplWin32_Shutdown();
    unhook_wnd_proc();
    ImGui_ImplWin32_Init(hwnd);
    hook_wnd_proc(hwnd);
    log_msg("[d3d_hook] rebound imgui hwnd %p -> %p", old_hwnd, hwnd);
  }

  auto render_frame() -> void {
    rebind_imgui_hwnd();
    ext_client::core::get().tick();
    ext_client::combat_overlay::tick();

    if (!imgui_ready) {
      return;
    }

    if (ext_client::menu::is_visible()) {
      ext_client::menu::render();
    } else {
      const auto& combat_cfg = ext_client::config::data().combat_overlay;
      if (combat_cfg.enabled && combat_cfg.show_hp_percent) {
        ext_client::menu::render_external(ext_client::target_window_hp::render_overlay);
      } else if (ext_client::combat_overlay::wants_render()) {
        ext_client::menu::render_external(ext_client::combat_overlay::render_imgui);
      }
    }
  }

  auto is_mouse_message(UINT msg) -> bool {
    switch (msg) {
      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_LBUTTONDBLCLK:
      case WM_RBUTTONDOWN:
      case WM_RBUTTONUP:
      case WM_RBUTTONDBLCLK:
      case WM_MBUTTONDOWN:
      case WM_MBUTTONUP:
      case WM_MBUTTONDBLCLK:
      case WM_XBUTTONDOWN:
      case WM_XBUTTONUP:
      case WM_MOUSEWHEEL:
      case WM_MOUSEHWHEEL:
      case WM_MOUSEMOVE:
        return true;
      default:
        return false;
    }
  }

  auto is_keyboard_message(UINT msg) -> bool {
    switch (msg) {
      case WM_KEYDOWN:
      case WM_KEYUP:
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_CHAR:
        return true;
      default:
        return false;
    }
  }

  auto CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) -> LRESULT {
    if (msg == WM_CLOSE || msg == WM_DESTROY || msg == WM_NCDESTROY || msg == WM_QUERYENDSESSION || msg == WM_ENDSESSION ||
        (msg == WM_SYSCOMMAND && (w_param & 0xFFF0) == SC_CLOSE)) {
      ext_client::utils::shutdown_guard::arm("window_close");
    }

    if (msg == WM_KEYUP && w_param == VK_INSERT) {
      ext_client::menu::set_visible(!ext_client::menu::is_visible());
    }

    if (imgui_ready) {
      ImGui_ImplWin32_WndProcHandler(hwnd, msg, w_param, l_param);
    }

    if (ext_client::menu::is_visible()) {
      if (is_mouse_message(msg) && ext_client::menu::wants_capture_mouse()) {
        return 0;
      }
      if (is_keyboard_message(msg) && w_param != VK_INSERT && ext_client::menu::wants_capture_keyboard()) {
        return 0;
      }
    }

    return CallWindowProcA(original_wndproc, hwnd, msg, w_param, l_param);
  }

  auto __stdcall end_scene_detour(IDirect3DDevice9* device) -> HRESULT {
    std::call_once(imgui_init_once, [&]() { init_imgui(device); });
    const HRESULT hr = g_end_scene.call_original(device);
    render_frame();
    return hr;
  }

  auto __stdcall reset_detour(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) -> HRESULT {
    const auto& cfg = ext_client::config::data().graphic;
    if (params) {
      if (cfg.d3d_triple_buffering && params->Windowed) {
        log_msg("[d3d_hook] Reset: Forcing triple buffering (BackBufferCount = 2).");
        params->BackBufferCount = 2;
      }
      if (cfg.d3d_discard_depth_stencil) {
        log_msg("[d3d_hook] Reset: Forcing D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL.");
        params->Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
      }
    }

    ImGui_ImplDX9_InvalidateDeviceObjects();
    const HRESULT hr = g_reset.call_original(device, params);
    if (SUCCEEDED(hr)) {
      ImGui_ImplDX9_CreateDeviceObjects();
    }
    return hr;
  }

  auto WINAPI direct3d_create9_detour(UINT SDKVersion) -> IDirect3D9* {
    IDirect3D9* d3d = g_direct3d_create9.call_original(SDKVersion);
    if (d3d) {
      log_msg("[d3d_hook] Direct3DCreate9 intercepted (SDKVersion=%u)", SDKVersion);
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
    const auto& cfg = ext_client::config::data().graphic;

    log_msg("[d3d_hook] CreateDevice: Adapter=%u, DeviceType=%d, BehaviorFlags=0x%X",
            Adapter, DeviceType, BehaviorFlags);

    if (cfg.d3d_force_hardware_vp) {
      D3DCAPS9 caps{};
      if (SUCCEEDED(self->GetDeviceCaps(Adapter, DeviceType, &caps))) {
        if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
          log_msg("[d3d_hook] GPU supports Hardware T&L. Forcing Hardware VP.");
          BehaviorFlags &= ~D3DCREATE_SOFTWARE_VERTEXPROCESSING;
          BehaviorFlags &= ~D3DCREATE_MIXED_VERTEXPROCESSING;
          BehaviorFlags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;

          if (cfg.d3d_force_pure_device) {
            if (caps.DevCaps & D3DDEVCAPS_PUREDEVICE) {
              log_msg("[d3d_hook] GPU supports Pure Device. Forcing Pure Device.");
              BehaviorFlags |= D3DCREATE_PUREDEVICE;
            } else {
              log_msg("[d3d_hook] Pure Device requested but not supported by GPU.");
            }
          }
        } else {
          log_msg("[d3d_hook] GPU does not support Hardware T&L. Software VP will be used.");
        }
      } else {
        log_msg("[d3d_hook] Failed to query device capabilities.");
      }
    }

    if (pPresentationParameters) {
      if (cfg.d3d_triple_buffering && pPresentationParameters->Windowed) {
        log_msg("[d3d_hook] Forcing triple buffering (BackBufferCount = 2).");
        pPresentationParameters->BackBufferCount = 2;
      }
      if (cfg.d3d_discard_depth_stencil) {
        log_msg("[d3d_hook] Forcing D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL.");
        pPresentationParameters->Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
      }
    }

    HRESULT hr = g_create_device.call_original(self, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
    if (SUCCEEDED(hr) && ppReturnedDeviceInterface && *ppReturnedDeviceInterface) {
      log_msg("[d3d_hook] Device successfully created @ %p", *ppReturnedDeviceInterface);
      hook_device_methods(*ppReturnedDeviceInterface);
    } else {
      log_msg("[d3d_hook] CreateDevice failed (hr=0x%08X)", hr);
    }
    return hr;
  }

  auto hook_create_device(IDirect3D9* d3d) -> void {
    if (g_create_device.is_applied()) {
      return;
    }
    const auto vtable = reinterpret_cast<std::uintptr_t>(*reinterpret_cast<void***>(d3d));
    const auto create_device_addr = ext_client::offsets::vtable_slot(vtable, 16);
    if (g_hooks.install(g_create_device, create_device_addr, &create_device_detour, "d3d_hook", "CreateDevice")) {
      log_msg("[d3d_hook] hooked CreateDevice @ 0x%08X", create_device_addr);
    }
  }

  auto hook_device_methods(IDirect3DDevice9* device) -> void {
    if (g_end_scene.is_applied() || g_reset.is_applied()) {
      return;
    }
    const auto vtable = reinterpret_cast<std::uintptr_t>(*reinterpret_cast<void***>(device));
    const auto end_scene_addr = ext_client::offsets::vtable_slot(vtable, k_end_scene_slot);
    const auto reset_addr = ext_client::offsets::vtable_slot(vtable, k_reset_slot);

    if (g_hooks.install(g_end_scene, end_scene_addr, &end_scene_detour, "d3d_hook", "EndScene") &&
        g_hooks.install(g_reset, reset_addr, &reset_detour, "d3d_hook", "Reset")) {
      log_msg("[d3d_hook] hooked EndScene @ 0x%08X and Reset @ 0x%08X", end_scene_addr, reset_addr);
    }
  }

} // namespace

namespace ext_client::hooks::d3d {

  auto install() -> bool {
    if (is_installed()) {
      return true;
    }

    HMODULE h_d3d9 = GetModuleHandleA("d3d9.dll");
    if (!h_d3d9) {
      return false;
    }

    auto d3d_create9_fn = reinterpret_cast<std::uintptr_t>(GetProcAddress(h_d3d9, "Direct3DCreate9"));
    if (d3d_create9_fn && !g_direct3d_create9.is_applied()) {
      if (g_hooks.install(g_direct3d_create9, d3d_create9_fn, &direct3d_create9_detour, "d3d_hook", "Direct3DCreate9")) {
        log_msg("[d3d_hook] hooked Direct3DCreate9 @ 0x%08X", d3d_create9_fn);
      }
    }

    // Fallback: if device is already created, hook it immediately
    if (auto* app = cgfx_video3d::get()) {
      if (auto* device = app->device()) {
        log_msg("[d3d_hook] active device already exists, hooking device methods immediately");
        hook_device_methods(device);
      }
    }

    return is_installed();
  }

  auto uninstall() -> void {
    g_hooks.uninstall();

    ext_client::menu::shutdown();
    unhook_wnd_proc();
    imgui_ready = false;
    logged_waiting = false;

    log_msg("[d3d_hook] unhooked");
  }

  auto is_installed() -> bool {
    return g_end_scene.is_applied() && g_reset.is_applied();
  }

  auto is_imgui_ready() -> bool {
    return imgui_ready;
  }

  auto apply_from_control() -> void {
    // D3D presentation flags apply on the next CreateDevice/Reset call.
  }

  auto game_hwnd() -> HWND {
    return hooked_hwnd;
  }

  auto client_mouse_pos(int& x, int& y) -> bool {
    if (!hooked_hwnd || !IsWindow(hooked_hwnd)) {
      return false;
    }

    POINT pt{};
    if (!GetCursorPos(&pt)) {
      return false;
    }
    if (!ScreenToClient(hooked_hwnd, &pt)) {
      return false;
    }

    x = pt.x;
    y = pt.y;
    return true;
  }

} // namespace ext_client::hooks::d3d
