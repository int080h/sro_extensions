#pragma once

#include <Windows.h>
#include <d3d9.h>

namespace ext_client::render {

  class imgui_renderer {
  public:
    imgui_renderer() = default;
    ~imgui_renderer();

    imgui_renderer(const imgui_renderer&) = delete;
    imgui_renderer& operator=(const imgui_renderer&) = delete;
    imgui_renderer(imgui_renderer&&) = delete;
    imgui_renderer& operator=(imgui_renderer&&) = delete;

    auto initialize(HWND hwnd, IDirect3DDevice9* device) -> bool;
    auto shutdown() -> void;
    auto is_initialized() const -> bool;

    auto rebind_hwnd(HWND new_hwnd) -> void;
    auto on_device_lost() -> void;
    auto on_device_restored() -> void;

    auto begin_frame() -> void;
    auto end_frame() -> void;

    auto hwnd() const -> HWND;

  private:
    bool m_initialized = false;
    HWND m_hwnd = nullptr;
  };

} // namespace ext_client::render
