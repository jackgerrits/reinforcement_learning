#include "joiner.h"

#include <string>
#include <vector>
#include <fstream>
#include <flatbuffers/flatbuffers.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <map>

#include "../../rlclientlib/logger/preamble.h"
#include "../../rlclientlib/logger/message_type.h"
#include "../../rlclientlib/generated/RankingEvent_generated.h"
#include "../../rlclientlib/generated/OutcomeEvent_generated.h"
#include "../../rlclientlib/generated/DecisionRankingEvent_generated.h"

namespace rlog = reinforcement_learning::logger;
namespace rl = reinforcement_learning;

using outcome_t = float;

constexpr float default_reward = 0;

std::ifstream get_stream(const std::string& file)
{
  std::ifstream infile;
  infile.open(file, std::ios_base::binary);
  if (infile.fail() || infile.bad())
  {
    std::cout << "Unable to open file: " << file << std::endl;
  }
  return infile;
}

template <typename T>
void for_each_item_in_log(std::istream& in_strm, T func)
{
  do
  {
    if (in_strm.fail() || in_strm.bad())
    {
      std::cerr << "Error in input stream." << std::endl;
      return;
    }

    char raw_preamble[8];
    in_strm.read(raw_preamble, 8);
    rlog::preamble p;
    p.read_from_bytes(reinterpret_cast<uint8_t*>(raw_preamble), 8);
    std::unique_ptr<char> msg_data(new char[p.msg_size]);
    in_strm.read(msg_data.get(), p.msg_size);
    if (in_strm.fail() || in_strm.bad())
    {
      continue;
    }

    func(p, msg_data.get());
  } while (!in_strm.fail() && !in_strm.bad());
}

std::map<std::string, std::vector<outcome_t>> get_outcome_events(std::istream& in_strm)
{
  std::map<std::string, std::vector<outcome_t>> outcomes;
  for_each_item_in_log(in_strm, [&outcomes](rlog::preamble& p, void* data) {
    if (p.msg_type == rlog::message_type::fb_outcome_event_collection)
    {
      const auto batch = rl::messages::flatbuff::GetOutcomeEventBatch(data);
      for (auto const event : *batch->events())
      {
        if (event->the_event_type() == rl::messages::flatbuff::OutcomeEvent::OutcomeEvent_NumericEvent)
        {
          auto event_id = event->event_id()->str();
          const auto number = event->the_event_as_NumericEvent();
          outcomes[event_id].emplace_back(number->value());
        }
      }
    }
  });

  return outcomes;
}

std::string join_and_serialize(const rl::messages::flatbuff::DecisionEvent* decision_event,
    std::map<std::string, std::vector<outcome_t>> const& outcomes)
{
  std::stringstream ss;
  ss << R"({"Version":"1")";

  std::string context(
      reinterpret_cast<const char*>(decision_event->context()->data()), decision_event->context()->size());
  ss << R"(,"c":)" << context << R"(,"_outcomes":[)";

  auto delimiter = "";
  for (auto const slot : *decision_event->slots())
  {
    ss << delimiter << "{";
    auto slot_event_id = slot->decision_slot_id()->str();
    ss << R"("_id":")" << slot_event_id << "\",";

    auto reward = default_reward;
    auto const decision_outcomes_iterator = outcomes.find(slot_event_id);
    if (decision_outcomes_iterator != outcomes.end())
    {
      // Reward function used is 0th.
      reward = decision_outcomes_iterator->second[0];
    }
    // Reward must be negated to become cost.
    ss << R"("_label_cost":)" << -1 * reward << R"(,"_a":[)";
    auto delimiter_inner = "";
    for (auto const& action_id : *slot->action_ids())
    {
      ss << delimiter_inner << action_id;
      delimiter_inner = ",";
    }
    ss << R"(],"_p":[)";
    delimiter_inner = "";
    for (auto const& probability : *slot->probabilities())
    {
      ss << delimiter_inner << probability;
      delimiter_inner = ",";
    }
    ss << R"(],"_o":[)";
    delimiter_inner = "";
    if (decision_outcomes_iterator != outcomes.end())
    {
      for (auto const& outcome : decision_outcomes_iterator->second)
      {
        ss << delimiter_inner << outcome;
        delimiter_inner = ",";
      }
    }

    ss << R"(]})";
    delimiter = ",";
  }

  const char* model_id = decision_event->model_id()->data();
  ss << R"(],"VWState":{"m":")" << model_id << R"("}})";
  return ss.str();
}

void join_ccb(std::string const& decisions_file_name, std::string const& outcomes_file_name)
{
  auto decisions_stream = get_stream(decisions_file_name);
  auto outcomes_stream = get_stream(outcomes_file_name);
  auto const outcome_events = get_outcome_events(outcomes_stream);

  for_each_item_in_log(decisions_stream, [&outcome_events](rlog::preamble& p, void* data) {
    if (p.msg_type == rlog::message_type::fb_decision_event_collection)
    {
      const auto batch = rl::messages::flatbuff::GetDecisionEventBatch(data);
      for (auto const event : *batch->events())
      {
        std::cout << join_and_serialize(event, outcome_events) << std::endl;
      }
    }
  });
}
