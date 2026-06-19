#pragma once

#include <Windows.h>

#include <functional>

namespace ext_client::render {

  auto is_mouse_message(UINT msg) -> bool;
  auto is_keyboard_message(UINT msg) -> bool;

  class input_handler {
  public:
    using toggle_callback = std::function<void()>;
    using shutdown_callback = std::function<void(const char* reason)>;

    input_handler() = default;
    ~input_handler();

    input_handler(const input_handler&) = delete;
    input_handler& operator=(const input_handler&) = delete;
    input_handler(input_handler&&) = delete;
    input_handler& operator=(input_handler&&) = delete;

    auto install(HWND hwnd, toggle_callback on_toggle, shutdown_callback on_shutdown) -> bool;
    auto uninstall() -> void;
    auto is_installed() const -> bool;
    auto hwnd() const -> HWND;

    auto set_imgui_ready(bool ready) -> void;
    auto set_wants_capture_mouse(bool v) -> void;
    auto set_wants_capture_keyboard(bool v) -> void;
    auto wants_capture_mouse() const -> bool;
    auto wants_capture_keyboard() const -> bool;

    auto client_mouse_pos(int& x, int& y) const -> bool;

  private:
    HWND m_hwnd = nullptr;
    WNDPROC m_original_wndproc = nullptr;
    bool m_imgui_ready = false;
    bool m_wants_capture_mouse = false;
    bool m_wants_capture_keyboard = false;
    toggle_callback m_on_toggle;
    shutdown_callback m_on_shutdown;

    static auto CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) -> LRESULT;
  };

} // namespace ext_client::render
