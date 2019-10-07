#define BOOST_TEST_DYN_LINK
#ifdef STAND_ALONE
#define BOOST_TEST_MODULE Main
#endif

#include <boost/test/unit_test.hpp>

#include "event_builder.h"
#include "configuration.h"
#include "config_utility.h"
#include "constants.h"

BOOST_AUTO_TEST_CASE(json_serialize_cb_problem)
{
  problem_data<cb_problem_type> prob({ feature_space{{feat_namespace{"a", {feature{"a", 0.5}}}}} });
  feature_space shared(std::vector<feat_namespace>{ { {"one", { {"test"} }}}});
  problem_event<cb_problem_type> event("eventid");
  event.shared_context(shared);
  event.event_problem(prob);

  auto result = event.emit<json_serializer>();
  BOOST_CHECK_EQUAL(result, R"({"one":{"test":1},"_multi":[{"a":{"a":0.5}}]})");
}

BOOST_AUTO_TEST_CASE(json_serialize_slate_problem)
{
  problem_data<slate_problem_type> prob(std::vector<slate_slot>{
    {
      feature_space(),
      {
        feature_space{{feat_namespace{"a", {feature{"a", 0.5}}}}}
      }
    }
    });
  feature_space shared(std::vector<feat_namespace>{ { {"one", { {"test"} }}}});
  problem_event<slate_problem_type> event("eventid");
  event.shared_context(shared);
  event.event_problem(prob);

  auto result = event.emit<json_serializer>();
  BOOST_CHECK_EQUAL(result, R"({"one":{"test":1},"_multi":[{"a":{"a":0.5}}],"_slots":[{"_inc":[0]}]})");
}

template<typename T>
std::string to_string(T const& item)
{
  std::ostringstream ss;
  json_serializer::serialize(ss, item);
  return ss.str();
}

BOOST_AUTO_TEST_CASE(json_serialize_feature)
{
  BOOST_CHECK_EQUAL(to_string(feature("f", 1.0)), R"("f":1)");
  BOOST_CHECK_EQUAL(to_string(feature("f", 1.5)), R"("f":1.5)");
  BOOST_CHECK_EQUAL(to_string(feature("f")), R"("f":1)");
}

BOOST_AUTO_TEST_CASE(json_serialize_namespace)
{
  BOOST_CHECK_EQUAL(to_string(feat_namespace("test", { {"f", 1.0} })), R"("test":{"f":1})");
  BOOST_CHECK_EQUAL(to_string(feat_namespace({ {"f", 1.0}, {"g", 4.5} })), R"("f":1,"g":4.5)");
}

BOOST_AUTO_TEST_CASE(json_serialize_feature_space)
{
  BOOST_CHECK_EQUAL(to_string(feature_space({ { "test", { {"f", 1.0} } } })), R"("test":{"f":1})");
  BOOST_CHECK_EQUAL(to_string(feature_space({ { "test", { {"f", 1.0} } }, { "test", { {"f", 1.0} } } })), R"("test":{"f":1},"test":{"f":1})");
  BOOST_CHECK_EQUAL(to_string(feature_space({ { "test", { {"f", 1.0} } }, { { {"f", 1.0} } },{ { {"g", 5.5} } } })), R"("test":{"f":1},"f":1,"g":5.5)");
  BOOST_CHECK_EQUAL(to_string(feature_space({ { "test", { {"f", 1.0} } }, { { {"f", 1.0} } },{ { {"g", 5.5} } }, { "another", { {"feat", -1.0} } } })), R"("test":{"f":1},"f":1,"g":5.5,"another":{"feat":-1})");
}

BOOST_AUTO_TEST_CASE(json_serialize_slot)
{
  BOOST_CHECK_EQUAL(to_string(slot()), "");
  BOOST_CHECK_EQUAL(to_string(slot("event_id")), R"("_id":"event_id")");
  BOOST_CHECK_EQUAL(to_string(slot({ 1,2,3 }, "event_id")), R"("_a":[1,2,3],"_id":"event_id")");
  BOOST_CHECK_EQUAL(to_string(slot(feature_space({ { "test", { {"f", 1.0} } } }))), R"("test":{"f":1})");
  BOOST_CHECK_EQUAL(to_string(slot(feature_space({ { "test", { {"f", 1.0} } } }), std::vector<uint32_t>{ 1, 2, 3 })), R"("test":{"f":1},"_inc":[1,2,3])");
  BOOST_CHECK_EQUAL(to_string(slot(feature_space({ { "test", { {"f", 1.0} } } }), { 1,2,3 }, "event_id")), R"("test":{"f":1},"_inc":[1,2,3],"_id":"event_id")");
  BOOST_CHECK_EQUAL(to_string(slot(feature_space({ {{ {"f", 1.f}, {"g_feat", -5.3f} } } }), { 1,2,3 }, "event_id")), R"("f":1,"g_feat":-5.3,"_inc":[1,2,3],"_id":"event_id")");
}

