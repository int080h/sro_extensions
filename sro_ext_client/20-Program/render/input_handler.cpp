#include "pch.hpp"
#include "render/input_handler.hpp"

#include "utils/log.hpp"
#include "utils/hooks.hpp"
#include "core/core_event_manager.hpp"

#include <imgui.h>
#include <imgui_impl_win32.h>

#include <Windows.h>

using ext_client::utils::log_msg;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

namespace ext_client::render {

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

  namespace {
    input_handler *g_instance = nullptr;
  } // namespace

  input_handler::~input_handler() {
    uninstall();
  }

  auto input_handler::install(HWND hwnd, toggle_callback on_toggle, shutdown_callback on_shutdown) -> bool {
    if (!hwnd || !IsWindow(hwnd) || m_hwnd) {
      return false;
    }

    m_on_toggle = std::move(on_toggle);
    m_on_shutdown = std::move(on_shutdown);
    g_instance = this;
    SetLastError(0);
    const auto previous = SetWindowLongPtrA(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&wnd_proc));
    if (!previous && GetLastError()) {
      g_instance = nullptr;
      return false;
    }
    m_original_wndproc = reinterpret_cast<WNDPROC>(previous);
    m_hwnd = hwnd;
    return true;
  }

  auto input_handler::uninstall() -> bool {
    if (!m_hwnd)
      return true;
    if (IsWindow(m_hwnd)) {
      // Another subclass may retain our procedure in its chain. Keep the DLL resident in that case.
      if (!m_original_wndproc || GetWindowLongPtrA(m_hwnd, GWLP_WNDPROC) != reinterpret_cast<LONG_PTR>(&wnd_proc)) {
        return false;
      }
      SetLastError(0);
      if (!SetWindowLongPtrA(m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_original_wndproc)) && GetLastError()) {
        return false;
      }
    }
    m_hwnd = nullptr;
    m_imgui_ready = false;
    m_wants_capture_mouse = false;
    m_wants_capture_keyboard = false;
    return true;
  }

  auto input_handler::is_installed() const -> bool {
    return m_hwnd != nullptr;
  }

  auto input_handler::hwnd() const -> HWND {
    return m_hwnd;
  }

  auto input_handler::set_imgui_ready(bool ready) -> void {
    m_imgui_ready = ready;
  }

  auto input_handler::set_wants_capture_mouse(bool v) -> void {
    m_wants_capture_mouse = v;
  }

  auto input_handler::set_wants_capture_keyboard(bool v) -> void {
    m_wants_capture_keyboard = v;
  }

  auto input_handler::wants_capture_mouse() const -> bool {
    return m_wants_capture_mouse;
  }

  auto input_handler::wants_capture_keyboard() const -> bool {
    return m_wants_capture_keyboard;
  }

  auto input_handler::client_mouse_pos(int &x, int &y) const -> bool {
    if (!m_hwnd || !IsWindow(m_hwnd)) {
      return false;
    }

    POINT pt{};
    if (!GetCursorPos(&pt)) {
      return false;
    }
    if (!ScreenToClient(m_hwnd, &pt)) {
      return false;
    }

    x = pt.x;
    y = pt.y;
    return true;
  }

  auto CALLBACK input_handler::wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) -> LRESULT {
    ext_client::utils::hook_call_scope active_call;
    std::unique_lock lock(ext_client::core::event::dispatch_mutex());
    auto *self = g_instance;
    if (!self) {
      return DefWindowProcA(hwnd, msg, w_param, l_param);
    }

    const auto original = self->m_original_wndproc;
    if (ext_client::core::event::dispatch_stopping()) {
      lock.unlock();
      return CallWindowProcA(original, hwnd, msg, w_param, l_param);
    }

    if (msg == WM_CLOSE || msg == WM_DESTROY || msg == WM_NCDESTROY || msg == WM_QUERYENDSESSION ||
        msg == WM_ENDSESSION || (msg == WM_SYSCOMMAND && (w_param & 0xFFF0) == SC_CLOSE)) {
      if (self->m_on_shutdown) {
        self->m_on_shutdown("window_close");
      }
    }

    if (msg == WM_KEYUP && w_param == VK_INSERT) {
      if (self->m_on_toggle) {
        self->m_on_toggle();
      }
    }

    if (self->m_imgui_ready) {
      ImGui_ImplWin32_WndProcHandler(hwnd, msg, w_param, l_param);
    }

    if (self->m_wants_capture_mouse || self->m_wants_capture_keyboard) {
      if (self->m_wants_capture_mouse && is_mouse_message(msg)) {
        return 0;
      }
      if (self->m_wants_capture_keyboard && w_param != VK_INSERT && is_keyboard_message(msg)) {
        return 0;
      }
    }

    lock.unlock();
    return CallWindowProcA(original, hwnd, msg, w_param, l_param);
  }

} // namespace ext_client::render
