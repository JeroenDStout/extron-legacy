#include "extron_core/data.h"

#include <cmath>
#include <unordered_map>


using namespace extron::core;


extron_data::extron_data()
{
    this->clear();
    
    day_offset_and_weight.push_back({ -2, .08f });
    day_offset_and_weight.push_back({ -1, .15f });
    day_offset_and_weight.push_back({  0, .30f });
    day_offset_and_weight.push_back({  1, .20f });
    day_offset_and_weight.push_back({  2, .15f });
    day_offset_and_weight.push_back({  3, .08f });
    day_offset_and_weight.push_back({  4, .04f });
}


extron_data::~extron_data()
{
}


void extron_data::clear()
{
    this->description.reset(new data_description);
    this->history.reset(new data_history);
    this->derived_exercise_state_data.reset(nullptr);
}


void extron_data::replace_description(tinyxml2::XMLNode const *xml)
{
    this->description->clear_and_load(xml);
    this->derived_exercise_state_data.reset(nullptr);
}


void extron_data::replace_history(tinyxml2::XMLNode const *xml)
{
    this->history->clear_and_load(xml);
    this->derived_exercise_state_data.reset(nullptr);
}


void extron_data::save_history(tinyxml2::XMLNode *xml) const
{
    this->history->save(xml);
}


derived_exercise_state const& extron_data::get_derived_exercise_state()
{
    if (!this->derived_exercise_state_data.get())
      this->update_derived();
    return *this->derived_exercise_state_data.get();
}


bool extron_data::get_workout_data(data_history::workout_time const &previous_time, data_history::workout *workout) const
{
    return this->history->get_workout_data(previous_time, workout);
}


void extron_data::save_workout_data(data_history::workout_time const &previous_time, data_history::workout_time &inout_new_time, data_history::workout const &workout)
{
    this->history->save_workout_data(previous_time, inout_new_time, workout);
    this->derived_exercise_state_data.reset(nullptr);
}


balance_proc_structure* extron_data::create_balance_proc_structure()
{
    balance_proc_structure* balance = new balance_proc_structure();
    auto const &state        = this->get_derived_exercise_state();
    auto const &workout_data = this->history->get_workout_data();

    size_t exercise_count = 0;
    for (auto const& exe : state.extercises) 
      exercise_count += exe.second.instance_count > 0 ? 1 : 0;

    std::map<std::string, int> indexes;
    balance->exercise_weight     = new float[exercise_count];
    balance->exercise_weight_min = new float[exercise_count];

    int idx = 0;
    for (auto const& exe : state.extercises) {
        if (exe.second.instance_count == 0)
          continue;
        balance->exercise_names.push_back(exe.first);
        balance->exercise_weight[idx]     = this->exercise_balance[exe.first];
        balance->exercise_weight_min[idx] = 0.f;
        indexes[exe.first] = idx;
        ++idx;
    }
      
    std::time_t first_date = workout_data.begin()->first.time;
    std::time_t last_date  = workout_data.rbegin()->first.time;

    for (auto workout = workout_data.rbegin(); workout != workout_data.rend(); ++workout) {
        for (auto const& count : workout->second.workout_counts) {
            if (count.second.count > 0)
              goto _effort;
        }
        continue;

      _effort:
        last_date = workout->first.time;
        break;
    }

    first_date -= first_date % (24 * 3600);
    last_date  -= last_date % (24 * 3600);

    size_t const       exe_count    = exercise_count;
    std::int32_t const day_count    = std::int32_t(1 + int((last_date - first_date) / (24 * 3600)));
    std::int32_t const day_count_ex = day_count + 2;
    size_t const       data_size    = exercise_count * day_count;
    size_t const       data_size_ex = exercise_count * day_count_ex;

    std::unordered_map<std::string, std::uint32_t> peak_exercise;
    
    balance->day_weights = new float[data_size];
    float *tmp_data = new float[data_size_ex];
    
    memset(balance->day_weights, 0x0, sizeof(float) * data_size);
    memset(tmp_data,             0x0, sizeof(float) * data_size_ex);
    balance->day_count = day_count;

    for (auto const& workout : workout_data) {
        std::int32_t const idx_day = int((workout.first.time - first_date) / (24 * 3600));
        if (idx_day >= day_count_ex)
          continue;

        for (auto const& exercise : workout.second.workout_counts) {
            int const idx_exercise = indexes[exercise.first];
            tmp_data[idx_day*exe_count + idx_exercise] += state.get_effort_for_exercise_count_pure(exercise.first, exercise.second.count);
            peak_exercise[exercise.first] = std::max(peak_exercise[exercise.first], exercise.second.count);
        }
    }
    
    idx = -1;
    for (auto const& exe : state.extercises) {
        if (exe.second.instance_count == 0)
          continue;
          
        ++idx;

        if (exe.second.peak_min <= 0.f)
          continue;

        int peak_count = peak_exercise[exe.first];
        if (peak_count == 0)
          continue;

        float ratio = exe.second.peak_min / (float(peak_count) / exe.second.weighed_median);
        balance->exercise_weight_min[indexes[exe.first]] = std::log(ratio);
    }

    for (auto const& off : this->day_offset_and_weight) {
        for (int read_idx = 0; read_idx < day_count_ex; read_idx++) {
            const int write_idx = read_idx + off.first;
            if (write_idx < 0)
              continue;
            if (write_idx >= day_count)
              break;

            float *read_itt = tmp_data + read_idx*exe_count;
            float *write_itt = balance->day_weights + write_idx*exe_count;
            float *read_end = read_itt + exe_count;

            while (read_itt < read_end) {
              *write_itt += *read_itt * off.second;
              read_itt++;
              write_itt++;
            }
        }
    }

    return balance;
}


