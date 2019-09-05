#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <thread>

#include "live_model.h"
#include "rl_sim_cpp.h"
#include "person.h"
#include "simulation_stats.h"
#include "constants.h"

using namespace std;

std::string get_dist_str(const reinforcement_learning::ranking_response& response);

int rl_sim::loop() {
  if ( !init() ) return -1;
  if(ccb_mode)
  {
    return ccb_loop();
  }
  else {
    return cb_loop();
  }
}

int rl_sim::cb_loop() {
  r::ranking_response response;
  simulation_stats stats;

  while ( _run_loop ) {
    auto& p = pick_a_random_person();
    const auto context_features = p.get_features();
    const auto action_features = get_action_features();
    const auto context_json = create_context_json(context_features,action_features);
    const auto req_id = create_event_id();
    r::api_status status;

    solver<json_representation> s(_rl.get());
    s.predict_and_log(event);

    // Choose an action
    // if ( _rl->choose_rank(req_id.c_str(), context_json.c_str(), response, &status) != err::success ) {
    //   std::cout << status.get_error_msg() << std::endl;
    //   continue;
    // }

    // Use the chosen action
    size_t chosen_action;
    if ( response.get_chosen_action_id(chosen_action) != err::success ) {
      std::cout << status.get_error_msg() << std::endl;
      continue;
    }

    // What outcome did this action get?
    const auto outcome = p.get_outcome(_topics[chosen_action]);

    // Report outcome received
    if ( _rl->report_outcome(req_id.c_str(), outcome, &status) != err::success && outcome > 0.00001f ) {
      std::cout << status.get_error_msg() << std::endl;
      continue;
    }

    stats.record(p.id(), chosen_action, outcome);

    std::cout << " " << stats.count() << ", ctxt, " << p.id() << ", action, " << chosen_action << ", outcome, " << outcome
      << ", dist, " << get_dist_str(response) << ", " << stats.get_stats(p.id(), chosen_action) << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  }

  return 0;
}

