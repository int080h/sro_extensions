#pragma once



#include "utils/msvc9_stl.hpp"




// cgobj — compact game-object base used by in-game net processes (CNetProcessIn).

class cgobj {

public:

  auto get_field_0c() -> int;
  auto get_list() -> ext_client::msvc9::n_list<void*>;
  auto set_field_0c(int val) -> void;
  auto set_list(ext_client::msvc9::n_list<void*> val) -> void;

};

