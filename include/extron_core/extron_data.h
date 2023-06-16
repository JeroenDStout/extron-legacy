#pragma once


#include <memory>
#include <string>


#include "extron_core/extron_data_description.h"
#include "data_history.h"
#include "../external/tinyxml2.h"


class extron_data {
  public:
	class derived_exercise_state;
	class balance_proc_structure;
	class sorted_exercise_set;

	extron_data();
	~extron_data();

	void clear();
	void replace_description(tinyxml2::XMLNode const *);
	void replace_history(tinyxml2::XMLNode const *);

	void save_history(tinyxml2::XMLNode *) const;

	derived_exercise_state const* get_derived_exercise_state();
	bool get_workout_data(data_history::workout_time const &time, data_history::workout *out_workout) const;
	void save_workout_data(data_history::workout_time const &previous_time, data_history::workout_time &inout_new_time, data_history::workout const&);
	
	balance_proc_structure* create_balance_proc_structure();
	void adjust_balance(std::map<std::string, float> const&);

	std::string const& get_nickname() const;

  protected:
	void update_derived();
	void update_derived_balance();
	void update_derived_chart();
  
	std::vector<std::pair<int, float>> day_offset_and_weight;
	std::map<std::string, float> exercise_balance;
	std::unique_ptr<data_description> description;
	std::unique_ptr<data_history> history;
	std::unique_ptr<derived_exercise_state> derived_exercise_state_data;
};


class extron_data::derived_exercise_state {
  public:
	struct category_data {
	  std::string display_name, sort_name;
	};
	struct exercise_data {
	  std::string name;
	  std::string category;
	  std::string display_name, sort_name;
	  std::string unit;
	  float weight, relative_weight;
	  float time_relative_advance;
	  float interval_relative_advance;
	  float full_relative_advance;
	  float weighed_median;
      float momentum;
	  std::pair<float, float> weighed_median_range;
	  float relative_balance;
	  float rank_min, rank_max;
      bool is_suspended;
	  time_t last_time;
	  int instance_count;
	};
	struct workout_data {
	  std::string type;
	  std::string dominant_exercise;
	  int total_exercises;
	  float total_effort;
	};

	std::map<std::string, category_data>				categories;
	std::map<std::string, exercise_data>				extercises;
	std::map<data_history::workout_time, workout_data>	workouts;
	std::map<std::string, float>						exercise_balance;

	std::vector<std::pair<std::time_t, float>>			chart_effort;
	
	float get_effort_for_exercise_count_pure(std::string const&, int count) const;
	float get_effort_for_exercise_count_balanced(std::string const&, int count) const;
};


class extron_data::balance_proc_structure {
  public:
	balance_proc_structure();
	~balance_proc_structure();

	std::vector<std::string> exercise_names;
	float *exercise_weight;
	float *exercise_rank_min;
	float *exercise_rank_max;
	int	day_count;
	float *day_weights;

	void update_step();
};