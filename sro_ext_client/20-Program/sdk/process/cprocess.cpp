#include "pch.hpp"
#include "sdk/process/cprocess.hpp"

#include "utils/offsets.hpp"

auto cprocess::get_net_state() const -> int {
  return ext_client::off::field_at<int>(this, 0x084);
}

auto cprocess::get_thread_map() -> thread_map_t& {
  return ext_client::off::field_at<thread_map_t>(this, 0x088);
}

auto cprocess::get_msg_queue() -> msg_queue_set_t& {
  return ext_client::off::field_at<msg_queue_set_t>(this, 0x0A4);
}

auto cprocess::get_load_thread() -> void* {
  return ext_client::off::field_at<void*>(this, 0x0A0);
}

auto cprocess::set_net_state(int val) -> void {
  ext_client::off::field_at<int>(this, 0x084) = val;
}

auto cprocess::set_load_thread(void* val) -> void {
  ext_client::off::field_at<void*>(this, 0x0A0) = val;
}
