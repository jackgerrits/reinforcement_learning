#include "decision_response.h"
#include "api_status.h"
#include "err_constants.h"

namespace reinforcement_learning
{
  void decision_response::push_back(ranking_response&& r_response) {
    _decision.emplace_back(std::move(r_response));
  }

  size_t decision_response::size() const {
    return _decision.size();
  }

  void decision_response::set_model_id(const char* model_id) {
    _model_id = model_id;
  }
  void decision_response::set_model_id(std::string&& model_id) {
    _model_id.assign(std::move(model_id));
  }
  const char* decision_response::get_model_id() const {
    return _model_id.c_str();
  }

  bool decision_response::is_sample_slot_set() const
  {
    return _sample_slot_set;
  }

  int decision_response::get_sample_slot(uint32_t& slot_id, api_status* status ) const
  {
    if ( _decision.empty() ) {
      RETURN_ERROR_LS(nullptr, status, action_not_found);
    }

    slot_id = _sample_slot;
    return error_code::success;
  }

  int decision_response::set_sample_slot(uint32_t slot_id, api_status* status)
  {
    if ( slot_id >= _decision.size() ) {
      RETURN_ERROR_LS(nullptr, status, action_out_of_bounds) << " id:" << _sample_slot << ", size:" << _decision.size();
    }

    _sample_slot = slot_id;
    _sample_slot_set = true;
    return error_code::success;
  }

  void decision_response::clear() {
    _decision.clear();
    _model_id.clear();
  }

  decision_response::const_iterator_t decision_response::begin() const {
    return _decision.cbegin();
  }

  decision_response::iterator_t decision_response::begin() {
    return _decision.begin();
  }

  decision_response::const_iterator_t decision_response::end() const {
    return _decision.cend();
  }

  decision_response::iterator_t decision_response::end() {
    return _decision.end();
  }

  decision_response::decision_response(decision_response&& other) noexcept :
    _decision(std::move(other._decision)),
    _model_id(std::move(other._model_id))
  {}

  decision_response &decision_response::operator=(decision_response&& other) noexcept {
    std::swap(_model_id, other._model_id);
    std::swap(_decision, other._decision);
    return *this;
  }
} // namespace reinforcement_learning
