#pragma once

#include "sdk/types/ibsnet_init_config.hpp"

#include <cstdint>

class cnet_engine;

class cmsg;



// CNetEngine / IBSNet — Joymax socket layer (Winsock, CMsg pools, active/passive sockets).

class cnet_engine {

public:

  auto get_ref_count() -> int;
  auto get_com_ptr() -> void*;
  auto get_socket_manager() -> void*;
  auto get_socket_manager_tail() -> void*;
  auto set_ref_count(int val) -> void;
  auto set_com_ptr(void* val) -> void;
  auto set_socket_manager(void* val) -> void;
  auto set_socket_manager_tail(void* val) -> void;

};

