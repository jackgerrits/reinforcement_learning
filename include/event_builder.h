#pragma once
#include <iostream>
#include <string>
#include <numeric>

#include "live_model.h"
#include "ranking_response.h"
#include "decision_response.h"


struct cb_problem_type
{
  using response_t = reinforcement_learning::ranking_response;
};

// struct ccb_problem_type
// {
//   using response_t = reinforcement_learning::decision_response;
// };

struct slate_problem_type
{
  using response_t = reinforcement_learning::decision_response;
};

struct feature
{
  feature() = default;
  feature(std::string const& name, float value) : name(name), value(value) {}
  feature(std::string const& name, std::string const& value) : name(name + value) {}
  feature(std::string const& name) : name(name) {}

  std::string name = "";
  float value = 1.0f;
};

struct continuous_action
{
  continuous_action(std::string const& ref) : ref(ref) {}
  std::string ref;
};

struct feat_namespace
{
  feat_namespace() = default;
  feat_namespace(std::string const& name)  : name(name){}
  feat_namespace(std::vector<feature> const& features) : features(features) {}
  feat_namespace(std::string const& name, std::vector<feature> const& features) : name(name), features(features) {}

  void push_feature(feature const& ns)
  {
    features.push_back(ns);
  }

  void push_feature(feature const&& ns)
  {
    features.emplace_back(ns);
  }

  void push_feature(std::string const& name, float value)
  {
    features.emplace_back(name, value);
  }

  void push_feature(std::string const& name)
  {
    features.emplace_back(name);
  }

  std::string name = "";
  std::vector<feature> features;
};

struct feature_space
{
  feature_space() = default;
  feature_space(std::vector<feat_namespace> const& namespaces) : namespaces(namespaces) {}

  void push_namespace(feat_namespace const& ns)
  {
    namespaces.push_back(ns);
  }

  void push_namespace(feat_namespace const&& ns)
  {
    namespaces.emplace_back(ns);
  }

  void push_namespace(std::string const& name, std::vector<feature> const& features)
  {
    namespaces.emplace_back(name, features);
  }

  void push_namespace(std::vector<feature> const& features)
  {
    namespaces.emplace_back(features);
  }

  std::vector<feat_namespace> namespaces;
};

struct slot
{
  slot() = default;
  slot(feature_space const& context, std::vector<uint32_t> const& actions, std::string const& id) : slot_context(context), _explicit_included_actions(actions), _id(id) {}
  slot(feature_space const& context, std::vector<uint32_t> const& actions) : slot_context(context), _explicit_included_actions(actions) {}
  slot(std::vector<uint32_t> const& actions, std::string const& id) : _explicit_included_actions(actions), _id(id) {}
  slot(feature_space const& context) : slot_context(context) {}
  slot(feature_space const& context, std::string const& id) : slot_context(context), _id(id) {}
  slot(std::vector<uint32_t> const& actions) : _explicit_included_actions(actions) {}
  slot(std::string const& id) : _id(id) {}

  feature_space slot_context;
  std::vector<uint32_t> _explicit_included_actions;
  std::string _id = "";
};


struct builder
{
  builder() = default;

  template <typename TSerializer>
  typename TSerializer::serialized_t emit()
  {
    return TSerializer::serialize(shared_context, actions, slots);
  }

  feature_space shared_context;
  std::vector<feature_space> actions;
  std::vector<slot> slots;
};

struct json_serializer
{
  using serialized_t = std::string;

  static serialized_t serialize(feature_space const& shared, std::vector<feature_space> const& actions, std::vector<slot> const& slots)
  {
    std::ostringstream ss;
    ss << "{";
    serialize(ss, shared);
    ss << R"(,"_multi":[)";
    for (auto& action : actions)
    {
      ss << "{";
      serialize(ss, action);
      ss << "},";
    }
    ss.seekp(-1, std::ios_base::end);
    ss << R"(])";

    if (slots.size() > 0)
    {
      ss << R"(,"_slots":[)";
      for (auto& slot : slots)
      {
        ss << "{";
        serialize(ss, slot);
        ss << "},";
      }
      ss.seekp(-1, std::ios_base::end);
      ss << R"(])";
    }

    ss << "}";
    return ss.str();
  }

  static serialized_t serialize(builder const& b)
  {
    return serialize(b.shared_context, b.actions, b.slots);
  }

