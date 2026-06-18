#include "features/combat_overlay/entity_manager.hpp"

#include "sdk/cicharactor.hpp"
#include "sdk/centity_manager_client.hpp"

namespace ext_client::entity_manager {

  auto get() -> void* {
    return centity_manager_client::get();
  }

  auto lookup_by_slot(void* mgr, std::uint32_t slot_id) -> ci_charactor* {
    auto* client = static_cast<centity_manager_client*>(mgr);
    if (!client || !centity_manager_client::is_instance(client)) {
      return nullptr;
    }
    return client->lookup_by_slot(slot_id);
  }

  auto lookup_by_uid(void* mgr, std::uint32_t uid) -> ci_charactor* {
    auto* client = static_cast<centity_manager_client*>(mgr);
    if (!client || !centity_manager_client::is_instance(client) || uid == 0) {
      return nullptr;
    }

    const auto count = client->entity_count();
    if (count > 4096) {
      return nullptr;
    }

    auto* const* begin = client->entity_begin();
    if (!begin || !ext_client::msvc9::is_readable_ptr(begin, count * sizeof(ci_charactor*))) {
      return nullptr;
    }

    for (std::size_t i = 0; i < count; ++i) {
      ci_charactor* entity = begin[i];
      if (!entity || !ext_client::msvc9::is_readable_ptr(entity, ext_client::offsets::ci_charactor::fields::unique_id + sizeof(std::uint32_t))) {
        continue;
      }
      if (unique_id(entity) == uid) {
        return entity;
      }
    }
    return nullptr;
  }

  auto unique_id(const ci_charactor* entity) -> std::uint32_t {
    return entity ? entity->unique_id() : 0;
  }

  auto display_name(const ci_charactor* entity) -> const wchar_t* {
    return entity ? entity->display_name() : L"";
  }

} // namespace ext_client::entity_manager
