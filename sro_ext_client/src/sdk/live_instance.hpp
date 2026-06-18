#pragma once

namespace ext_client::sdk {

  template<typename T> class live_instance {
  public:
    static auto set(T* instance) -> void { current() = instance; }
    static auto get() -> T* { return current(); }

  private:
    static auto current() -> T*& {
      static T* value = nullptr;
      return value;
    }
  };

} // namespace ext_client::sdk
