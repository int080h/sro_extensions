#include "sro_hierarchy.hpp"

#include "cg_interface.hpp"
#include "cprocess.hpp"
#include "cps_silkroad.hpp"
#include "cps_title.hpp"
#include "cps_version_check.hpp"
#include "cps_character_select.hpp"

namespace ext_client::sdk::hierarchy {

  auto as_outer(cps_title* title) -> cps_outer_interface* {
    return cast<cps_outer_interface>(title);
  }

  auto as_outer(cps_version_check* screen) -> cps_outer_interface* {
    return cast<cps_outer_interface>(screen);
  }

  auto as_outer(void* screen) -> cps_outer_interface* {
    return cast<cps_outer_interface>(screen);
  }

  auto as_cobj(void* obj) -> cobj* {
    return cast<cobj>(obj);
  }

  auto as_cobj_child(void* obj) -> cobj_child* {
    return cast<cobj_child>(obj);
  }

  auto as_gwnd_base(void* obj) -> cgwnd_base* {
    return cast_offset<cgwnd_base>(obj, ext_client::offsets::cgwnd_base::fields::region_begin);
  }

  auto as_gwnd(void* obj) -> cgwnd* {
    return cast<cgwnd>(obj);
  }

  auto as_cprocess(void* obj) -> cprocess* {
    return cast_offset<cprocess>(obj, ext_client::offsets::cprocess::fields::region_begin);
  }

  auto as_silkroad(void* obj) -> cps_silkroad* {
    return cast_offset<cps_silkroad>(obj, ext_client::offsets::cps_silkroad::fields::region_begin);
  }

  auto as_gwnd(cps_outer_interface* outer) -> cgwnd* {
    return outer ? outer->as_gwnd() : nullptr;
  }

  auto as_title(cps_outer_interface* outer) -> cps_title* {
    return cast<cps_title>(outer);
  }

  auto as_version_check(cps_outer_interface* outer) -> cps_version_check* {
    return cast<cps_version_check>(outer);
  }

  auto as_character_select(cps_outer_interface* outer) -> cps_character_select* {
    return cast<cps_character_select>(outer);
  }

  auto as_cif_wnd(void* obj) -> cif_wnd* {
    return cast<cif_wnd>(obj);
  }

  auto as_ctextboard(void* obj) -> ctextboard* {
    if (!obj) {
      return nullptr;
    }
    return cast_offset<ctextboard>(obj, ext_client::offsets::cif_wnd::fields::textboard_vftable);
  }

  auto as_cg_interface(void* obj) -> cg_interface* {
    return cast<cg_interface>(obj);
  }

  auto as_cnif_wnd(void* obj) -> cnif_wnd* {
    return cast<cnif_wnd>(obj);
  }

  auto as_cnif_page_manager(void* obj) -> cnif_page_manager* {
    return cast<cnif_page_manager>(obj);
  }

  auto as_entity_manager(void* obj) -> centity_manager* {
    return cast<centity_manager>(obj);
  }

  auto as_entity_manager_client(void* obj) -> centity_manager_client* {
    return cast<centity_manager_client>(obj);
  }

} // namespace ext_client::sdk::hierarchy
