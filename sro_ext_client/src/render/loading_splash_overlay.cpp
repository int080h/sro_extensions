#include "core/core_main.hpp"

#include "render/loading_splash_overlay.hpp"

#include "utils/log.hpp"

#include <gdiplus.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <iterator>

#pragma comment(lib, "gdiplus.lib")

using ext_client::utils::log_msg;

namespace ext_client::render::loading_splash_overlay {
  namespace {

    constexpr char k_window_class[] = "ext_client_loading_splash_overlay";

    std::atomic<bool> g_running{false};
    HANDLE g_thread = nullptr;
    HANDLE g_stop_event = nullptr;
    HWND g_hwnd = nullptr;
    HWND g_owner = nullptr;
    config g_cfg{};
    int g_frame = 1;
    ULONG_PTR g_gdiplus = 0;

    auto clamp_positive(int value, int fallback) -> int {
      return value > 0 ? value : fallback;
    }

    auto draw_fallback(HDC dc, const RECT& rc) -> void {
      const COLORREF colors[] = {
        RGB(20, 24, 32),
        RGB(34, 24, 20),
        RGB(18, 36, 28),
        RGB(38, 30, 18),
        RGB(26, 22, 40),
      };
      HBRUSH brush = CreateSolidBrush(colors[(g_frame - 1) % (sizeof(colors) / sizeof(colors[0]))]);
      FillRect(dc, &rc, brush);
      DeleteObject(brush);

      SetBkMode(dc, TRANSPARENT);
      SetTextColor(dc, RGB(235, 238, 242));

      char text[512]{};
      std::snprintf(text, sizeof(text), "Loading splash %02d", g_frame);
      RECT text_rc = rc;
      InflateRect(&text_rc, -14, -10);
      DrawTextA(dc, text, -1, &text_rc, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
    }

    auto paint(HWND hwnd) -> void {
      PAINTSTRUCT ps{};
      HDC dc = BeginPaint(hwnd, &ps);
      RECT rc{};
      GetClientRect(hwnd, &rc);

      if (g_cfg.frames && g_frame >= 1 && g_frame <= g_cfg.frame_count) {
        auto* bmp = reinterpret_cast<Gdiplus::Bitmap*>(g_cfg.frames[g_frame - 1]);
        if (bmp && g_gdiplus) {
          Gdiplus::Graphics graphics(dc);
          if (bmp->GetLastStatus() == Gdiplus::Ok) {
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            graphics.DrawImage(bmp, 0, 0, rc.right - rc.left, rc.bottom - rc.top);
            EndPaint(hwnd, &ps);
            return;
          }
        }
      }

      draw_fallback(dc, rc);
      EndPaint(hwnd, &ps);
    }

    auto CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) -> LRESULT {
      switch (msg) {
        case WM_TIMER:
          g_frame = (g_frame % clamp_positive(g_cfg.frame_count, 1)) + 1;
          InvalidateRect(hwnd, nullptr, FALSE);
          return 0;
        case WM_PAINT:
          paint(hwnd);
          return 0;
        case WM_CLOSE:
          DestroyWindow(hwnd);
          return 0;
        case WM_DESTROY:
          KillTimer(hwnd, 1);
          g_hwnd = nullptr;
          PostQuitMessage(0);
          return 0;
        default:
          return DefWindowProcA(hwnd, msg, w_param, l_param);
      }
    }

    auto register_window_class() -> bool {
      WNDCLASSEXA wc{};
      wc.cbSize = sizeof(wc);
      wc.lpfnWndProc = wnd_proc;
      wc.hInstance = GetModuleHandleA(nullptr);
      wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
      wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
      wc.lpszClassName = k_window_class;
      RegisterClassExA(&wc);
      return true;
    }

