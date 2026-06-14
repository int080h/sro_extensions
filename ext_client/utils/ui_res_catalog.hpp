#pragma once

#include <cstddef>

namespace ext_client::utils::ui_res_catalog {

  struct entry {
    char name[64]{};
    char type[32]{};
    int id = -1;
    char ddj[128]{};
    int rect_x = 0;
    int rect_y = 0;
    int rect_w = 0;
    int rect_h = 0;
  };

  enum class screen : int {
    pstitle = 0,
    character_select = 1,
  };

  enum class catalog_source : int {
    none = 0,
    file = 1,
    memory = 2,
    fallback = 3,
  };

  auto ensure_loaded(screen which) -> void;
  auto sync_from_game(void* res_ui_root, screen which) -> void;
  auto find(screen which, int res_id) -> const entry*;
  auto find_by_name(screen which, const char* gdr_name) -> const entry*;
  auto find_by_ddj(screen which, const char* ddj_path) -> const entry*;
  auto file_loaded(screen which) -> bool;
  auto memory_loaded(screen which) -> bool;
  auto catalog_source_for(screen which) -> catalog_source;
  auto layout_relative_path(screen which) -> const char*;

} // namespace ext_client::utils::ui_res_catalog
