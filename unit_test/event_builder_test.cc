#define BOOST_TEST_DYN_LINK
#ifdef STAND_ALONE
#define BOOST_TEST_MODULE Main
#endif

#include <boost/test/unit_test.hpp>

#include "event_builder.h"


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
  problem_data<slate_problem_type> prob({
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
  BOOST_CHECK_EQUAL(result, R"({"one":{"test":1},"_multi":[{"a":{"a":0.5}}],"_slots":[{"_a":[0]}]})");
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
  BOOST_CHECK_EQUAL(to_string(slot(feature_space({ { "test", { {"f", 1.0} } } }), std::vector<uint32_t>{ 1, 2, 3 })), R"("test":{"f":1},"_a":[1,2,3])");
  BOOST_CHECK_EQUAL(to_string(slot(feature_space({ { "test", { {"f", 1.0} } } }), { 1,2,3 }, "event_id")), R"("test":{"f":1},"_a":[1,2,3],"_id":"event_id")");
  BOOST_CHECK_EQUAL(to_string(slot(feature_space({ {{ {"f", 1.0}, {"g_feat", -5.3} } } }), { 1,2,3 }, "event_id")), R"("f":1,"g_feat":-5.3,"_a":[1,2,3],"_id":"event_id")");
}

BOOST_AUTO_TEST_CASE(json_serialize_builder)
{
  builder b;
  b.actions = { 
    feature_space{{
      { "test", { {"f", 1.0} } },
      { { {"f", 1.0} } },{ { {"g", 5.5} } },
      { "another", { {"feat", -1.0} } } }
    },
    feature_space{{
      { "test", { {"f", 1.0} } },
      { { {"f", 1.0} } },{ { {"g", 5.5} } },
      { "another", { {"feat", -1.0} } } }
    },
  };
}