  static void serialize(std::ostringstream& ss, slot const& s)
  {

    auto i = ss.tellp();
    serialize(ss, s.slot_context);
    auto i2 = ss.tellp();


    if (s._explicit_included_actions.size() > 0)
    {
      if (i2 > i)
      {
        ss << ",";
      }

      ss << R"("_inc":[)";
      bool first_run = true;

      for (auto& x : s._explicit_included_actions)
      {
        if (first_run)
        {
          first_run = false;
        }
        else
        {
          ss << ",";
        }
        ss << x;

      }
      ss << "]";
    }

    if (s._id != "")
    {
      if (s._explicit_included_actions.size() > 0 || i2 > i)
      {
        ss << ",";
      }
      ss << R"("_id":")" << s._id << "\"";
    }
  }

  static void serialize(std::ostringstream& ss, feature_space const& fs)
  {
    bool first_run = true;
    if (fs.namespaces.size() > 0)
    {
      for (auto& ns : fs.namespaces)
      {
        if (first_run)
        {
          first_run = false;
        }
        else
        {
          ss << ",";
        }
        serialize(ss, ns);
      }
    }
  }

  static void serialize(std::ostringstream& ss, feat_namespace const& fn)
  {
    if (fn.name != "")
    {
      ss << "\"" << fn.name << R"(":{)";
    }
    bool first_run = true;
    for (auto& feature : fn.features)
    {
      if (first_run)
      {
        first_run = false;
      }
      else
      {
        ss << ",";
      }
      serialize(ss, feature);
    }
    if (fn.name != "")
    {
      ss << "}";
    }
  }

  static void serialize(std::ostringstream& ss, feature const& f)
  {
    ss << R"(")" << f.name << R"(":)" << f.value;
  }
};

template <typename TProblemType>
struct problem_data
{
  // Specializations MUST be used.
  problem_data() = delete;
};

struct slate_slot
{
  // ActionSets for Slates do not overlap, so the provided interface only allows for that.
  slate_slot(feature_space const& shared_context, std::vector<feature_space> actions) : shared_context(shared_context), actions(actions) {}
  slate_slot(feature_space const& shared_context) : shared_context(shared_context) {}

  void push_action(feature_space const& a)
  {
    actions.push_back(a);
  }

  void push_action(std::vector<feat_namespace> const& namespaces)
  {
    actions.emplace_back(namespaces);
  }

  feature_space shared_context;
  std::vector<feature_space> actions;
};

template <>
struct problem_data<cb_problem_type>
{
  problem_data(std::vector<feature_space> const& actions) : actions(actions) {}
  void add_to(builder& b) const
  {
    for (auto& action : actions)
    {
      b.actions.push_back(action);
    }
  }
  std::vector<feature_space> actions;

};

template <>
struct problem_data<slate_problem_type>
{
  problem_data() = default;
  problem_data(std::vector<slate_slot> const& slots) : slots(slots) {}

  void push_slot(slate_slot const& s)
  {
    slots.push_back(s);
  }

  void push_slot(feature_space const& shared_context, std::vector<feature_space> actions)
  {
    slots.emplace_back(shared_context, actions);
  }

  void add_to(builder& b) const
  {
    for (auto& slot : slots)
    {
      uint32_t current_last_index_plus_one = static_cast<uint32_t>(b.actions.size());
      for (auto& action : slot.actions)
      {
        b.actions.push_back(action);
      }

      std::vector<uint32_t> indicies(slot.actions.size());
      std::iota(indicies.begin(), indicies.end(), current_last_index_plus_one);

      b.slots.push_back({ slot.shared_context, indicies });
    }
  }

  std::vector<slate_slot> slots;

};

template <typename TProblemType>
struct problem_event
{
  problem_event() {}
  problem_event(std::string const& id) : id(id) {}

  void shared_context(feature_space const& shared_context)
  {
    b.shared_context = shared_context;
  }

  void event_problem(problem_data<TProblemType> const& problem_data)
  {
    problem_data.add_to(b);
  }

  template <typename TRepresentation>
  typename TRepresentation::serialized_t emit() const
  {
    return TRepresentation::serialize(b);
  }

  std::string id;
  builder b;
};

struct estimator
{
  estimator() = default;
  estimator(reinforcement_learning::live_model* lm) : lm(lm) {}

  template <typename TProblemType, typename... Ts>
  typename TProblemType::response_t predict_and_log(problem_event<TProblemType> const& instance, Ts&& ...) = delete;

  reinforcement_learning::live_model* lm;
};

template <>
inline slate_problem_type::response_t estimator::predict_and_log<slate_problem_type>(problem_event<slate_problem_type> const& instance)
{
  auto context = instance.emit<json_serializer>();
  reinforcement_learning::decision_response response;
  lm->request_decision(context.c_str(), response);

  int index = 0;
  int accumulated_size = 0;
  for (auto& item : response)
  {
    size_t id;
    item.get_chosen_action_id(id);
    item.set_chosen_action_id(id - accumulated_size);

    accumulated_size += instance.b.slots[index]._explicit_included_actions.size();
    index++;
  }

  return response;
}
