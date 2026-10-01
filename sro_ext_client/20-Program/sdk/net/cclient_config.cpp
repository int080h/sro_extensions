#include "pch.hpp"
#include "sdk/net/cclient_config.hpp"
#include "utils/offsets.hpp"

auto cclient_config::get_selected_server() -> ext_client::msvc9::wstring {
  return ext_client::off::field_at<ext_client::msvc9::wstring>(this, 0x4C);
}