void extron_data::update_derived()
{
    time_t raw_current_time;
    time(&raw_current_time);

    this->derived_exercise_state_data.reset(new derived_exercise_state);

    this->derived_exercise_state_data->categories.clear();
    this->derived_exercise_state_data->extercises.clear();
    
    this->update_derived_balance();

    data_description::described_day day_desc;
    this->description->described_day_init(day_desc);

    struct progress_type {
        struct value_type {
            float value, weight;
            bool operator<(value_type const& rh) {
                return this->value < rh.value;
            }
        };
        std::vector<value_type> median_values;
        float sum_median_weight;
        float relative_weight;
        float time_relative_advance;
        float momentum;
        std::time_t last_performed;
        
        float get_median_factor(float fraction) {
            if (median_values.size() == 0)
              return 0.f;
            
            float target = sum_median_weight * fraction;
            for (auto const& value : median_values) {
                target -= value.weight;
                if (target <= 0.f)
                  return value.value;
            }
            return median_values[median_values.size()-1].value;
        }
        
        void add_to_median(float value, float weight) {
            value_type median{value, weight};
            auto itt = std::lower_bound(median_values.begin(), median_values.end(), median);
            median_values.insert(itt, median);
            sum_median_weight += weight;
        }
    };
    std::map<std::string, progress_type> progress;

    for (auto const &workout : this->history->get_workout_data()) {
        if (workout.second.workout_counts.size() == 0)
          continue;
        
        float const time_weight = std::exp2f(
            float(std::difftime(raw_current_time, workout.first.time) / 3600. / 24.) * -1e-2f
        );
        
        float exercise_weight = 1.f;
        if (workout.second.type == "Incidental")
          exercise_weight = 1e-2f;
        
        this->description->described_day_next(day_desc, workout.first.time);
        
        // Update weights
        float sum_weight = 0.f;
        for (auto const &exe : day_desc.extercises) {
            float rel_weight = exe.second.weight * day_desc.categories[exe.second.category].weight;
            if (exe.second.is_suspended)
              rel_weight = 0.f;
            progress[exe.first].relative_weight = rel_weight;
            sum_weight += rel_weight;
        }
        for (auto &exe : progress)
          exe.second.relative_weight /= sum_weight;
        
        for (auto const &exe : workout.second.workout_counts) {
            if (exe.second.count == 0)
              continue;
            
            auto &exe_progress = progress[exe.first];
            
            if (exe_progress.median_values.size() > 0) {
                exe_progress.momentum *= .5f;
                exe_progress.momentum += .5f * (exe.second.count - exe_progress.get_median_factor(.5f));
            }
            else {
                exe_progress.momentum = 0.f;
            }
            
            exe_progress.add_to_median((float)exe.second.count, exercise_weight * time_weight);
            exe_progress.last_performed = workout.first.time;
            float avg_median = .5f * (exe_progress.get_median_factor(.25f) + exe_progress.get_median_factor(.75f));
            float relative_impact = float(exe.second.count) / avg_median;
            
            progress[exe.first].time_relative_advance += relative_impact;
            
            for (auto &elem : progress)
              elem.second.time_relative_advance -= elem.second.relative_weight * relative_impact;
        }
    }

    this->description->described_day_next(day_desc, raw_current_time);

    for (auto const cat : day_desc.categories) {
        auto& category = this->derived_exercise_state_data->categories[cat.first];
        category.display_name = cat.second.display_name;
        category.sort_name = cat.second.sort_name;
    }

    float avg_balance = 0.f;
    int balance_count = 0;
    for (auto const& balance : this->exercise_balance) {
        if (progress[balance.first].get_median_factor(.5f) == 0.f)
          continue;
        avg_balance += balance.second;
        balance_count += 1;
    }
    if (balance_count > 0)
      avg_balance /= float(balance_count);

    float avg_advance = 0.f;
    int avg_advance_count = 0;

    for (auto const exe : day_desc.extercises) {
        auto& exercise                       = this->derived_exercise_state_data->extercises[exe.first];
        auto& exe_progress                   = progress[exe.first];
        exercise.name                        = exe.first;
        exercise.category                    = exe.second.category;
        exercise.display_name                = exe.second.display_name;
        exercise.sort_name                   = exe.second.sort_name;
        exercise.unit                        = exe.second.unit;
        exercise.weight                      = exe.second.weight;
        exercise.weighed_median_range.first  = exe_progress.get_median_factor(.25f);
        exercise.weighed_median_range.second = exe_progress.get_median_factor(.75f);
        exercise.weighed_median              = .5f * (exercise.weighed_median_range.first + exercise.weighed_median_range.second);
        exercise.momentum                    = exe_progress.momentum;
        exercise.relative_balance            = exercise.weighed_median != 0.f ? this->exercise_balance[exe.first] - avg_balance : 0.f;
        exercise.time_relative_advance       = progress[exe.first].time_relative_advance;
        if (exe_progress.last_performed == 0)
          exe_progress.last_performed        = exe.second.first_mentioned;
        exercise.interval_relative_advance   = (
            -std::log2(std::max(1.f, 1.f + float(raw_current_time - exe_progress.last_performed) / (3600.f * 24.f)))
        );
        exercise.full_relative_advance       = exercise.time_relative_advance * 2.f + exercise.interval_relative_advance * 2.f;
        exercise.peak_min                    = exe.second.peak_min;
        exercise.instance_count              = (int)exe_progress.median_values.size();
        exercise.is_suspended                = exe.second.is_suspended;
        
        if (!exercise.is_suspended)
        {
            avg_advance       += exercise.full_relative_advance;
            avg_advance_count += 1;
        }
    }

    avg_advance /= float(avg_advance_count);
    for (auto& exe : this->derived_exercise_state_data->extercises) {
      exe.second.full_relative_advance -= avg_advance;
    }

    for (auto const &data : this->history->get_workout_data()) {
      derived_exercise_state::workout_data &workout = this->derived_exercise_state_data->workouts[data.first];
      workout.type            = data.second.type;
      workout.total_exercises = (int)data.second.workout_counts.size();
      workout.total_effort    = 0.f;
      workout.dominant_exercise.clear();

      std::pair<std::string, float> top_exercise{ "", 0.f };

      for (auto const &counts : data.second.workout_counts) {
        float effort = this->derived_exercise_state_data->get_effort_for_exercise_count_balanced(counts.first, counts.second.count);
        if (effort > top_exercise.second) {
          top_exercise.first  = counts.first;
          top_exercise.second = effort;
        }
        workout.total_effort += effort;
      }

      if (top_exercise.second > workout.total_effort * 0.4f) {
        workout.dominant_exercise = top_exercise.first;
      }
    }

    update_derived_chart();
}


