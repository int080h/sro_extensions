#pragma once

#include "calarm_data.hpp"
#include "utils/offsets.hpp"

class calarm_store {
public:
  auto alarm_data() -> calarm_data*;
  auto alarm_data() const -> const calarm_data*;

  static auto from_interface(void* cg_interface) -> calarm_store*;

private:
  PAD(ext_client::offsets::calarm_store::fields::alarm_data);
  calarm_data* m_alarm_data;
};
