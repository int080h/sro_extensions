#pragma once



#include "sdk/render/cd3d_application.hpp"




#include <cstdint>



class cgfx_video3d;



class cgfx_video3d : public cd3d_application {

public:

  static auto get() -> cgfx_video3d*;



  auto create_things(HWND hwnd_param, void* handler, int flags) -> bool;

  auto destroy_things() -> bool;

  auto set_size(std::uint32_t width, std::uint32_t height) -> bool;

  auto begin_scene() -> bool;

  auto end_scene() -> bool;

  auto present(int a2 = 0, int a3 = 0, int a4 = 0, int a5 = 0) -> bool;

  auto set_format(int format) -> int;

  auto render() -> int;

  auto frame_move() -> int;

private:

};

