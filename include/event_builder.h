#pragma once
#include <iostream>
#include <string>

#include "live_model.h"
#include "ranking_response.h"

struct delete_default_constructor
{
  delete_default_constructor() = delete;
};

struct cb_problem_type
{
  using response_t = reinforcement_learning::ranking_response;
};

struct json_representation
{
  using emit_type = std::string;

  struct feature
  {
    feature(std::string name, float value) : str(name), value(value) {}
    feature(std::string name, std::string value) : str(name + value) {}
    feature(std::string name) : str(name) {}

    std::string emit() const
    {
      std::stringstream ss;
      ss << R"(")" << str << R"(":)" << value;
      return ss.str();
    }

    std::string str;
    float value = 1.0f;
  };

  struct continuous_action
  {
    continuous_action(std::string ref) : ref(ref) {}
    std::string ref;
  };

  struct feat_namespace
  {
    feat_namespace(std::vector<feature> features) : features(features) {}
    feat_namespace(std::string name, std::vector<feature> features) : str(name), features(features) {}

    std::string emit() const
    {
      std::stringstream ss;
      ss << R"(")" << str << R"(":{)";
      for (auto &feature : features)
      {
        ss << feature.emit() << R"(,)";
      }
      ss.seekp(-1, std::ios_base::end);
      ss << R"(})";
      return ss.str();
    }

    std::string str = "";
    std::vector<feature> features;
  };

  struct feature_space
  {
    feature_space(std::vector<feat_namespace> namespaces) : namespaces(namespaces) {}

    std::string emit() const
    {
      std::stringstream ss;
      for (auto &ns : namespaces)
      {
        ss << ns.emit() << ",";
      }
      auto ret = ss.str();
      ret.pop_back();
      return ret;
    }

    std::vector<feat_namespace> namespaces;
  };

  struct slot
  {
    slot(feature_space context, std::vector<int> actions) : namespaces(namespaces) {}

    std::string emit() const
    {
      // std::stringstream ss;
      // for (auto &ns : namespaces)
      // {
      //   ss << ns.emit() << ",";
      // }
      // auto ret = ss.str();
      // ret.pop_back();
      // return ret;
    }

    std::vector<feat_namespace> namespaces;
  };

  template <typename TProblemType>
  struct problem_data
  {
  };

  template <typename TProblemType>
  static std::string emit(std::string id, feature_space shared_context, problem_data<TProblemType>);
};

template <>
struct json_representation::problem_data<cb_problem_type>
{
  problem_data(std::vector<feature_space> actions) : actions(actions){};

  std::string emit() const
  {
    std::stringstream ss;
    ss << R"("_multi":[)";
    for (auto &action : actions)
    {
      ss << "{" << action.emit() << "},";
    }
    ss.seekp(-1, std::ios_base::end);
    ss << R"(])";

    return ss.str();
  }

  std::vector<feature_space> actions;
};

template <>
std::string json_representation::emit<cb_problem_type>(std::string id, feature_space shared_context, problem_data<cb_problem_type>);

struct vw_representation
{
};

struct delete_default_constructor
{
  delete_default_constructor() = delete;
};

template <typename TProblemType, typename TRepresentation>
struct problem_event
{
  problem_event();
  problem_event(std::string id);

  void shared_context(typename TRepresentation::feature_space &shared_context)
  {
    m_shared_context = shared_context;
  }

  void event_problem(typename TRepresentation::template problem_data<TProblemType> &problem_data)
  {
    m_problem_data = problem_data;
  }

  typename TRepresentation::emit_type emit() const
  {
    return TRepresentation::emit(id, m_shared_context, m_problem_data);
  }

  std::string id;
  typename TRepresentation::feature_space m_shared_context;
  typename TRepresentation::template problem_data<TProblemType> m_problem_data;
};



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

template <typename TRepresentation>
struct solver : delete_default_constructor
{
};

template <>
struct solver<json_representation>
{
  solver();
  solver(reinforcement_learning::live_model *lm);

  template <typename TProblemType, typename... Ts>
  typename TProblemType::response_t predict_and_log(problem_event<TProblemType, json_representation> &instance, Ts &&...) = delete;

  reinforcement_learning::live_model *lm;
};

template <>
inline cb_problem_type::response_t solver<json_representation>::predict_and_log<cb_problem_type>(problem_event<cb_problem_type, json_representation> &instance)
{
  auto str = instance.emit();
  cb_problem_type::response_t resp;
  lm->choose_rank(instance.id.c_str(), str.c_str(), resp);
  return resp;
}