void extron_data::adjust_balance(std::map<std::string, float> const &balance)
{
    for (auto const& elem : balance)
      this->exercise_balance[elem.first] = elem.second;

    this->update_derived();
}


std::string const& extron_data::get_nickname() const
{
    static const std::string str_unloaded = "NOT LOADED";

    if (!this->description.get())
      return str_unloaded;

    return this->description->get_nickname();
}


void extron_data::update_derived_chart()
{
    auto &derived_state = *this->derived_exercise_state_data.get();
    auto const& workout_data = this->history->get_workout_data();

    std::time_t first_date = workout_data.begin()->first.time;
    std::time_t last_date = workout_data.rbegin()->first.time;

    for (auto workout = workout_data.rbegin(); workout != workout_data.rend(); ++workout) {
        for (auto const& count : workout->second.workout_counts) {
          if (count.second.count > 0)
            goto _effort;
        }
        continue;
      _effort:
        last_date = workout->first.time;
        break;
    }

    first_date -= first_date % (24 * 3600);
    last_date -= last_date % (24 * 3600);
    //last_date -= (24 * 3600);

    int const day_count = 1 + int((last_date - first_date) / (24 * 3600));

    derived_state.chart_effort.clear();
    derived_state.chart_effort.reserve(day_count);
    
    std::time_t itt_date = first_date + (12 * 3600);
    for (int i = 0; i < day_count; i++) {
      derived_state.chart_effort.push_back( { itt_date, 0.f} );
      itt_date += 24 * 3600;
    }

    for (auto const& workout : workout_data) {
        int const day_base_offset = int((workout.first.time - first_date) / (24 * 3600));
        float sum = 0.f;

        for (auto const& exercise : workout.second.workout_counts) {
          sum += derived_state.get_effort_for_exercise_count_balanced(exercise.first, exercise.second.count);
        }

        for (auto const& offset : this->day_offset_and_weight) {
          int const day_spread_idx = day_base_offset + offset.first;
          if (day_spread_idx < 0 || day_spread_idx >= (int)derived_state.chart_effort.size())
            continue;

          auto &day_data = derived_state.chart_effort[day_spread_idx];
          day_data.second += sum * offset.second;
        }
    }
}


