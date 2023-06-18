#include "extron_core/data_history.h"

#include <iomanip>
#include <sstream>


using namespace extron::core;


data_history::data_history()
{
}


data_history::~data_history()
{
}


void data_history::clear()
{
    this->workout_data.clear();
}


void data_history::clear_and_load(tinyxml2::XMLNode const *dict)
{
    this->clear();

    tinyxml2::XMLElement const *xml_meta = nullptr;
    tinyxml2::XMLElement const *xml_workouts = nullptr;

    for (auto entry = dict->FirstChild(); entry; entry = entry->NextSibling()) {
        if (0 == strcmp(entry->Value(), "meta"))
          xml_meta = entry->ToElement();
        else if (0 == strcmp(entry->Value(), "workouts"))
          xml_workouts = entry->ToElement();
    }

    if (xml_workouts) {
        for (auto entry = xml_workouts->FirstChild(); entry; entry = entry->NextSibling()) {
          if (0 == strcmp(entry->Value(), "workout")) 
            this->deserialise_from_xml_workout(entry);
        }
    }
}


void data_history::deserialise_from_xml_workout(tinyxml2::XMLNode const *node)
{
    workout_time time(0, 0);

    char const *date = node->ToElement()->Attribute("date");
    if (!date || !date[0])
      return;
    char const *type = node->ToElement()->Attribute("type");
    if (!type || !type[0])
      return;

    // Deserialise the date

    std::tm tm;
    memset(&tm, 0x0, sizeof(std::tm));

    std::istringstream ss(date);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M");
    time.time = _mkgmtime(&tm);

    // Obtain the workout

    for (;;) {
        if (this->workout_data.find(time) == this->workout_data.end())
          break;
        time.uid += 1;
    }

    auto &workout = this->workout_data[time];
    workout.type = type;

    // Load workout data

    for (auto entry = node->FirstChild(); entry; entry = entry->NextSibling()) {
        if (0 == strcmp(entry->Value(), "x")) {
            char const *name = entry->ToElement()->Attribute("n");
            if (!name || !name[0])
              continue;
        
            workout::counts counts;
            counts.count = entry->ToElement()->IntAttribute("c");
            counts.target = entry->ToElement()->IntAttribute("t");
        
            workout.workout_counts[name] = counts;
            continue;
        }
    }
}


bool data_history::get_workout_data(data_history::workout_time const &time, data_history::workout *out_workout) const
{
    auto const &prev = this->workout_data.find(time);
    if (prev == this->workout_data.end())
      return false;

    *out_workout = prev->second;
    return true;
}


void data_history::save_workout_data(data_history::workout_time const &previous_time, data_history::workout_time &inout_new_time, data_history::workout const &workout)
{
    if (previous_time != inout_new_time) {
      this->workout_data.erase(previous_time);
      
      while (true) {
        auto const &prev = this->workout_data.find(inout_new_time);
        if (prev == this->workout_data.end())
          break;
        inout_new_time.uid += 1;
      }
    }

    this->workout_data[inout_new_time] = workout;
}


const std::map<data_history::workout_time, data_history::workout>& data_history::get_workout_data() const
{
    return this->workout_data;
}