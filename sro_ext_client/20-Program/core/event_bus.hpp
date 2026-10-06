#pragma once

#include "core/client_events.hpp"

#include <atomic>
#include <memory>
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace ext_client::core::event {

  auto current_owner() -> const char *;
  auto resolve_owner_state(const char *owner) -> std::shared_ptr<std::atomic<bool>>;
  auto dispatch_mutex() -> std::recursive_mutex &;
  auto dispatch_stopping() -> bool;
  auto stop_dispatch() -> void;

  struct plugin_owner_scope {
    explicit plugin_owner_scope(const char *owner);
    ~plugin_owner_scope();

    plugin_owner_scope(const plugin_owner_scope &) = delete;
    plugin_owner_scope &operator=(const plugin_owner_scope &) = delete;

  private:
    const char *m_previous = nullptr;
  };

  template <int EventId, typename Callback, typename... TriggerArgs> class event_handler {
  public:
    static auto instance() -> event_handler & {
      static event_handler handler;
      return handler;
    }

    auto add(Callback callback, const char *owner, const char *debug_name) -> void {
      if (!callback)
        return;
      std::lock_guard lock(m_registration_mutex);
      auto next = std::make_shared<callback_list>(*m_callbacks.load());
      next->push_back(entry{std::move(callback), owner ? owner : "", resolve_owner_state(owner), debug_name});
      m_callbacks.store(std::shared_ptr<const callback_list>(std::move(next)));
    }

    auto trigger(TriggerArgs... args) -> void {
      constexpr bool lifecycle = EventId == static_cast<int>(event_type::on_shutdown) ||
                                 EventId == static_cast<int>(event_type::on_background_tick);
      if (!lifecycle && dispatch_stopping())
        return;
      if constexpr (EventId == static_cast<int>(event_type::on_background_tick)) {
        const auto callbacks = m_callbacks.load();
        for (const auto &item : *callbacks)
          item.callback(args...);
        return;
      }
      std::lock_guard lock(dispatch_mutex());
      if (!lifecycle && dispatch_stopping())
        return;
      const auto callbacks = m_callbacks.load();
      for (const auto &item : *callbacks) {
        if (lifecycle || active(item))
          item.callback(args...);
      }
    }

    auto trigger_owner(const char *owner, TriggerArgs... args) -> void {
      if (!owner || dispatch_stopping())
        return;
      std::lock_guard lock(dispatch_mutex());
      if (dispatch_stopping())
        return;
      const auto callbacks = m_callbacks.load();
      for (const auto &item : *callbacks) {
        if (item.owner == owner && active(item))
          item.callback(args...);
      }
    }

    auto has_owner(const char *owner) const -> bool {
      if (!owner)
        return false;
      const auto callbacks = m_callbacks.load();
      for (const auto &item : *callbacks)
        if (item.owner == owner)
          return true;
      return false;
    }

    auto has_active_listeners() const -> bool {
      if (dispatch_stopping())
        return false;
      const auto callbacks = m_callbacks.load();
      for (const auto &item : *callbacks)
        if (active(item))
          return true;
      return false;
    }

  private:
    struct entry {
      Callback callback;
      std::string owner;
      std::shared_ptr<std::atomic<bool>> enabled;
      const char *debug_name;
    };
    using callback_list = std::vector<entry>;
    static auto active(const entry &item) -> bool {
      return item.owner.empty() || (item.enabled && item.enabled->load(std::memory_order_acquire));
    }
    std::mutex m_registration_mutex;
    std::atomic<std::shared_ptr<const callback_list>> m_callbacks{std::make_shared<const callback_list>()};
  };

} // namespace ext_client::core::event

#define ADD_EVENT(evt, callback)                                                                                       \
  ::ext_client::core::event::event_handler<evt>::instance().add(callback, ::ext_client::core::event::current_owner(),  \
                                                                #callback)

#define TRIGGER_EVENT(evt, ...) ::ext_client::core::event::event_handler<evt>::instance().trigger(__VA_ARGS__)
