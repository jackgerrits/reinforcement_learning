#include "multistep_example_joiner.h"

#include "generated/v2/DedupInfo_generated.h"
#include "generated/v2/Event_generated.h"
#include "generated/v2/Metadata_generated.h"
#include "generated/v2/OutcomeEvent_generated.h"
#include "io/logger.h"

#include <limits.h>
#include <time.h>

// VW headers
#include "example.h"
#include "io/logger.h"
#include "parse_example_json.h"
#include "parser.h"
#include "v_array.h"


multistep_example_joiner::multistep_example_joiner(vw *vw)
    : _vw(vw), _reward_calculation(&reward::earliest) {}

multistep_example_joiner::~multistep_example_joiner() {
  // cleanup examples
  for (auto *ex : _example_pool) {
    VW::dealloc_examples(ex, 1);
  }
}

bool multistep_example_joiner::process_event(const v2::JoinedEvent &joined_event) {
  auto event = flatbuffers::GetRoot<v2::Event>(joined_event.event()->data());
  const v2::Metadata& meta = *event->meta();
  auto enqueued_time_utc = timestamp_to_chrono(*joined_event.timestamp());
  switch (meta.payload_type()) {
    case v2::PayloadType_MultiStep:
    {
      auto interaction = flatbuffers::GetRoot<v2::MultiStepEvent>(event->payload()->data());
      _interactions[interaction->event_id()->str()].push_back({enqueued_time_utc, meta, *interaction});
      break;
    }
    case v2::PayloadType_Outcome:
    {
      auto outcome = flatbuffers::GetRoot<v2::OutcomeEvent>(event->payload()->data());
      const char* id =  outcome->index_type() == v2::IndexValue_literal ? outcome->index_as_literal()->c_str() : nullptr;
      if (id == nullptr) {
        _episodic_outcomes.push_back({enqueued_time_utc, meta, *outcome});
      } else {
        _outcomes[std::string(id)].push_back({enqueued_time_utc, meta, *outcome});
      }
      break;    
    }
    default:
      break;
  }
  return true;
}

void multistep_example_joiner::set_default_reward(float default_reward) {
  _default_reward = default_reward;
}

void multistep_example_joiner::set_learning_mode_config(v2::LearningModeType learning_mode) {
  _learning_mode_config = learning_mode;
}

void multistep_example_joiner::set_problem_type_config(v2::ProblemType problem_type) {
  _problem_type_config = problem_type;
}

void multistep_example_joiner::set_reward_function(const v2::RewardFunctionType type) {
  switch (type) {
  case v2::RewardFunctionType_Earliest:
    _reward_calculation = &reward::earliest;
    break;
  case v2::RewardFunctionType_Average:
    _reward_calculation = &reward::average;
    break;

  case v2::RewardFunctionType_Sum:
    _reward_calculation = &reward::sum;
    break;

  case v2::RewardFunctionType_Min:
    _reward_calculation = &reward::min;
    break;

  case v2::RewardFunctionType_Max:
    _reward_calculation = &reward::max;
    break;

  case v2::RewardFunctionType_Median:
    _reward_calculation = &reward::median;
    break;

  default:
    break;
  }
}

void multistep_example_joiner::populate_order() {
  //TODO: topological sort
  for (const auto it: _interactions) {
    _order.push(it.first);
  }
  _sorted = true;
}

outcome_event multistep_example_joiner::process_outcome(const multistep_example_joiner::Parsed<v2::OutcomeEvent> &event_meta) {
  const auto& metadata = event_meta.meta;
  const auto& event = event_meta.event;
  outcome_event o_event;
  o_event.metadata = {timestamp_to_chrono(*metadata.client_time_utc()),
                      metadata.app_id() ? metadata.app_id()->str() : "",
                      metadata.payload_type(),
                      metadata.pass_probability(),
                      metadata.encoding(),
                      metadata.id()->str()};

  if (event.value_type() == v2::OutcomeValue_literal) {
    o_event.s_value = event.value_as_literal()->c_str();
  } else if (event.value_type() == v2::OutcomeValue_numeric) {
    o_event.value = event.value_as_numeric()->value();
  }

  return o_event;
}

