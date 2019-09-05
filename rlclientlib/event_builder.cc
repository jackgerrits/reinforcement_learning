#include "event_builder.h"


// template <>
// cb_problem_type::response_t solver<json_representation>::predict_and_log<cb_problem_type>(problem_event<cb_problem_type, json_representation> &instance)
// {
//   auto str = instance.emit();
//   cb_problem_type::response_t resp;
//   lm->choose_rank(instance.id.c_str(), str.c_str(), resp);
//   return resp;
// }

// class logger
// {
//   template <typename TProblemType>
//   void log_response(TProblemType::response resp);

//   template <typename TProblemType>
//   void log_reward(TProblemType::response resp, float reward);
//   template <typename TProblemType>
//   void log_reward(std::string event_id, float reward);
// };

//class SlateProblemType
//{
//  template <typename TRepresentation>
//  class instance {
//    instance(std::vector<SlateSlot<TRepresentation>> slots);
//  };
//
//  class response {
//  };
//
//  template <typename TRepresentation>
//  class SlateSlot {
//    SlateSlot(feature_space shared_context, std::vector<feature_space>);
//    SlateSlot(feature_space shared_context, continuous_action);
//  };
//
//  template<>
//  class instance<json_representation> {
//    instance(std::vector<SlateSlot<TRepresentation>> slots);
//  };
//
//  template <>
//  class SlateSlot<json_representation> {
//    SlateSlot(feature_space shared_context, std::vector<feature_space>);
//    SlateSlot(feature_space shared_context, continuous_action);
//  };
//};

//class problem_event<SlateProblemType, json_representation>
//{
//  problem_event();
//  problem_event(std::string id);
//
//  void shared_context(feature_space shared_context);
//  void event_problem(SlateProblemType::Instance<json_representation>);
//  std::string emit();
//
//  std::string;
//  feature_space shared_context;
//};