    DWORD WINAPI overlay_thread_proc(LPVOID) {
      Gdiplus::GdiplusStartupInput gdiplus_input{};
      if (Gdiplus::GdiplusStartup(&g_gdiplus, &gdiplus_input, nullptr) != Gdiplus::Ok) {
        g_gdiplus = 0;
      }

      register_window_class();

      g_frame = 1;
      g_hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT | WS_EX_LAYERED,
                               k_window_class,
                               "",
                               WS_POPUP,
                               g_cfg.x,
                               g_cfg.y,
                               clamp_positive(g_cfg.width, 400),
                               clamp_positive(g_cfg.height, 148),
                               g_owner,
                               nullptr,
                               GetModuleHandleA(nullptr),
                               nullptr);
      if (!g_hwnd) {
        if (g_cfg.log_events) {
          log_msg("[version_check] overlay create failed, gle=%lu", GetLastError());
        }
        if (g_gdiplus) {
          Gdiplus::GdiplusShutdown(g_gdiplus);
          g_gdiplus = 0;
        }
        g_running.store(false, std::memory_order_release);
        return 0;
      }

      SetLayeredWindowAttributes(g_hwnd, 0, 255, LWA_ALPHA);

      ShowWindow(g_hwnd, SW_SHOWNOACTIVATE);
      UpdateWindow(g_hwnd);
      SetTimer(g_hwnd, 1, clamp_positive(g_cfg.interval_ms, 250), nullptr);

      if (g_cfg.log_events) {
        log_msg("[version_check] overlay started hwnd=%p owner=%p pos=%d,%d size=%dx%d interval=%d frames=%d",
                g_hwnd,
                g_owner,
                g_cfg.x,
                g_cfg.y,
                g_cfg.width,
                g_cfg.height,
                g_cfg.interval_ms,
                g_cfg.frame_count);
      }

      MSG msg{};
      while (WaitForSingleObject(g_stop_event, 0) != WAIT_OBJECT_0) {
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
          if (msg.message == WM_QUIT) {
            SetEvent(g_stop_event);
            break;
          }
          TranslateMessage(&msg);
          DispatchMessageA(&msg);
        }
        MsgWaitForMultipleObjects(1, &g_stop_event, FALSE, 50, QS_ALLINPUT);
      }

      if (g_hwnd) {
        DestroyWindow(g_hwnd);
        g_hwnd = nullptr;
      }

      if (g_gdiplus) {
        Gdiplus::GdiplusShutdown(g_gdiplus);
        g_gdiplus = 0;
      }

      if (g_cfg.log_events) {
        log_msg("[version_check] overlay stopped");
      }
      g_running.store(false, std::memory_order_release);
      return 0;
    }

  } // namespace

  auto start(HWND owner, const config& cfg) -> bool {
    if (g_running.load(std::memory_order_acquire)) {
      return true;
    }

    g_cfg = cfg;
    g_owner = owner;
    g_cfg.width = clamp_positive(g_cfg.width, 400);
    g_cfg.height = clamp_positive(g_cfg.height, 148);
    g_cfg.interval_ms = clamp_positive(g_cfg.interval_ms, 250);
    g_cfg.frame_count = clamp_positive(g_cfg.frame_count, 1);

    g_stop_event = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (!g_stop_event) {
      return false;
    }

    g_running.store(true, std::memory_order_release);
    g_thread = CreateThread(nullptr, 0, overlay_thread_proc, nullptr, 0, nullptr);
    if (!g_thread) {
      CloseHandle(g_stop_event);
      g_stop_event = nullptr;
      g_running.store(false, std::memory_order_release);
      return false;
    }
    return true;
  }

  auto stop() -> void {
    if (!g_running.load(std::memory_order_acquire) && !g_thread) {
      return;
    }

    if (g_stop_event) {
      SetEvent(g_stop_event);
    }
    if (g_hwnd) {
      PostMessageA(g_hwnd, WM_CLOSE, 0, 0);
    }
    if (g_thread) {
      WaitForSingleObject(g_thread, 1500);
      CloseHandle(g_thread);
      g_thread = nullptr;
    }
    if (g_stop_event) {
      CloseHandle(g_stop_event);
      g_stop_event = nullptr;
    }
    g_running.store(false, std::memory_order_release);
  }

  auto is_running() -> bool {
    return g_running.load(std::memory_order_acquire);
  }

} // namespace ext_client::render::loading_splash_overlay
