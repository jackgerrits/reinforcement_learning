#include "person.h"
#include <utility>
#include <sstream>

person::person(std::string id, std::string major,
               std::string hobby, std::string fav_char,
               topic_prob &p) : _id(std::move(id)), _topic_click_probability{p},
  shared_context{{
    {
      {"GUser",
        {
          {"id", _id},
          {"major", major},
          {"hobby", hobby},
          {"favorite_character", fav_char},
        }
    }
  }}}
{
}

person::~person() = default;

const feature_space<json_representation>& person::get_features() const
{
  return shared_context;
}

float person::get_outcome(const std::string &chosen_action)
{
  int const draw_uniform = rand() % 10000;
  float const norm_draw_val = static_cast<float>(draw_uniform) / 10000.0f;
  float const click_prob = _topic_click_probability[chosen_action];
  if (norm_draw_val <= click_prob)
    return 1.0f;
  else
    return 0.0f;
}

std::string person::id() const
{
  return _id;
}