BOOST_AUTO_TEST_CASE(json_serialize_builder)
{
  feat_namespace ns1("TAction", { {"f", 1.0} });
  feat_namespace ns2({ {"f", 1.0}, {"g", 5.5} });
  feature_space action1({ ns1, ns2 });
  feature_space action2({ ns1, ns2 });

  feature_space shared_context;
  feat_namespace shared_ns("TUser");
  shared_ns.push_feature("f", 5.6f);
  shared_context.push_namespace(shared_ns);

  slot slot1;
  slot1.slot_context = feature_space({ {"TSlot", {{"slot_feature"}} } });
  slot1._id = "event_id";

  builder b;
  b.actions = { action1, action2 };
  b.shared_context = shared_context;
  b.slots = { slot1 };

  auto value = b.emit<json_serializer>();
  BOOST_CHECK_EQUAL(value, R"({"TUser":{"f":5.6},"_multi":[{"TAction":{"f":1},"f":1,"g":5.5},{"TAction":{"f":1},"f":1,"g":5.5}],"_slots":[{"TSlot":{"slot_feature":1},"_id":"event_id"}]})");
}

BOOST_AUTO_TEST_CASE(slate_slot_test)
{
  namespace rlname = reinforcement_learning::name;
  namespace rlvalue = reinforcement_learning::value;
  using reinforcement_learning::error_code::success;

  reinforcement_learning::utility::configuration config;
  config.set(rlname::APP_ID, "slate_unit_test");
  config.set(rlname::INTERACTION_SENDER_IMPLEMENTATION, rlvalue::INTERACTION_FILE_SENDER);
  config.set(rlname::OBSERVATION_SENDER_IMPLEMENTATION, rlvalue::OBSERVATION_FILE_SENDER);
  config.set(rlname::MODEL_SRC, rlvalue::NO_MODEL_DATA);

  reinforcement_learning::live_model lm(config);
  BOOST_CHECK_EQUAL(lm.init(), success);
  estimator est(&lm);

  feature_space shared_context;
  feat_namespace shared_ns("TUser");
  shared_ns.push_feature("f", 5.6f);
  shared_context.push_namespace(shared_ns);

  std::vector<feature_space> actions;
  feat_namespace ns1("TAction", { {"f", 1.0} });
  feat_namespace ns2({ {"f", 1.0}, {"g", 5.5} });
  feature_space action1({ ns1, ns2 });
  feature_space action2({ ns1, ns2 });

  problem_data<slate_problem_type> prob_data;
  prob_data.push_slot({ shared_context, { action1, action2 } });
  prob_data.push_slot({ shared_context, { action1, action2 } });
  prob_data.push_slot({ shared_context, { action1, action2 } });

  problem_event<slate_problem_type> prob;
  prob.shared_context(shared_context);
  prob.event_problem(prob_data);

  auto ret = est.predict_and_log(prob);
  size_t chosen;
  auto it = ret.begin();

  // Since we are running in no model mode, it just gives us back the first one each time.
  // As part of slates we transform the action indices back into the original spaces that they were supplied as.
  // Before translation these 3 ids would have been 0,2,4
  BOOST_CHECK_EQUAL(it->get_chosen_action_id(chosen), success);
  BOOST_CHECK_EQUAL(chosen, 0);
  it++;

  BOOST_CHECK_EQUAL(it->get_chosen_action_id(chosen), success);
  BOOST_CHECK_EQUAL(chosen, 0);
  it++;

  BOOST_CHECK_EQUAL(it->get_chosen_action_id(chosen), success);
  BOOST_CHECK_EQUAL(chosen, 0);
}