joined_event multistep_example_joiner::process_interaction(
    const multistep_example_joiner::Parsed<v2::MultiStepEvent> &event_meta,
    v_array<example *> &examples) {
  const auto& metadata = event_meta.meta;
  const auto& event = event_meta.event;
  metadata_info meta = {metadata.client_time_utc()
                         ? timestamp_to_chrono(*metadata.client_time_utc())
                         : TimePoint(),
                        metadata.app_id() ? metadata.app_id()->str() : "",
                        metadata.payload_type(),
                        metadata.pass_probability(),
                        metadata.encoding(),
                        metadata.id()->str(),
                        v2::LearningModeType::LearningModeType_Online};

  DecisionServiceInteraction data;
  data.eventId = event.event_id()->str();
  data.actions = {event.action_ids()->data(),
                  event.action_ids()->data() + event.action_ids()->size()};
  data.probabilities = {event.probabilities()->data(),
                        event.probabilities()->data() +
                        event.probabilities()->size()};
  data.probabilityOfDrop = 1.f - metadata.pass_probability();
  data.skipLearn = false;//cb->deferred_action();

  std::string line_vec(reinterpret_cast<char const *>(event.context()->data()),
                        event.context()->size());

  if (_vw->audit || _vw->hash_inv) {
    VW::template read_line_json<true>(
        *_vw, examples, const_cast<char *>(line_vec.c_str()),
        reinterpret_cast<VW::example_factory_t>(&VW::get_unused_example), _vw);
  } else {
    VW::template read_line_json<false>(
        *_vw, examples, const_cast<char *>(line_vec.c_str()),
        reinterpret_cast<VW::example_factory_t>(&VW::get_unused_example), _vw);
  }
  return joined_event(event_meta.timestamp, std::move(meta), std::move(data), line_vec, event.model_id() ? event.model_id()->c_str() : "N/A");
}

void try_set_label(const joined_event &je, float reward,
                                   v_array<example *> &examples) {
  if (je.interaction_data.actions.empty()) {
    VW::io::logger::log_error("missing actions for event [{}]",
                              je.interaction_data.eventId);
    return;
  }

  if (je.interaction_data.probabilities.empty()) {
    VW::io::logger::log_error("missing probabilities for event [{}]",
                              je.interaction_data.eventId);
    return;
  }

  if (std::any_of(je.interaction_data.probabilities.begin(),
                  je.interaction_data.probabilities.end(),
                  [](float p) { return std::isnan(p); })) {
    VW::io::logger::log_error(
        "distribution for event [{}] contains invalid probabilities",
        je.interaction_data.eventId);
  }

  int index = je.interaction_data.actions[0];
  auto action = je.interaction_data.actions[0];
  auto cost = -1.f * reward;
  auto probability = je.interaction_data.probabilities[0];
  auto weight = 1.f - je.interaction_data.probabilityOfDrop;

  examples[index]->l.cb.costs.push_back({cost, action, probability});
  examples[index]->l.cb.weight = weight;
}

bool multistep_example_joiner::process_joined(v_array<example *> &examples) {
  if (!_sorted) {
    populate_order();
  }
  const auto& id = _order.front();

  const auto& interactions = _interactions[id];
  if (interactions.size() != 1) {
    return -1;
  }
  const auto& interaction = interactions[0];
  auto joined = process_interaction(interaction, examples);

  const auto outcomes = _outcomes[id];
  for (const auto& o: outcomes) {
    joined.outcome_events.push_back(process_outcome(o));
  }
  for (const auto& o: _episodic_outcomes) {
    joined.outcome_events.push_back(process_outcome(o));
  }
  const auto reward = _reward_calculation(joined);
  try_set_label(joined, reward, examples);
  
  // add an empty example to signal end-of-multiline
  examples.push_back(&VW::get_unused_example(_vw));
  _vw->example_parser->lbl_parser.default_label(&examples.back()->l);
  examples.back()->is_newline = true;

  _order.pop();
  return true;
}

bool multistep_example_joiner::processing_batch() {
  return _sorted && !_order.empty();
}

void multistep_example_joiner::on_new_batch() {
  _interactions.clear();
  _outcomes.clear();
  _episodic_outcomes.clear();
  _sorted = false;
}

void multistep_example_joiner::on_batch_read() {
  populate_order();
  _sorted = true;
}
