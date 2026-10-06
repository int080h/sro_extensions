#pragma once

#include "sdk/game/cobj_child.hpp"
#include "sdk/game/ci_charactor.hpp"
#include "sdk/runtime/rtti.hpp"
#include "utils/memory.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

class ci_entity;

// ---------------------------------------------------------------------------
// CEntityManager — Global world entity registry manager
// Integrates both canonical g_sHashGID (world registry) & CEntityManagerClient
// ---------------------------------------------------------------------------
class centity_manager : public cobj_child {
public:
  static constexpr std::uint32_t k_singleton_addr = 0x013BAE08;
  static constexpr std::uint32_t k_gid_map_addr   = 0x01199120;
  static constexpr std::uint32_t k_gid_head_addr  = 0x01199124;
  static constexpr std::uint32_t k_gid_size_addr  = 0x01199128;
  static constexpr std::uint32_t k_gid_lookup_fn  = 0x00B24030;

  // Non-owning MSVC9 STL Views
  static auto gid_map_view() -> ext_client::msvc9::map_view<std::uint32_t, ci_charactor*> {
    return ext_client::msvc9::map_view<std::uint32_t, ci_charactor*>::from_head_and_size_addrs(
      k_gid_head_addr, k_gid_size_addr);
  }

  auto entity_vector_view() const -> ext_client::msvc9::vector_view<ci_charactor*> {
    return ext_client::msvc9::vector_view<ci_charactor*>::from_pointers(
      get_entity_begin(), get_entity_end());
  }

  // 1. Singleton & Runtime Inspection
  static auto get_singleton() -> centity_manager*;
  static auto is_instance(const void* ptr) -> bool;
  static auto cast(void* ptr) -> centity_manager*;
  static auto cast(const void* ptr) -> const centity_manager*;

  // 2. Entity Lookups
  auto lookup_by_slot(std::uint32_t slot_id) const -> ci_charactor*;
  auto lookup_entity(std::uint32_t slot_id) const -> ci_entity*;

  // 3. Entity Collections
  auto get_entity_begin() const -> ci_charactor* const*;
  auto get_entity_end() const -> ci_charactor* const*;
  auto get_entity_count() const -> std::size_t;

  // 4. Safe Templated Queries & Iteration
  // Invokes fn(ci_charactor* ent). If fn returns bool and returns false, iteration terminates early.
  template<typename Func>
  auto for_each_entity(Func&& fn) const -> void {
    // 1. Primary: Traverse canonical global entity registry (g_sHashGID) via map_view
    const auto map = gid_map_view();
    if (!map.empty()) {
      map.for_each([&](std::uint32_t /*uid*/, ci_charactor* ent) -> bool {
        if (ext_client::utils::memory::is_valid_ptr(ent)) {
          if constexpr (std::is_invocable_r_v<bool, Func, ci_charactor*>) {
            return fn(ent);
          } else {
            fn(ent);
            return true;
          }
        }
        return true;
      });
      return;
    }

    // 2. Secondary: Fall back to contiguous vector (CEntityManagerClient / CEntityManager)
    const auto vec = entity_vector_view();
    vec.for_each([&](ci_charactor* ent) -> bool {
      if (ext_client::utils::memory::is_valid_ptr(ent)) {
        if constexpr (std::is_invocable_r_v<bool, Func, ci_charactor*>) {
          return fn(ent);
        } else {
          fn(ent);
          return true;
        }
      }
      return true;
    });
  }

  // Iterates entities filtered by type T (or CICharactor by default)
  template<typename T = ci_charactor, typename Func>
  auto for_each(Func&& fn) const -> void {
    for_each_entity([&](ci_charactor* ent) -> bool {
      if constexpr (!std::is_same_v<T, ci_charactor>) {
        if (!ext_client::rtti::is_kind_of(ent, T::k_class_name)) {
          return true; // continue
        }
      } else {
        if (!ext_client::rtti::is_kind_of(ent, "CICharactor")) {
          return true; // continue
        }
      }
      auto* typed_ent = reinterpret_cast<T*>(ent);
      if constexpr (std::is_invocable_r_v<bool, Func, T*>) {
        return fn(typed_ent);
      } else {
        fn(typed_ent);
        return true;
      }
    });
  }

