#pragma once
#include "api_status.h"

#include <vector>
#include <map>
#include <utility>

namespace reinforcement_learning {
  class i_trace;
  namespace utility {
  int get_action_count(size_t& count, const char *context, i_trace* trace, api_status* status = nullptr);
  int get_event_ids_and_slot_count(
      const char* context, std::map<size_t, std::string>& event_ids, size_t& count, i_trace* trace, api_status* status);
  int validate_multi_before_slots(const char *context, i_trace* trace, api_status* status = nullptr);
}}
