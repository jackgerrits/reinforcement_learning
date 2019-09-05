#define BOOST_TEST_DYN_LINK
#ifdef STAND_ALONE
#define BOOST_TEST_MODULE Main
#endif

#include <boost/test/unit_test.hpp>

#include "event_builder.h"

BOOST_AUTO_TEST_CASE(json_cb_problem_type)
{
  cb_problem_type::instance<json_representation> prob({{{{"a", {{"a", 0.5}}}}}});
  auto str = prob.emit();
  BOOST_CHECK_EQUAL(str, R"("_multi":[{"a":{"a":0.5}}])");

  cb_problem_type::instance<json_representation> prob2({{{{"a", {{"a", 0.5}}}, {"b", {{"a"}}}}},
                                                        {{{"other", {{"new", 0.5}}}, {"b", {{"a", "b"}}}}}});
  auto str2 = prob2.emit();
  BOOST_CHECK_EQUAL(str2, R"("_multi":[{"a":{"a":0.5},"b":{"a":1}},{"other":{"new":0.5},"b":{"ab":1}}])");
}
