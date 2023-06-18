#pragma once


#include "extron_core/data.h"


#include <string>
#include <utility>


namespace extron::ui_qt::format {

    int determine_emoji_from_progress(float progress);
    std::string format_progress_to_emoji(float progress);
    std::string format_balance_to_emoji(float progress);
    std::string format_median_range(std::pair<float, float> const&);
    std::string format_momentum(float momentum);
    
    struct workout_filter {
        std::string name;
    };
      
    using exe_data = core::derived_exercise_state::exercise_data;
    struct exe_data_ex : exe_data {
        std::string category_display_name, category_sort_name;
    };
    
    std::vector<exe_data_ex> get_filtered_sorted_exercises(core::derived_exercise_state const&, workout_filter const&);

};