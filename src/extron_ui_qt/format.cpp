#include "extron_ui_qt/format.h"


#include <algorithm>
#include <sstream>
#include <iomanip>


using namespace extron::ui_qt;


int format::determine_emoji_from_progress(float progress)
{
    if (progress == 0.f)
      return 1;

    progress += 0.999f;

    float const abs_progress = std::abs(progress);
    int emoji_count = 1;
    if (abs_progress >= 1.f)
      emoji_count += int(std::log(abs_progress));
    return emoji_count * (std::signbit(progress) ? -1 : 1);
}


std::string format::format_progress_to_emoji(float progress)
{
    auto emoji_count = determine_emoji_from_progress(progress);

    std::stringstream ss;

    char const *character_use = emoji_count > 0 ? u8"⭐" : u8"🌊";
    emoji_count = std::abs(emoji_count);

    int const abs_emoji_count = std::min(19, emoji_count);
    int const emoji_split = emoji_count > 6 ? emoji_count / 2 + 1 : 9999;
    if (abs_emoji_count > 0) {
      for (int i = 0; i < abs_emoji_count; i++) {
        if (i == emoji_split)
          ss << "\n";
        ss << character_use;
      }
    }

    return ss.str();
}


std::string format::format_balance_to_emoji(float progress)
{
    if (progress == 0.f)
      return "";

    float const abs_progress = std::abs(progress);
    int emoji_count = 1;
    if (abs_progress >= 1.f)
      emoji_count += int(std::log(abs_progress));

    std::stringstream ss;

    char const *character_use = progress > 0.f ? u8"💧" : u8"🍀";
    int const abs_emoji_count = std::min(19, emoji_count);
    int const emoji_split = emoji_count > 6 ? emoji_count / 2 + 1 : 9999;
    if (abs_emoji_count > 0) {
      for (int i = 0; i < abs_emoji_count; i++) {
        if (i == emoji_split)
          ss << "\n";
        ss << character_use;
      }
    }

    return ss.str();
}


std::string format::format_median_range(std::pair<float, float> const &range)
{
	int low = int(range.first);
	int high = int(range.second);

	if (low == high)
	  return std::to_string(low);

	std::stringstream ss;
	ss << low << "-" << high;

	return ss.str();
}


std::string format::format_momentum(float momentum)
{
    if (std::abs(momentum) < .2f)
      return "";
    return momentum > 0 ? "+" : "-";
}


std::vector<format::exe_data_ex> format::get_filtered_sorted_exercises(core::derived_exercise_state const *state, workout_filter const &filter)
{
    std::vector<format::exe_data_ex> exercises;

    std::string filter_name = filter.name;

    for (auto const& exe : state->extercises) {
        if (   filter_name.length() > 0
            && std::search(
              exe.second.display_name.begin(), exe.second.display_name.end(),
              filter_name.begin(), filter_name.end(),
              [](char ch1, char ch2) {
                return std::toupper(ch1) == std::toupper(ch2);
              }
            ) == exe.second.display_name.end()
        ) {
          continue;
        }
        
        exe_data_ex data;
        *((exe_data*)&data) = exe.second;
        
        if (!exe.second.is_suspended) {
            auto const found_cat = state->categories.find(data.category);
            if (found_cat != state->categories.end()) {
                data.category_display_name = found_cat->second.display_name;
                data.category_sort_name = found_cat->second.sort_name;
            }
            else {
                data.category_display_name = "No Category";
            }
        }
        else {
            data.category_display_name = "Suspended";
            data.category_sort_name = "zzzzzzzzzzzzzz";
        }
        
        exercises.push_back(data);
    }

    std::sort(
      exercises.begin(), exercises.end(),
      [=](exe_data_ex const &lh, exe_data_ex const &rh) -> bool {
        if (lh.category_sort_name != rh.category_sort_name)
          return lh.category_sort_name < rh.category_sort_name;
        return lh.sort_name < rh.sort_name;
      }
    );

    return exercises;
}