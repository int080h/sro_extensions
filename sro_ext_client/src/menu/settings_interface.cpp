#include "ext_client.hpp"

#include "menu/settings_interface.hpp"

#include "menu/interface_manager.hpp"

namespace ext_client::menu::settings_interface {

  auto draw() -> void {
    interface_manager::draw();
  }

} // namespace ext_client::menu::settings_interface