  template<typename Func>
  auto for_each_character(Func&& fn) const -> void {
    for_each<ci_charactor>(std::forward<Func>(fn));
  }

  template<typename T = ci_charactor, typename Predicate>
  auto find_if(Predicate&& pred) const -> T* {
    T* found = nullptr;
    for_each<T>([&](T* ent) -> bool {
      if (pred(ent)) {
        found = ent;
        return false; // Stop iteration immediately
      }
      return true;
    });
    return found;
  }

  template<typename T = ci_charactor>
  auto find_by_unique_id(std::uint32_t unique_id) const -> T* {
    if (unique_id == 0) {
      return nullptr;
    }

    // 1. Fast O(log N) binary search in g_sHashGID via map_view
    const auto map = gid_map_view();
    if (auto* ent_ptr = map.find_value(unique_id)) {
      auto* ent = *ent_ptr;
      if (ext_client::utils::memory::is_valid_ptr(ent)) {
        if constexpr (!std::is_same_v<T, ci_charactor>) {
          if (!ext_client::rtti::is_kind_of(ent, T::k_class_name)) {
            return nullptr;
          }
        }
        return reinterpret_cast<T*>(ent);
      }
      return nullptr;
    }

    // 2. Native helper fallback (sub_B24030)
    using find_fn = ci_charactor*(__cdecl*)(std::uint32_t);
    if (const auto fn = ext_client::off::as_fn<find_fn>(k_gid_lookup_fn)) {
      auto* ent = fn(unique_id);
      if (ext_client::utils::memory::is_valid_ptr(ent)) {
        if constexpr (!std::is_same_v<T, ci_charactor>) {
          if (!ext_client::rtti::is_kind_of(ent, T::k_class_name)) {
            return nullptr;
          }
        }
        return reinterpret_cast<T*>(ent);
      }
    }

    // 3. Fall back to linear find_if across active entities
    return find_if<T>([unique_id](const T* ent) {
      return ent->get_unique_id() == unique_id;
    });
  }

  template<typename T = ci_charactor>
  auto lookup_by_uid_or_slot(std::uint32_t id) const -> T* {
    if (id == 0) {
      return nullptr;
    }
    if (auto* ent = find_by_unique_id<T>(id)) {
      return ent;
    }
    auto* slot_ent = lookup_by_slot(id);
    if (!ext_client::utils::memory::is_valid_ptr(slot_ent)) {
      return nullptr;
    }
    if constexpr (!std::is_same_v<T, ci_charactor>) {
      if (!ext_client::rtti::is_kind_of(slot_ent, T::k_class_name)) {
        return nullptr;
      }
    }
    return reinterpret_cast<T*>(slot_ent);
  }

  // 5. Static Convenience Wrappers (Access via Singleton)
  static auto entity_count() -> std::size_t {
    auto* mgr = get_singleton();
    return mgr ? mgr->get_entity_count() : 0;
  }

  template<typename T = ci_charactor, typename Func>
  static auto for_each_in_world(Func&& fn) -> void {
    if (auto* mgr = get_singleton()) {
      mgr->for_each<T>(std::forward<Func>(fn));
    }
  }

  template<typename Func>
  static auto for_each_all_entities(Func&& fn) -> void {
    if (auto* mgr = get_singleton()) {
      mgr->for_each_entity(std::forward<Func>(fn));
    }
  }

  template<typename T = ci_charactor>
  static auto find_entity_by_uid(std::uint32_t unique_id) -> T* {
    auto* mgr = get_singleton();
    return mgr ? mgr->find_by_unique_id<T>(unique_id) : nullptr;
  }

  template<typename T = ci_charactor>
  static auto resolve_by_uid_or_slot(std::uint32_t id) -> T* {
    auto* mgr = get_singleton();
    return mgr ? mgr->lookup_by_uid_or_slot<T>(id) : nullptr;
  }
};