int rl_sim::ccb_loop() {
  r::decision_response decision;
  simulation_stats stats;

  while ( _run_loop ) {
    auto& p = pick_a_random_person();
    const auto context_features = p.get_features();
    const auto action_features = get_action_features();

    std::vector<std::string> ids;
    for(int i = 0; i < NUM_SLOTS; i++)
    {
      ids.push_back(create_event_id());
    }

    const auto slot_json =  get_slot_features(ids);
    const auto context_json = create_context_json(context_features,action_features, slot_json);
    std::cout << context_json <<std::endl;
    r::api_status status;

    // Choose an action
    if ( _rl->request_decision(context_json.c_str(), decision, &status) != err::success ) {
      std::cout << status.get_error_msg() << std::endl;
      continue;
    }

    auto index = 0;
    for(auto& response : decision)
    {
      size_t chosen_action;
      if ( response.get_chosen_action_id(chosen_action) != err::success ) {
        std::cout << status.get_error_msg() << std::endl;
        continue;
      }

      const auto outcome = p.get_outcome(_topics[chosen_action]);

      // Report outcome received
      if ( _rl->report_outcome(ids[index].c_str(), outcome, &status) != err::success && outcome > 0.00001f ) {
        std::cout << status.get_error_msg() << std::endl;
        continue;
      }

      stats.record(p.id(), chosen_action, outcome);

      std::cout << " " << stats.count() << ", ctxt, " << p.id() << ", action, " << chosen_action << ", slot, " << index << ", outcome, " << outcome
        << ", dist, " << get_dist_str(response) << ", " << stats.get_stats(p.id(), chosen_action) << std::endl;
      index++;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  }

  return 0;
}

person& rl_sim::pick_a_random_person() {
  return _people[rand() % _people.size()];
}

int rl_sim::load_config_from_json(  const std::string& file_name,
                                    u::configuration& config,
                                    r::api_status* status) {
  std::string config_str;

  // Load contents of config file into a string
  RETURN_IF_FAIL(load_file(file_name, config_str));

  // Use library supplied convenience method to parse json and build config object
  return cfg::create_from_json(config_str, config, nullptr, status);
}

int rl_sim::load_file(const std::string& file_name, std::string& config_str) {
  std::ifstream fs;
  fs.open(file_name);
  if ( !fs.good() ) return err::invalid_argument;
  std::stringstream buffer;
  buffer << fs.rdbuf();
  config_str = buffer.str();
  return err::success;
}

void _on_error(const reinforcement_learning::api_status& status, rl_sim* psim) {
  psim->on_error(status);
}

int rl_sim::init_rl() {
  r::api_status status;
  u::configuration config;

  // Load configuration from json config file
  const auto config_file = _options["json_config"].as<std::string>();
  if ( load_config_from_json(config_file, config, &status) != err::success ) {
    std::cout << status.get_error_msg() << std::endl;
    return -1;
  }

  if(_options["log_to_file"].as<bool>()) {
    config.set(r::name::INTERACTION_SENDER_IMPLEMENTATION, r::value::INTERACTION_FILE_SENDER);
    config.set(r::name::OBSERVATION_SENDER_IMPLEMENTATION, r::value::OBSERVATION_FILE_SENDER);
  }

  if (!_options["get_model"].as<bool>()) {
    // Set the time provider to the clock time provider
    config.set(r::name::MODEL_SRC, r::value::NO_MODEL_DATA);
  }

  if (_options["log_timestamp"].as<bool>()) {
    // Set the time provider to the clock time provider
    config.set(r::name::TIME_PROVIDER_IMPLEMENTATION, r::value::CLOCK_TIME_PROVIDER);
  }

  // Trace log API calls to the console
  config.set(r::name::TRACE_LOG_IMPLEMENTATION, r::value::CONSOLE_TRACE_LOGGER);

  // Initialize the API
  _rl = std::unique_ptr<r::live_model>(new r::live_model(config,_on_error,this));
  if ( _rl->init(&status) != err::success ) {
    std::cout << status.get_error_msg() << std::endl;
    return -1;
  }

  std::cout << " API Config " << config;

  return err::success;
}

bool rl_sim::init_sim_world() {

  //  Initilize topics
  _topics = {
    "SkiConditions-VT",
    "HerbGarden",
    "BeyBlades",
    "NYCLiving",
    "MachineLearning"
  };

  // Initialize click probability for p1
  person::topic_prob tp = {
    { _topics[0],0.08f },
    { _topics[1],0.03f },
    { _topics[2],0.05f },
    { _topics[3],0.03f },
    { _topics[4],0.25f }
  };
  _people.emplace_back("rnc", "engineering", "hiking", "spock", tp);

  // Initialize click probability for p2
  tp = {
    { _topics[0],0.08f },
    { _topics[1],0.30f },
    { _topics[2],0.02f },
    { _topics[3],0.02f },
    { _topics[4],0.10f }
  };
  _people.emplace_back("mk", "psychology", "kids", "7of9", tp);

  std::vector<feature_space<json_representation>> feature_spaces;
  std::transform (_topics.begin(), _topics.end(), feature_spaces.begin(),
    [](std::string topic) -> feature_space<json_representation>
      { return {{{"TAction", {{"topic", topic}}}}}; });
  actions= {feature_spaces};

  return true;
}

bool rl_sim::init() {
  if ( init_rl() != err::success ) return false;
  if ( !init_sim_world() ) return false;
  return true;
}

const cb_problem_type::instance<json_representation>& rl_sim::get_action_features() {
  return actions;
}


std::string rl_sim::get_slot_features(const std::vector<std::string>& ids) {
  std::ostringstream oss;
  // example
  // R"("_slots": [ { "_id":"abc"}, {"_id":"def"} ])";
  oss << R"("_slots": [ )";
   for ( auto idx = 0; idx < ids.size() - 1; ++idx) {
    oss << R"({ "_id":")" << ids[idx] << R"("}, )";
  }
  oss << R"({ "_id":")" << ids.back() << R"("}] )";
  return oss.str();
}

void rl_sim::on_error(const reinforcement_learning::api_status& status) {
  std::cout << "Background error in Inference API: " << status.get_error_msg() << std::endl;
  std::cout << "Exiting simulation loop." << std::endl;
  _run_loop = false;
}

std::string rl_sim::create_context_json(const std::string& cntxt, const std::string& action) {
  std::ostringstream oss;
  oss << "{ " << cntxt << ", " << action << " }";
  return oss.str();
}

std::string rl_sim::create_context_json(const std::string& cntxt, const std::string& action, const std::string& slots ) {
  std::ostringstream oss;
  oss << "{ " << cntxt << ", " << action << ", " << slots << " }";
  return oss.str();
}

std::string rl_sim::create_event_id() {
  return boost::uuids::to_string(boost::uuids::random_generator()());
}

rl_sim::rl_sim(boost::program_options::variables_map vm) : _options(std::move(vm)), ccb_mode(_options["ccb"].as<bool>()) {}

std::string get_dist_str(const reinforcement_learning::ranking_response& response) {
  std::string ret;
  ret += "(";
  for (const auto& ap_pair : response) {
    ret += "[" + to_string(ap_pair.action_id) + ",";
    ret += to_string(ap_pair.probability) + "]";
    ret += " ,";
  }
  ret += ")";
  return ret;
}

std::string get_dist_str(const reinforcement_learning::decision_response& response) {
  std::string ret;
  ret += "(";
  for (const auto& resp : response) {
    ret += get_dist_str(resp);
    ret += " ,";
  }
  ret += ")";
  return ret;
}
