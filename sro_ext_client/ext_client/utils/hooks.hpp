#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <functional>
#include <type_traits>

namespace ext_client::utils {

  // Forward declare log_msg just in case so hooks.hpp is self-contained.
  auto log_msg(const char* format, ...) -> void;

  // ---------------------------------------------------------------------------
  // Detour Hooking Utilities
  // ---------------------------------------------------------------------------

  enum class convention_type {
    thiscall_t,
    stdcall_t,
    cdecl_t,
    fastcall_t,
  };

  namespace detail {

    // x86 thiscall -> detour must be __fastcall:
    //   ECX = this (Self)
    //   EDX = garbage padding when Rest... is non-empty (declare void* /*edx*/; never use it)
    //   stack = Rest...
    // apply() takes hook_type, so a wrong detour signature is a compile error.
    template<typename Retn, typename Self, typename... Rest> struct thiscall_types {
      using original = Retn(__thiscall*)(Self, Rest...);
      using detour = std::conditional_t<sizeof...(Rest) == 0, Retn(__fastcall*)(Self), Retn(__fastcall*)(Self, void* /*edx*/, Rest...)>;
    };

  } // namespace detail

  template<convention_type Tp, typename Retn, typename... Args> struct convention;

  // thiscall hook args: make_hook<thiscall_t, Retn, Self, Rest...>
  // e.g. make_hook<thiscall_t, int, cps_title*, cmsg_stream_buffer*>
  template<typename Retn, typename Self, typename... Rest> struct convention<convention_type::thiscall_t, Retn, Self, Rest...> {
    using types = detail::thiscall_types<Retn, Self, Rest...>;
    using type = typename types::detour;
    using original_type = typename types::original;
  };

  template<typename Retn, typename... Args> struct convention<convention_type::stdcall_t, Retn, Args...> {
    using type = Retn(__stdcall*)(Args...);
    using original_type = type;
  };

  template<typename Retn, typename... Args> struct convention<convention_type::cdecl_t, Retn, Args...> {
    using type = Retn(__cdecl*)(Args...);
    using original_type = type;
  };

  template<typename Retn, typename... Args> struct convention<convention_type::fastcall_t, Retn, Args...> {
    using type = Retn(__fastcall*)(Args...);
    using original_type = type;
  };

  auto hook_lib_init() -> bool;
  auto hook_lib_shutdown() -> bool;
  auto apply_detour(char* original_addr, char* detour_addr, char** out_gateway) -> bool;
  auto remove_detour(char* original_addr) -> bool;
  auto reapply_detour(char* original_addr) -> bool;

  template<convention_type Tp, typename Retn, typename... Args> class make_hook {
  public:
    using hook_type = typename convention<Tp, Retn, Args...>::type;
    using original_type = typename convention<Tp, Retn, Args...>::original_type;

    auto apply(std::uintptr_t func_addr, hook_type detour) -> bool {
      if (is_applied_) {
        return true;
      }
      original_ = reinterpret_cast<hook_type>(func_addr);
      detour_ = detour;
      is_applied_ = apply_detour(reinterpret_cast<char*>(original_), reinterpret_cast<char*>(detour_), reinterpret_cast<char**>(&gateway_));
      return is_applied_;
    }

    auto disable() -> bool {
      if (!is_applied_) {
        return true;
      }
      if (!remove_detour(reinterpret_cast<char*>(original_))) {
        return false;
      }
      is_applied_ = false;
      return true;
    }

    auto enable() -> bool {
      if (is_applied_) {
        return true;
      }
      if (!gateway_) {
        return false;
      }
      if (!reapply_detour(reinterpret_cast<char*>(original_))) {
        return false;
      }
      is_applied_ = true;
      return true;
    }

    auto is_applied() const -> bool { return is_applied_; }

    auto get_original() const -> std::uintptr_t { return reinterpret_cast<std::uintptr_t>(gateway_); }

    auto call_original(Args... p) -> Retn { return reinterpret_cast<original_type>(gateway_)(p...); }

  private:
    bool is_applied_ = false;
    hook_type original_{};
    hook_type detour_{};
    hook_type gateway_{};
  };

  class hook_group {
  public:
    template<typename Hook, typename Detour>
    auto install(Hook& hook, std::uintptr_t address, Detour detour, const char* log_prefix, const char* name) -> bool {
      if (!hook.apply(address, detour)) {
        log_msg("[%s] %s hook failed", log_prefix, name);
        uninstall();
        return false;
      }
      teardown_.push_back([&hook] { hook.disable(); });
      return true;
    }

    auto uninstall() -> void {
      for (auto it = teardown_.rbegin(); it != teardown_.rend(); ++it) {
        (*it)();
      }
      teardown_.clear();
    }

    auto is_installed() const -> bool { return !teardown_.empty(); }

  private:
    std::vector<std::function<void()>> teardown_;
  };

  // ---------------------------------------------------------------------------
  // Virtual Method Table (VMT) Hooking Utilities
  // ---------------------------------------------------------------------------

  auto is_code_ptr(void* ptr) -> bool;

  class table_hook {
  public:
    constexpr table_hook() = default;

    ~table_hook() {
      if (m_new_vmt) {
        delete[] (m_new_vmt - 1);
      }
    }

    table_hook(const table_hook&) = delete;
    table_hook& operator=(const table_hook&) = delete;

  protected:
    auto initialize(void** original_table) -> void;
    auto hook_instance(void* inst) const -> void;
    auto unhook_instance(void* inst) const -> void;
    auto initialize_and_hook_instance(void* inst) -> bool;

    template<typename Fn> auto hook_function(Fn hooked_fn, std::size_t index) -> Fn {
      m_new_vmt[index] = reinterpret_cast<void*>(hooked_fn);
      return reinterpret_cast<Fn>(m_old_vmt[index]);
    }

    template<typename Fn> auto get_original_function(std::size_t index) const -> Fn { return reinterpret_cast<Fn>(m_old_vmt[index]); }

  private:
    void** m_new_vmt = nullptr;
    void** m_old_vmt = nullptr;
  };

  class vmt_hook : public table_hook {
  public:
    explicit vmt_hook(void* class_base);
    ~vmt_hook();

    vmt_hook(const vmt_hook&) = delete;
    vmt_hook& operator=(const vmt_hook&) = delete;

    template<typename Fn> auto apply_hook(std::size_t idx, Fn hooked_fn) -> void { hook_function(hooked_fn, idx); }

    template<typename T> auto apply_hook(std::size_t idx) -> void { T::m_original = hook_function(&T::hooked, idx); }

    auto unhook() const -> void { unhook_instance(m_class); }

    using table_hook::get_original_function;

  private:
    void* m_class = nullptr;
  };

} // namespace ext_client::utils