void extron_data::update_derived_balance()
{
    this->derived_exercise_state_data->exercise_balance.clear();

    for (auto const& exe : this->exercise_balance)
      this->derived_exercise_state_data->exercise_balance[exe.first] = std::exp(exe.second);
}


float derived_exercise_state::get_effort_for_exercise_count_pure(std::string const &name, int count) const
{
    if (count == 0)
      return 0.f;

    auto const& exercise = this->extercises.find(name);
    if (exercise == this->extercises.end())
      return 6.f;

    if (exercise->second.weighed_median == 0.f)
      return 6.f;

    return float(count) / exercise->second.weighed_median;
}


float derived_exercise_state::get_effort_for_exercise_count_balanced(std::string const &name, int count) const
{
    auto const& balance = this->exercise_balance.find(name);
    if (balance == this->exercise_balance.end())
      return this->get_effort_for_exercise_count_pure(name, count);
    return this->get_effort_for_exercise_count_pure(name, count) * balance->second;
}


data_history::workout_time::workout_time(std::time_t time, uint32_t uid):
  time(time), uid(uid)
{
}


bool data_history::workout_time::operator<(data_history::workout_time const& other) const
{
    if (this->time != other.time)
      return this->time < other.time;
    return this->uid < other.uid;
}


bool data_history::workout_time::operator==(data_history::workout_time const& other) const
{
    return (
      this->time == other.time
      && this->uid == other.uid
    );
}


bool data_history::workout_time::operator!=(data_history::workout_time const& other) const
{
    return !(*this == other);
}


balance_proc_structure::balance_proc_structure():
  exercise_weight(nullptr),
  day_weights(nullptr)
{
}


balance_proc_structure::~balance_proc_structure()
{
    delete[] exercise_weight;
    delete[] day_weights;
}


void balance_proc_structure::update_step()
{
    int const exe_count = (int)this->exercise_names.size();
    static const float eff_per_day = 1000.f / 7.f;

    std::vector<float> effective_weights(exe_count);
    std::vector<float> adapting_weights(exe_count);

    for (int idx = 0; idx < exe_count; idx++) {
        effective_weights[idx] = std::exp(this->exercise_weight[idx]);
        adapting_weights[idx] = 0.f;
    }

    float sum_error = 0.f;

    for (std::size_t idx_day = 0; idx_day < this->day_count; idx_day++)
    {
        float const* day_read = this->day_weights + idx_day*exe_count;
      
        float day_weight = 0.f;

        for (int i = 0; i < exe_count; i++)
          day_weight += day_read[i] * effective_weights[i];

        float sqrt_error = -std::max(-1e2f, std::log2(day_weight / eff_per_day));
        float error = sqrt_error * sqrt_error;
        float deriv_error = sqrt_error;
      
        sum_error += error;

        for (int i = 0; i < exe_count; i++)
          adapting_weights[i] += day_read[i] * deriv_error;
    }

    float average = 0.f;
    float magnitude = 0.f;
    float adapting_weights_scale = 5e-2f;
    for (int idx = 0; idx < exe_count; idx++) {
        average += this->exercise_weight[idx];
        if (this->exercise_weight[idx] <= -2.f && adapting_weights[idx] < 0.f)
          adapting_weights[idx] = 0.f;
        else
          magnitude += adapting_weights[idx] * adapting_weights[idx];
    }
    average /= float(exe_count);

    if (magnitude > 1e-1f) {
        adapting_weights_scale /= std::sqrt(magnitude);
    }

    for (int idx = 0; idx < exe_count; idx++) {
        this->exercise_weight[idx] += adapting_weights[idx] * adapting_weights_scale;
        this->exercise_weight[idx] = std::max(this->exercise_weight_min[idx], this->exercise_weight[idx]);
    }
    
    std::vector<float> ranked_weights;
    ranked_weights.reserve(exe_count);
    for (int idx = 0; idx < exe_count; idx++) {
        ranked_weights.insert(
          std::upper_bound(
            ranked_weights.begin(),
            ranked_weights.end(),
            this->exercise_weight[idx]
          ),
          this->exercise_weight[idx]
        );
    }
}